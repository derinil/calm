#include "client.h"
#include "../capture/capture.h"
#include "../data/stack.h"
#include "uv.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

void on_connect(uv_connect_t *connection, int status);
void on_close_cb(uv_handle_t *handle);

struct NetClient *setup_client(struct DStack *stack) {
  int err;
  struct NetClient *client;
  client = malloc(sizeof(*client));
  if (!client)
    return NULL;
  memset(client, 0, sizeof(*client));
  client->loop = uv_default_loop();
  client->stack = stack;
  client->read_state = calloc(1, sizeof(*client->read_state));
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

static void read_cframe_alloc_cb(uv_handle_t *handle, size_t size,
                                 uv_buf_t *buf) {
  struct NetClient *client = (struct NetClient *)handle->data;
  switch (client->read_state->state) {
  case 0:
    // Allocate 8 bytes to read the start of a cframe
    client->read_state->buf_len_buffer =
        malloc(sizeof(*client->read_state->buf_len_buffer) * 8);
    *buf = uv_buf_init((char *)client->read_state->buf_len_buffer, 8);
    break;
  case 1:
    // Fill up the allocated frame buffer length
    *buf = uv_buf_init((char *)client->read_state->buf_len_buffer +
                           client->read_state->buf_len_off,
                       8 - client->read_state->buf_len_off);
    break;
  case 2:
    // Allocate bytes required to read the frame
    client->read_state->buffer = malloc(sizeof(*client->read_state->buffer) *
                                        client->read_state->buf_len);
    *buf = uv_buf_init((char *)client->read_state->buffer,
                       client->read_state->buf_len);
    break;
  case 3:
    // Fill up the allocated frame buffer
    *buf = uv_buf_init(
        (char *)(client->read_state->buffer + client->read_state->buf_off),
        client->read_state->buf_len - client->read_state->buf_off);
    break;
  default:
    /* UNREACHABLE */
    break;
  }
}

void on_close_cb(uv_handle_t *handle) {
#if 0
  printf("connection closed\n")
#endif
  free(handle);
  // TODO: get rid of pthread
}

void on_write(uv_write_t *req, int status) {
  if (status)
    uv_close((uv_handle_t *)req->handle, on_close_cb);
  free(req);
}

// NOTE: https://groups.google.com/g/libuv/c/fRNQV_QGgaA

void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
  struct CFrame *frame;
  struct NetClient *client = (struct NetClient *)stream->data;

  if (nread < 0) {
    if (nread != UV_EOF) {
      printf("Read error %d %s\n", client->read_state->state,
             uv_err_name(nread));
      // uv_close((uv_handle_t *)stream, on_close_cb);
      // pthread_exit(NULL);
    }
  }

  client->read_state->buf_off += nread;

  switch (client->read_state->state) {
  case 0:
    if (client->read_state->buf_off != 8) {
      client->read_state->state = 1;
      break;
    }
    /* FALLTHROUGH IF BUFFER IS FULL */
  case 1:
    if (client->read_state->buf_off != 8) {
      break;
    }
    client->read_state->buf_len =
        read_uint64(client->read_state->buf_len_buffer);
    free(client->read_state->buf_len_buffer);
    client->read_state->state = 2;
    client->read_state->buf_off = 0;
    break;
  case 2:
    if ((uint64_t)client->read_state->buf_off != client->read_state->buf_len) {
      client->read_state->state = 3;
      break;
    }
    /* FALLTHROUGH IF BUFFER IS FULL */
  case 3:
    // TODO: use != here, sometimes it overflows so ill keep it as < for now
    if (client->read_state->buf_off != client->read_state->buf_len) {
      break;
    }
    // TODO: got -12712 once
    frame = unmarshal_cframe(client->read_state->buffer,
                             client->read_state->buf_len);
    dstack_push(client->stack, (void *)frame, 1);
    free(client->read_state->buffer);
    memset(client->read_state, 0, sizeof(*client->read_state));
    break;
  default:
    if (buf->base)
      free(buf->base);
    break;
  }
}

void write_stream(uv_stream_t *stream, char *data, int len2) {
  uv_buf_t buffer[] = {{.base = data, .len = len2}};
  uv_write_t *req = malloc(sizeof(uv_write_t));
  uv_write(req, stream, buffer, 1, on_write);
}

void on_connect(uv_connect_t *connection_req, int status) {
  struct NetClient *client = (struct NetClient *)connection_req->data;

  if (status < 0)
    pthread_exit(&status);

#if 1
  printf("connected.\n");
#endif

  client->tcp_stream = connection_req->handle;
  client->tcp_stream->data = client;
  free(connection_req);
  write_stream(client->tcp_stream, "echo  world!", 12);
  uv_read_start(client->tcp_stream, read_cframe_alloc_cb, on_read);
}
