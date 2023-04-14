#include "server.h"
#include "../capture/capture.h"
#include "../data/stack.h"
#include "uv.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

void on_new_connection(uv_stream_t *server, int status);

static struct NetServer *g_net_server;

struct NetServer *net_setup_server(struct DStack *stack) {
  int err;
  struct NetServer *s;
  struct sockaddr_in addr;
  s = malloc(sizeof(*s));
  if (!s)
    return NULL;
  memset(s, 0, sizeof(*s));
  g_net_server = s;
  s->loop = uv_default_loop();
  if (!s->loop)
    return NULL;
  s->tcp_server = malloc(sizeof(*s->tcp_server));
  if (!s->tcp_server)
    return NULL;
  s->stack = stack;
  uv_tcp_init(s->loop, s->tcp_server);
  uv_ip4_addr("0.0.0.0", CALM_PORT, &addr);
  err = uv_tcp_bind(s->tcp_server, (const struct sockaddr *)&addr, 0);
  if (err)
    return NULL;
  return s;
}

int net_destroy_server(struct NetServer *s) {
  int err;
  err = uv_loop_close(s->loop);
  free(s->tcp_server);
  free(s);
  return err;
}

int net_start_server(struct NetServer *server) {
  int err;
  err = uv_listen((uv_stream_t *)server->tcp_server, 128, on_new_connection);
  if (err)
    return err;
  err = uv_run(server->loop, UV_RUN_DEFAULT);
  return err;
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  buf->base = (char *)malloc(suggested_size);
  buf->len = suggested_size;
}

void on_write_callback(uv_write_t *req, int status) {
  if (status) {
    fprintf(stderr, "Write error %s\n", uv_strerror(status));
  }
  free(req);
}

void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
  if (nread < 0) {
    if (nread != UV_EOF) {
      fprintf(stderr, "Read error %s\n", uv_err_name(nread));
      uv_close((uv_handle_t *)client, NULL);
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
  printf("closed conn\n");
  free(handle);
}

void on_new_connection(uv_stream_t *server, int status) {
  int err;
  uv_buf_t wrbuf;
  uv_write_t *req;
  struct CFrame *cframe;
  struct SerializedBuffer *buf;

  if (status < 0) {
    fprintf(stderr, "New connection error %s\n", uv_strerror(status));
    return;
  }

  uv_tcp_t *client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
  printf("client\n");
  uv_tcp_init(g_net_server->loop, client);
  printf("init\n");
  err = uv_accept(server, (uv_stream_t *)client);
  printf("accepted\n");
  if (err) {
    uv_close((uv_handle_t *)client, on_close_callback);
    return;
  }
  uv_read_start((uv_stream_t *)client, alloc_buffer, echo_read);
  printf("started reading\n");
  // TODO: not sure if blocking a callback is ok
  while (1) {
    cframe = (struct CFrame *)dstack_pop_block(g_net_server->stack);
    // This should never happen :) clueless
    if (!cframe)
      continue;
    printf("sending buffer\n");
    buf = serialize_cframe(cframe);
    if (!buf)
      continue;
    req = (uv_write_t *)malloc(sizeof(uv_write_t));
    wrbuf = uv_buf_init((char *)buf->buffer, buf->length);
    uv_write(req, (uv_stream_t *)client, &wrbuf, 1, on_write_callback);
    release_cframe(cframe);
    printf("sent buffer\n");
  }
}