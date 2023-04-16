#ifndef NET_CLIENT_H_
#define NET_CLIENT_H_

#include "../data/stack.h"
#include "uv.h"
#include <stdint.h>

#define CALM_PORT 58912

// Statefully read a CFrame
// NOTE: https://groups.google.com/g/libuv/c/fRNQV_QGgaA
struct ClientReadState {
  // 0 -> reading the full packet length
  // 1 -> allocated the full packet length, waiting for frame
  // 2 -> reading the frame itself
  // 3 -> allocated the full packet length, fill it up
  int state;
  uint64_t buf_len;
  uint8_t *buf_len_buffer;
  uint64_t buf_len_off;
  uint8_t *buffer;
  uint64_t buf_off;
};

struct NetClient {
  uv_loop_t *loop;
  // to be used when waiting for the connection etc.
  uv_cond_t cond;
  uv_mutex_t mutex;
  uv_tcp_t *tcp_socket;
  uv_stream_t *tcp_stream;
  struct ClientReadState *read_state;
  struct DStack *stack;
};

struct NetClient *setup_client(struct DStack *stack);
int connect_client(struct NetClient *rcv, const char *ip);
int destroy_client(struct NetClient *rcv);

#endif
