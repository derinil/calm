#ifndef SERVER_H_
#define SERVER_H_

#include <pthread.h>
#include <stdint.h>

struct Server
{
    pthread_t net_thread;
    pthread_t capture_thread;
};

struct Frame
{
    uint8_t data;
    size_t length;
};

int start_server();

#endif
