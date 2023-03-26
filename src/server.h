#ifndef SERVER_H_
#define SERVER_H_

#include <pthread.h>
#include <stdint.h>
#include "data/stack.h"
#include "net/server.h"
#include "capture/capture.h"

struct Server
{
    pthread_t net_thread;
    pthread_t capture_thread;
    struct Capturer *capturer;
    struct NetServer *net_server;
    struct DStack *stack;
};

int start_server();

#endif
