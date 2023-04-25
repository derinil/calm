#include "server.h"
#include "../capture/capture.h"
#include "../cyborg/control.h"
#include "../data/stack.h"
#include "../util/util.h"
#include "read_state.h"
#include "uv.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

struct WriteContext {
  struct NetServer *server;
  void *buffer;
};

void on_new_connection(uv_stream_t *server, int status);
void on_write_callback(uv_write_t *req, int status);
void on_close_callback(uv_handle_t *handle);

struct NetServer *net_setup_server(struct DStack *frame_stack,
                                   struct DStack *ctrl_stack) {
  int err;
  struct NetServer *server;
  struct sockaddr_in addr;
  server = calloc(1, sizeof(*server));
  if (!server)
    return NULL;

  err = uv_mutex_init(&server->mutex);
  if (err)
    return NULL;

  server->frame_stack = frame_stack;
  server->ctrl_stack = ctrl_stack;
  server->read_state = calloc(1, sizeof(*server->read_state));

  // TODO: create separate loop for net
  server->loop = uv_default_loop();
  if (!server->loop)
    return NULL;

  uv_idle_init(server->loop, &server->idle);
  server->idle.data = server;

  server->tcp_server = malloc(sizeof(*server->tcp_server));
  if (!server->tcp_server)
    return NULL;
  uv_tcp_init(server->loop, server->tcp_server);
  uv_ip4_addr("0.0.0.0", CALM_PORT, &addr);
  err = uv_tcp_bind(server->tcp_server, (const struct sockaddr *)&addr, 0);
  if (err)
    return NULL;
  server->tcp_server->data = (void *)server;
  return server;
}

int net_destroy_server(struct NetServer *server) {
  int err;
  err = uv_loop_close(server->loop);
  free(server->tcp_server);
  free(server);
  return err;
}

int net_start_server(struct NetServer *server) {
  int err;
  err = uv_listen((uv_stream_t *)server->tcp_server, 128, on_new_connection);
  if (err)
    return err;
  return uv_run(server->loop, UV_RUN_DEFAULT);
}

void net_send_frame(uv_idle_t *handle) {
  uv_buf_t *uvbuf;
  uv_write_t *req;
  uint8_t *packet_id;
  struct WriteContext *ctx = NULL;
  struct CFrame *frame = NULL;
  struct SerializedCFrame *serfc = NULL;
  struct NetServer *server = (struct NetServer *)handle->data;

  frame = dstack_pop_nonblock(server->frame_stack);
  if (!frame)
    return;
  serfc = serialize_cframe(frame);
  packet_id = create_packet_id(serfc->length, 1);
  req = calloc(1, sizeof(*req));
  ctx = malloc(sizeof(*ctx));
  ctx->server = server;
  ctx->buffer = serfc;
  req->data = ctx;

  uvbuf = (uv_buf_t[]){
      {.base = (char *)packet_id, .len = 8},
      {.base = (char *)serfc->buffer, .len = serfc->length},
  };

  uv_write(req, (uv_stream_t *)server->tcp_client, uvbuf, 2, on_write_callback);
}

static void server_alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
  struct NetServer *server = (struct NetServer *)handle->data;
  struct ReadState *read_state = server->read_state;
  uint8_t *buffer;
  size_t length;

  read_state_alloc_buffer(read_state, &buffer, &length);
  *buf = uv_buf_init((char *)buffer, length);
}

void on_write_callback(uv_write_t *req, int status) {
  struct WriteContext *ctx = req->data;
  struct NetServer *server = ctx->server;
  struct SerializedCFrame *ser = ctx->buffer;
  release_cframe(&ser->frame);
  release_serialized_cframe(ser);
  free(ctx);
  free(req);
  if (status) {
    printf("Write error %s\n", uv_strerror(status));
    uv_close((uv_handle_t *)server->tcp_client, on_close_callback);
  }
}

void on_read_callback(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
  int buffer_ready = 0;
  struct Control *ctrl;
  struct NetServer *server = (struct NetServer *)client->data;
  struct ReadState *state = server->read_state;

  if (nread < 0) {
    if (nread != UV_EOF) {
      printf("read error %d %s\n", state->state, uv_err_name(nread));
      // uv_close((uv_handle_t *)client, on_close_callback);
    }
  }

  buffer_ready = read_state_handle_buffer(state, (uint8_t *)buf->base, nread);

  if (!buffer_ready)
    return;

  switch (state->packet_type) {
  case 2:
    ctrl = ctrl_unmarshal_control(state->buffer, state->buffer_len);
    dstack_push(server->ctrl_stack, (void *)ctrl, 1);
    break;
  default:
    printf("unknown/unhandled packet type: %d\n", state->packet_type);
    break;
  }

  // TODO: dont think we need to free buf->base as we free it through the state
  free(state->buffer);
  memset(state, 0, sizeof(*state));
}

void on_close_callback(uv_handle_t *handle) {
  struct NetServer *server = (struct NetServer *)handle->data;
  printf("closed conn\n");
  uv_mutex_lock(&server->mutex);
  server->connected = 0;
  server->tcp_client = NULL;
  uv_idle_stop(&server->idle);
  uv_mutex_unlock(&server->mutex);
  free(handle);
}

void on_new_connection(uv_stream_t *stream, int status) {
  int err;
  struct NetServer *server = (struct NetServer *)stream->data;

  uv_mutex_lock(&server->mutex);
  if (server->connected)
    return;
  uv_mutex_unlock(&server->mutex);

  if (status < 0) {
    fprintf(stderr, "New connection error %s\n", uv_strerror(status));
    return;
  }

  uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
  uv_tcp_init(server->loop, client);
  err = uv_accept(stream, (uv_stream_t *)client);
  if (err) {
    uv_close((uv_handle_t *)client, on_close_callback);
    return;
  }
  uv_mutex_lock(&server->mutex);
  server->connected = 1;
  server->tcp_client = client;
  server->tcp_client->data = server;
  uv_mutex_unlock(&server->mutex);
  printf("connected\n");
  uv_read_start((uv_stream_t *)client, server_alloc_cb, on_read_callback);
  uv_idle_start(&server->idle, net_send_frame);
  // uv_thread_create(&server->send_loop, net_send_frames_loop, server);
}
