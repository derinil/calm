#ifndef CLIENT_H_
#define CLIENT_H_

#include "net/client.h"
#include <pthread.h>

struct Client
{
    pthread_t net_thread;
    struct NetClient *net_client;
};

int start_client();

#endif
