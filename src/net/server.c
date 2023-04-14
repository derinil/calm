#include "server.h"
#include "uv.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

void on_new_connection(uv_stream_t *server, int status);

struct NetServer *net_setup_server() {
  int err;
  struct NetServer *s;
  struct sockaddr_in addr;
  s = malloc(sizeof(*s));
  if (!s)
    return NULL;
  memset(s, 0, sizeof(*s));
  s->loop = uv_default_loop();
  if (!s->loop)
    return NULL;
  s->tcp_server = malloc(sizeof(*s->tcp_server));
  if (!s->tcp_server)
    return NULL;
  uv_tcp_init(s->loop, s->tcp_server);
  uv_ip4_addr("0.0.0.0", CALM_PORT, &addr);
  err = uv_tcp_bind(s->tcp_server, (const struct sockaddr *)&addr, 0);
  if (err)
    return NULL;
  return s;
}

int net_destroy_server(struct NetServer *s) { return uv_loop_close(s->loop); }

int net_start_server(struct NetServer *server) {
  int err;
  err = uv_run(server->loop, UV_RUN_DEFAULT);
  if (err)
    return err;
  err = uv_listen((uv_stream_t *)server->tcp_server, 128, on_new_connection);
  return err;
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = (char*)malloc(suggested_size);
    buf->len = suggested_size;
}

void echo_write(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }
    free(req);
}

void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
            uv_close((uv_handle_t*) client, NULL);
        }
    } else if (nread > 0) {
        uv_write_t *req = (uv_write_t *) malloc(sizeof(uv_write_t));
        uv_buf_t wrbuf = uv_buf_init(buf->base, nread);
        uv_write(req, client, &wrbuf, 1, echo_write);
    }

    if (buf->base) {
        free(buf->base);
    }
}

void on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        return;
    }

    uv_tcp_t *client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(server->loop, client);
    if (uv_accept(server, (uv_stream_t*) client) == 0) {
        uv_read_start((uv_stream_t*)client, alloc_buffer, echo_read);
    } else {
        uv_close((uv_handle_t*) client, NULL);
    }
}