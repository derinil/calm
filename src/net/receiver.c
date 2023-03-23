#include "receiver.h"
#include <stdio.h>
#include "enet/enet.h"
#include <stdlib.h>

struct Receiver *setup_receiver()
{
    int err;
    ENetHost *client;
    struct Receiver *rcv;

    err = enet_initialize();
    if (err)
        return NULL;
    rcv = malloc(sizeof(*rcv));
    if (!rcv)
        return NULL;
    client = enet_host_create(NULL, 1, 2, 0, 0);
    if (client == NULL)
        return NULL;
    rcv->client = client;

    return rcv;
}

int destroy_receiver(struct Receiver *rcv)
{
    enet_host_destroy(rcv->client);
    enet_deinitialize();
    return 0;
}
