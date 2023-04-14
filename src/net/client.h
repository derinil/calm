#ifndef NET_CLIENT_H_
#define NET_CLIENT_H_

#include "uv.h"
#include <stdint.h>
#include "../data/stack.h"

#define CALM_PORT 58912

struct NetClient {
  uv_loop_t *loop;
  // to be used when waiting for the connection etc.
  uv_cond_t cond;
  uv_mutex_t mutex;
  uv_tcp_t *tcp_socket;
  uv_stream_t *tcp_stream;
  struct DStack *stack;
};

struct NetClient *setup_client(struct DStack *stack);
int connect_client(struct NetClient *rcv, const char *ip);
int destroy_client(struct NetClient *rcv);

#endif
