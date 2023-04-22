#include "client.h"
#include "../capture/capture.h"
#include "../cyborg/control.h"
#include "../data/stack.h"
#include "../util/util.h"
#include "read_state.h"
#include "uv.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

void on_connect(uv_connect_t *connection, int status);
void on_close_cb(uv_handle_t *handle);

struct NetClient *setup_client(struct DStack *frame_stack,
                               struct DStack *ctrl_stack) {
  int err;
  struct NetClient *client;
  client = calloc(1, sizeof(*client));
  if (!client)
    return NULL;

  client->frame_stack = frame_stack;
  client->ctrl_stack = ctrl_stack;
  client->read_state = calloc(1, sizeof(*client->read_state));
  client->loop = uv_default_loop();

  uv_idle_init(client->loop, &client->idle);
  client->idle.data = client;

  uv_cond_init(&client->cond);
  uv_mutex_init(&client->mutex);
  return client;
}

int destroy_client(struct NetClient *client) {
  if (client->tcp_stream)
    uv_close((uv_handle_t *)client->tcp_stream, on_close_cb);
  if (client->tcp_socket)
    free(client->tcp_socket);
  uv_loop_close(client->loop);
  free(client);
  return 0;
}

int connect_client(struct NetClient *c, const char *ip) {
  uv_connect_t *conn_req;
  c->tcp_socket = malloc(sizeof(*c->tcp_socket));
  uv_tcp_init(c->loop, c->tcp_socket);
  uv_tcp_keepalive(c->tcp_socket, 1, 60);
  struct sockaddr_in dest;
  uv_ip4_addr(ip, CALM_PORT, &dest);
  conn_req = malloc(sizeof(*conn_req));
  conn_req->data = c;
  uv_tcp_connect(conn_req, c->tcp_socket, (const struct sockaddr *)&dest,
                 on_connect);
  return uv_run(c->loop, UV_RUN_DEFAULT);
}

static void client_alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
  struct NetClient *client = (struct NetClient *)handle->data;
  struct ReadState *read_state = client->read_state;
  uint8_t *buffer;
  size_t length;

  read_state_alloc_buffer(read_state, &buffer, &length);
  *buf = uv_buf_init((char *)buffer, length);
}

void on_close_cb(uv_handle_t *handle) {
#if 0
  printf("connection closed\n")
#endif
  free(handle);
}

void on_write(uv_write_t *req, int status) {
  if (status)
    uv_close((uv_handle_t *)req->handle, on_close_cb);
  free(req);
}

// NOTE: https://groups.google.com/g/libuv/c/fRNQV_QGgaA

void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
  int buffer_ready = 0;
  struct CFrame *frame;
  struct NetClient *client = (struct NetClient *)stream->data;
  struct ReadState *state = client->read_state;

  if (nread < 0) {
    if (nread != UV_EOF) {
      printf("read error %d %s\n", state->state, uv_err_name(nread));
      // uv_close((uv_handle_t *)stream, on_close_cb);
    }
  }

  buffer_ready = read_state_handle_buffer(state, (uint8_t *)buf->base, nread);

  if (!buffer_ready)
    return;

  switch (state->packet_type) {
  case 1:
    printf("got cframe\n");
    frame = unmarshal_cframe(state->buffer, state->buf_len);
    dstack_push(client->frame_stack, (void *)frame, 1);
    break;
  default:
    printf("unknown/unhandled packet type: %d\n", state->packet_type);
    break;
  }

  // TODO: dont think we need to free buf->base as we free it through the state
  free(state->buffer);
  memset(state, 0, sizeof(*state));
}

void write_stream(uv_stream_t *stream, char *data, int len2) {
  uv_buf_t buffer[] = {{.base = data, .len = len2}};
  uv_write_t *req = malloc(sizeof(uv_write_t));
  uv_write(req, stream, buffer, 1, on_write);
}

void net_send_ctrl(uv_idle_t *handle) {
  uv_buf_t wrbuf;
  uv_write_t *req;
  struct Control *ctrl;
  struct SerializedBuffer *buf;
  struct Buffer ctrl_buffer;
  struct NetClient *client = (struct NetClient *)handle->data;

  ctrl_buffer = dstack_pop_all(client->ctrl_stack);
  if (!ctrl_buffer.length)
    return;
  for (size_t i = 0; i < ctrl_buffer.length; i++) {
    buf = ctrl_serialize_control(ctrl_buffer.elements[i]);
    if (!buf)
      continue;
    free(ctrl_buffer.elements[i]);
    // TODO: broken -> ctrl_release_control((struct Control
    // **)&ctrl_buffer.elements[i]);
    req = calloc(1, sizeof(*req));
    req->data = client;
    wrbuf = uv_buf_init((char *)buf->buffer, buf->length);
    if (!client->connected) {
      free(buf->buffer);
      free(req);
      free(buf);
      return;
    }
    free(buf);
    uv_write(req, (uv_stream_t *)client->tcp_socket, &wrbuf, 1, on_write);
  }
}

void on_connect(uv_connect_t *connection_req, int status) {
  struct NetClient *client = (struct NetClient *)connection_req->data;

  if (status < 0)
    printf("conn failed %d\n", status);
  else
    printf("connected.\n");

  client->connected = 1;
  client->tcp_stream = connection_req->handle;
  client->tcp_stream->data = client;
  free(connection_req);
  uv_read_start(client->tcp_stream, client_alloc_cb, on_read);
  uv_idle_start(&client->idle, net_send_ctrl);
}
