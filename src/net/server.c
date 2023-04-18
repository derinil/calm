#include "server.h"
#include "../capture/capture.h"
#include "../data/stack.h"
#include "uv.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

void on_new_connection(uv_stream_t *server, int status);
void on_write_callback(uv_write_t *req, int status);
void on_close_callback(uv_handle_t *handle);
void async_callback(uv_async_t *async);

struct AsyncWriteRequest {
  uv_write_t *req;
  uv_stream_t *handle;
  uv_buf_t *buffer;
  int s;
};

struct NetServer *net_setup_server(struct DStack *stack) {
  int err;
  struct NetServer *server;
  struct sockaddr_in addr;
  server = malloc(sizeof(*server));
  if (!server)
    return NULL;
  memset(server, 0, sizeof(*server));

  err = uv_mutex_init(&server->mutex);
  if (err)
    return NULL;

  server->stack = stack;
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
  uv_buf_t wrbuf;
  uv_write_t *req;
  struct CFrame *frame;
  struct SerializedBuffer *buf;
  struct NetServer *server = (struct NetServer *)handle->data;

  if (!server->connected)
    return;

  frame = (struct CFrame *)dstack_pop_nonblock(server->stack);
  if (!frame || frame->nalus_count == 0)
    return;
  buf = serialize_cframe(frame);
  if (!buf)
    return;
  release_cframe(frame);
  req = malloc(sizeof(*req));
  memset(req, 0, sizeof(*req));
  req->data = server;
  wrbuf = uv_buf_init((char *)buf->buffer, buf->length);
  // we can free the buffer here without freeing the actual underlying char
  // array
  free(buf);

  uv_write(req, (uv_stream_t *)server->tcp_client, &wrbuf, 1,
           on_write_callback);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}

void on_write_callback(uv_write_t *req, int status) {
  struct NetServer *server = (struct NetServer *)req->data;
  if (status) {
    printf("Write error %s\n", uv_strerror(status));
    uv_close((uv_handle_t *)server->tcp_client, on_close_callback);
  }
  free(req);
}

void on_read_callback(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
  struct NetServer *server = (struct NetServer *)client->data;
  if (nread < 0) {
    if (nread != UV_EOF) {
      printf("Read error %s\n", uv_err_name(nread));
      uv_close((uv_handle_t *)client, on_close_callback);
    }
  }

  if (buf->base) {
    free(buf->base);
  }
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
  uv_read_start((uv_stream_t *)client, alloc_buffer, on_read_callback);
  uv_idle_start(&server->idle, net_send_frame);
  // uv_thread_create(&server->send_loop, net_send_frames_loop, server);
}