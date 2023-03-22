#ifndef CLIENT_H_
#define CLIENT_H_

#include <pthread.h>

struct client
{
    pthread_t *gui_thread;
    pthread_t *net_thread;
};

int start_client();

#endif
