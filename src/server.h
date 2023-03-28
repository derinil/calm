#ifndef SERVER_H_
#define SERVER_H_

#include <pthread.h>
#include <stdint.h>
#include "data/stack.h"
#include "net/server.h"
#include "capture/capture.h"
#include "decode/decode.h"

struct Server
{
    pthread_t net_thread;
    pthread_t capture_thread;
    struct DStack *stack;
    struct Decoder *decoder;
    struct Capturer *capturer;
    struct NetServer *net_server;
};

int start_server();

#endif
