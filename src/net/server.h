#ifndef NET_SERVER_H_
#define NET_SERVER_H_

#include "../capture/capture.h"
#include "../data/stack.h"
#include "uv.h"
#include <stddef.h>
#include <stdint.h>

#define CALM_PORT 58912

struct NetServer {
  // TODO: make atomic or use lock
  int connected;
  uv_loop_t *loop;
  uv_tcp_t *tcp_client;
  uv_tcp_t *tcp_server;
  uv_idle_t idle;
  uv_mutex_t mutex;
  uv_thread_t send_loop;
  struct DStack *stack;
};

struct NetServer *net_setup_server(struct DStack *stack);
int net_start_server(struct NetServer *server);
int send_reliable(struct NetServer *server, uint8_t *data, size_t len);
int net_destroy_server(struct NetServer *server);

#endif
