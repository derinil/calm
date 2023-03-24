#ifndef CLIENT_H_
#define CLIENT_H_

#include <pthread.h>

struct Client
{
    pthread_t net_thread;
};

int start_client();

#endif
