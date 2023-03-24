#ifndef SERVER_H_
#define SERVER_H_

#include <pthread.h>

struct Server
{
    pthread_t net_thread;
    pthread_t capture_thread;
};

int start_server();

#endif
