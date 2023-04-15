#include "client.h"
#include "../capture/capture.h"
#include "../data/stack.h"
#include "uv.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

void on_connect(uv_connect_t *connection, int status);

struct NetClient *setup_client(struct DStack *stack) {
  int err;
  struct NetClient *client;
  client = malloc(sizeof(*client));
  if (!client)
    return NULL;
  memset(client, 0, sizeof(*client));
  client->loop = uv_default_loop();
  client->stack = stack;
  client->read_state = malloc(sizeof(*client->read_state));
  memset(client->read_state, 0, sizeof(*client->read_state));
  uv_cond_init(&client->cond);
  uv_mutex_init(&client->mutex);
  return client;
}

int destroy_client(struct NetClient *client) {
  if (client->tcp_stream) {
    // TODO: close callback
    uv_close((uv_handle_t *)client->tcp_stream, NULL);
    free(client->tcp_stream);
  }
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
    *buf = uv_buf_init(malloc(8), 8);
    break;
  case 1:
    // Allocate bytes required to read the frame
    *buf = uv_buf_init(malloc(client->read_state->buf_len),
                       client->read_state->buf_len);
    break;
  default:
    // UNREACHABLE
    break;
  }
}

void on_close(uv_handle_t *handle) {
#if 0
  printf("connection closed\n")
#endif
  free(handle);
  // TODO: get rid of pthread
  pthread_exit(NULL);
}

void on_write(uv_write_t *req, int status) {
  if (status)
    uv_close((uv_handle_t *)req->handle, NULL);
  free(req);
}

// NOTE: https://groups.google.com/g/libuv/c/fRNQV_QGgaA

void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
  struct CFrame *frame;
  struct NetClient *client = (struct NetClient *)stream->data;

  if (nread < 0) {
    if (nread != UV_EOF) {
      printf("Read error %s\n", uv_err_name(nread));
      uv_close((uv_handle_t *)stream, NULL);
      pthread_exit(NULL);
    }
  }

  switch (client->read_state->state) {
  case 0:
    if (nread != 8 || buf->len != 8) {
      printf("failed to read total buffer length %lu %lu\n", nread, buf->len);
      return;
    }
    client->read_state->buf_len = read_uint64((uint8_t *)buf->base);
    printf("got buf len %llu\n", client->read_state->buf_len);
    client->read_state->state = 1;
    break;
  case 1:
    if ((uint64_t)nread != client->read_state->buf_len ||
        buf->len != client->read_state->buf_len) {
      printf("failed to read total buffer %lu %lu\n", nread, buf->len);
      return;
    }
    frame = unmarshal_cframe((uint8_t *)buf->base, buf->len);
    dstack_push(client->stack, (void *)frame, 1);
    memset(client->read_state, 0, sizeof(*client->read_state));
    break;
  }

  if (buf->base) {
    free(buf->base);
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
