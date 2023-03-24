#ifndef NET_CLIENT_H_
#define NET_CLIENT_H_

#include <stdint.h>
#include "enet/enet.h"

#define NUM_PEERS 1
#define NUM_CHANNELS 2
#define CALM_PORT 58912

struct NetClient {
    ENetHost *client;
    ENetPeer *server;
};

struct NetClient *setup_client();
int connect_client(struct NetClient *rcv, const char *ip);
int destroy_client(struct NetClient *rcv);

#endif
