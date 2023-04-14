#ifndef SERVER_H_
#define SERVER_H_

#include "capture/capture.h"
#include "data/stack.h"
#include "decode/decode.h"
#include "net/server.h"
#include <pthread.h>
#include <stdint.h>

struct Server {
  pthread_t net_thread;
  pthread_t capture_thread;
  struct Decoder *decoder;
  struct Capturer *capturer;
  struct NetServer *net_server;
  struct DStack *compressed_stack;
  struct DStack *decompressed_stack;
};

int start_server();

#endif
