#include "client.h"
#include "enet/enet.h"
#include <stdlib.h>

struct NetClient *setup_client()
{
    int err;
    ENetHost *client;
    struct NetClient *c;
    err = enet_initialize();
    if (err)
        return NULL;
    c = malloc(sizeof(*c));
    if (!c)
        return NULL;
    client = enet_host_create(NULL, NUM_PEERS, NUM_CHANNELS, 0, 0);
    if (!client)
        return NULL;
    c->client = client;
    return c;
}

int destroy_client(struct NetClient *c)
{
    enet_host_destroy(c->client);
    enet_deinitialize();
    return 0;
}

int connect_client(struct NetClient *c, const char *ip)
{
    ENetPeer *peer;
    ENetEvent event;
    ENetAddress address;
    enet_address_set_host_ip(&address, ip);
    address.port = CALM_PORT;
    peer = enet_host_connect(c->client, &address, NUM_CHANNELS, 0);
    if (!peer)
        return 1;
    if (enet_host_service(c->client, &event, 5000) < 1 || event.type != ENET_EVENT_TYPE_CONNECT)
    {
        enet_peer_reset(peer);
        return 2;
    }
    c->server = peer;
    return 0;
}
