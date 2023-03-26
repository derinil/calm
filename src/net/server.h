#ifndef NET_SERVER_H_
#define NET_SERVER_H_

#include <stdint.h>
#include "enet/enet.h"

// TODO: we can increase this for the server later on to allow multiple clients
#define NUM_PEERS 1
#define NUM_CHANNELS 2
#define CALM_PORT 58912

typedef void (*server_callback)(ENetEvent *event);

struct NetServer
{
    ENetHost *server;
    ENetPeer *client;
    server_callback connected_callback;
    server_callback disconnected_callback;
};

struct NetServer *setup_server();
int listen_connections(ENetHost *server);
int send_reliable(struct NetServer *s, uint8_t *data, size_t len);
void set_connected_callback(struct NetServer *s, server_callback callback);
void set_disconnected_callback(struct NetServer *s, server_callback callback);
int destroy_server(struct NetServer *s);

#endif
