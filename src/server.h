#ifndef SERVER_H_
#define SERVER_H_

#include <pthread.h>
#include <stdint.h>
#include "net/server.h"

struct Server
{
    pthread_t net_thread;
    pthread_t capture_thread;
    struct NetServer *net_server;
};

int start_server();

#endif
