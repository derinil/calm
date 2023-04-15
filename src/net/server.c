#include "server.h"
#include "../capture/capture.h"
#include "../data/stack.h"
#include "uv.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

void on_new_connection(uv_stream_t *server, int status);
void on_write_callback(uv_write_t *req, int status);
void on_close_callback(uv_handle_t *handle);

// static struct NetServer *g_net_server;

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
  server->loop = uv_default_loop();
  if (!server->loop)
    return NULL;
  server->tcp_server = malloc(sizeof(*server->tcp_server));
  if (!server->tcp_server)
    return NULL;
  server->stack = stack;
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

void net_send_frames(struct NetServer *server) {
  uv_buf_t wrbuf;
  uv_write_t *req;
  struct CFrame *frame;
  struct SerializedBuffer *buf;

  printf("pre loop %d %d\n", server->connected, server->stack == NULL);

  while (server->connected) {
    printf("yoop\n");
    frame = (struct CFrame *)dstack_pop_block(server->stack);
    printf("serializing buffer\n");
    buf = serialize_cframe(frame);
    if (!buf)
      return;
    release_cframe(frame);
    frame = NULL;
    printf("sending buffer\n");
    req = (uv_write_t *)malloc(sizeof(uv_write_t));
    req->data = server;
    wrbuf = uv_buf_init((char *)buf->buffer, buf->length);
    // we can free the buffer here without freeing the actual underlying char
    // array
    free(buf);
    uv_write(req, (uv_stream_t *)server->tcp_client, &wrbuf, 1,
             on_write_callback);
    printf("sent buffer\n");
  }
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  buf->base = (char *)malloc(suggested_size);
  buf->len = suggested_size;
}

void on_write_callback(uv_write_t *req, int status) {
  struct NetServer *server = (struct NetServer *)req->data;
  if (status) {
    fprintf(stderr, "Write error %s\n", uv_strerror(status));
    uv_mutex_lock(&server->mutex);
    uv_close((uv_handle_t *)server->tcp_client, on_close_callback);
    uv_mutex_unlock(&server->mutex);
  }
  free(req);
}

void on_read_callback(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
  if (nread < 0) {
    if (nread != UV_EOF) {
      fprintf(stderr, "Read error %s\n", uv_err_name(nread));
      uv_close((uv_handle_t *)client, on_close_callback);
    }
  } else if (nread > 0) {
    uv_write_t *req = (uv_write_t *)malloc(sizeof(uv_write_t));
    uv_buf_t wrbuf = uv_buf_init(buf->base, nread);
    uv_write(req, client, &wrbuf, 1, on_write_callback);
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
  uv_mutex_unlock(&server->mutex);
  free(handle);
}

void on_new_connection(uv_stream_t *stream, int status) {
  int err;
  struct NetServer *server = (struct NetServer *)stream->data;

  if (status < 0) {
    fprintf(stderr, "New connection error %s\n", uv_strerror(status));
    return;
  }

  uv_tcp_t *client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
  printf("client\n");
  uv_tcp_init(server->loop, client);
  printf("init\n");
  err = uv_accept(stream, (uv_stream_t *)client);
  printf("accepted\n");
  if (err) {
    uv_close((uv_handle_t *)client, on_close_callback);
    return;
  }
  uv_mutex_lock(&server->mutex);
  server->connected = 1;
  server->tcp_client = client;
  uv_mutex_unlock(&server->mutex);
  uv_read_start((uv_stream_t *)client, alloc_buffer, on_read_callback);
  printf("started reading %d\n", server->connected);
  net_send_frames(server);
}