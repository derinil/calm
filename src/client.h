#ifndef CLIENT_H_
#define CLIENT_H_

#include "decode/decode.h"
#include "net/client.h"
#include "data/stack.h"
#include "uv.h"

struct Client {
  uv_thread_t net_thread;
  uv_thread_t decode_thread;
  struct Decoder *decoder;
  struct NetClient *net_client;
  struct DStack *compressed_stack;
  struct DStack *decompressed_stack;
};

int start_client();

#endif
