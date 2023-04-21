#ifndef NET_CLIENT_H_
#define NET_CLIENT_H_

#include "../data/stack.h"
#include "uv.h"
#include "read_state.h"
#include <stdint.h>

#define CALM_PORT 58912

struct NetClient {
  uv_loop_t *loop;
  // to be used when waiting for the connection etc.
  uv_cond_t cond;
  uv_mutex_t mutex;
  uv_tcp_t *tcp_socket;
  uv_stream_t *tcp_stream;
  uv_idle_t idle;
  struct DStack *ctrl_stack;
  struct DStack *frame_stack;
  struct ReadState *read_state;
};

struct NetClient *setup_client(struct DStack *frame_stack,struct DStack *ctrl_stack);
int connect_client(struct NetClient *rcv, const char *ip);
int destroy_client(struct NetClient *rcv);

#endif
