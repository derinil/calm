#include "client.h"
#include "uv.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

static struct NetClient *g_net_client;

void on_connect(uv_connect_t *connection, int status);

struct NetClient *setup_client() {
  int err;
  struct NetClient *c;
  c = malloc(sizeof(*c));
  if (!c)
    return NULL;
  memset(c, 0, sizeof(*c));
  c->loop = uv_default_loop();
  uv_cond_init(&c->cond);
  uv_mutex_init(&c->mutex);
  g_net_client = c;
  return c;
}

int destroy_client(struct NetClient *c) {
  uv_loop_close(c->loop);
  free(c->tcp_socket);
  free(c);
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
  uv_tcp_connect(conn_req, c->tcp_socket, (const struct sockaddr *)&dest,
                 on_connect);
  return uv_run(c->loop, UV_RUN_DEFAULT);
}

static void alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
  *buf = uv_buf_init(malloc(size), size);
}

void on_close(uv_handle_t *handle) { printf("closed."); }

void on_write(uv_write_t *req, int status) {
  if (status) {
    printf("uv_write error %d\n", status);
    return;
  }
  printf("wrote.\n");
  free(req);
}

void on_read(uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf) {
  printf("on_read. %p\n", tcp);
  if (nread >= 0) {
    // printf("read: %s\n", tcp->data);
    printf("read: %s\n", buf->base);
  } else {
    // we got an EOF
    uv_close((uv_handle_t *)tcp, on_close);
  }

  // cargo-culted
  free(buf->base);
}

void write_stream(uv_stream_t *stream, char *data, int len2) {
  uv_buf_t buffer[] = {{.base = data, .len = len2}};
  uv_write_t *req = malloc(sizeof(uv_write_t));
  uv_write(req, stream, buffer, 1, on_write);
}

void on_connect(uv_connect_t *connection_req, int status) {
  if (status < 0)
    pthread_exit(&status);

#if 1
  printf("connected.\n");
#endif

  g_net_client->tcp_stream = connection_req->handle;
  free(connection_req);
  write_stream(g_net_client->tcp_stream, "echo  world!", 12);
  uv_read_start(g_net_client->tcp_stream, alloc_cb, on_read);
}
