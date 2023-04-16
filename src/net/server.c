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

// void net_send_frames_loop(void *vargs) {
//   uv_buf_t wrbuf;
//   uv_write_t *req;
//   uv_async_t async;
//   struct CFrame *frame;
//   struct SerializedBuffer *buf;
//   struct AsyncWriteRequest *awr;
//   struct NetServer *server = (struct NetServer *)vargs;

//   uv_async_init(server->loop, &async, async_callback);

//   // TODO: make connected atomic
//   while (server->connected) {
//     frame = (struct CFrame *)dstack_pop_block(server->stack);
//     buf = serialize_cframe(frame);
//     if (!buf)
//       continue;
//     // TODO: sometimes libuv blocks for a bit and if we release before
//     decoder
//     // finishes we get a -12909 or a -12707 or a -12704
//     release_cframe(frame);
//     awr = malloc(sizeof(*awr));
//     memset(awr, 0, sizeof(*awr));
//     req = malloc(sizeof(*req));
//     memset(req, 0, sizeof(*req));
//     req->data = server;
//     wrbuf = uv_buf_init((char *)buf->buffer, buf->length);
//     // we can free the buffer here without freeing the actual underlying char
//     // array
//     free(buf);

//     awr->buffer = &wrbuf;
//     awr->req = req;
//     awr->handle = (uv_stream_t *)server->tcp_client;
//     awr->s = frame->is_keyframe;

//     uv_async_init(server->loop, &async, async_callback);

//     async.data = awr;
//     // TODO: libuv will coalesce calls??????
//     // http://docs.libuv.org/en/v1.x/async.html
//     uv_async_send(&async);
//   }

//   uv_close((uv_handle_t *)&async, NULL);
// }

void net_send_frames_loop(uv_idle_t *handle) {
  uv_buf_t wrbuf;
  uv_write_t *req;
  struct CFrame *frame;
  struct SerializedBuffer *buf;
  struct AsyncWriteRequest *awr;
  struct NetServer *server = (struct NetServer *)handle->data;

  printf("looping\n");

  if (!server->connected)
    return;

  frame = (struct CFrame *)dstack_pop_block(server->stack);
  buf = serialize_cframe(frame);
  if (!buf)
    return;
  printf("sending\n");
  // TODO: sometimes libuv blocks for a bit and if we release before decoder
  // finishes we get a -12909 or a -12707 or a -12704
  release_cframe(frame);
  awr = malloc(sizeof(*awr));
  memset(awr, 0, sizeof(*awr));
  req = malloc(sizeof(*req));
  memset(req, 0, sizeof(*req));
  req->data = server;
  wrbuf = uv_buf_init((char *)buf->buffer, buf->length);
  // we can free the buffer here without freeing the actual underlying char
  // array
  free(buf);

  awr->buffer = &wrbuf;
  awr->req = req;
  awr->handle = (uv_stream_t *)server->tcp_client;
  awr->s = frame->is_keyframe;

  // uv_async_init(server->loop, &async, async_callback);

  uv_write(awr->req, awr->handle, awr->buffer, 1, on_write_callback);

  free(awr);

  // async.data = awr;
  // TODO: libuv will coalesce calls??????
  // http://docs.libuv.org/en/v1.x/async.html
  // uv_async_send(&async);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}

void on_write_callback(uv_write_t *req, int status) {
  struct NetServer *server = (struct NetServer *)req->data;
  if (status) {
    // printf("Write error %s\n", uv_strerror(status));
    uv_close((uv_handle_t *)server->tcp_client, on_close_callback);
  }
  printf("sent %lu\n", req->bufs->len);
  free(req);
}

void on_read_callback(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
  struct NetServer *server = (struct NetServer *)client->data;
  if (nread < 0) {
    if (nread != UV_EOF) {
      // printf("Read error %s\n", uv_err_name(nread));
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
  uv_idle_start(&server->idle, net_send_frames_loop);
  // uv_thread_create(&server->send_loop, net_send_frames_loop, server);
}

// TODO:
volatile int once = 0;

void async_callback(uv_async_t *async) {
  struct AsyncWriteRequest *awr = (struct AsyncWriteRequest *)async->data;
  struct NetServer *server = (struct NetServer *)awr->handle->data;

  if (once != 0)
    return;
  if (!server->connected)
    return;

  // TODO: use queue

  printf("got async %d\n", awr->s);
  // TODO: we crash if conn closes right here
  uv_write(awr->req, awr->handle, awr->buffer, 1, on_write_callback);
  printf("wrote %lu\n", awr->buffer->len);
  free(awr);
  // once = 1;
}