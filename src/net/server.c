#include "server.h"
#include "enet/enet.h"
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

struct NetServer *setup_server()
{
    int err;
    ENetHost *server;
    struct NetServer *s;
    ENetAddress address;
    err = enet_initialize();
    if (err)
        return NULL;
    s = malloc(sizeof(*s));
    if (!s)
        return NULL;
    memset(s, 0, sizeof(*s));
    address.host = ENET_HOST_ANY;
    address.port = CALM_PORT;
    server = enet_host_create(&address, NUM_PEERS, NUM_CHANNELS, 0, 0);
    if (!server)
        return NULL;
    // TODO: consider enet_host_compress_with_range_coder
    s->server = server;
    return s;
}

int destroy_server(struct NetServer *s)
{
    enet_host_destroy(s->server);
    enet_deinitialize();
    return 0;
}

void set_connected_callback(struct NetServer *s, server_callback callback)
{
    s->connected_callback = callback;
}

void set_disconnected_callback(struct NetServer *s, server_callback callback)
{
    s->disconnected_callback = callback;
}

int send_reliable(struct NetServer *s, uint8_t *data, size_t len)
{
    int err = 1;
    ENetPacket *packet;
    if (!s->server->peerCount)
        return 0;
    packet = enet_packet_create(data, len, ENET_PACKET_FLAG_RELIABLE | ENET_PACKET_FLAG_NO_ALLOCATE);
    if (!packet)
        goto fail;
    err = enet_peer_send(s->server->peers, 0, packet);
    if (err)
        goto fail;
    return 0;
fail:
    enet_packet_destroy(packet);
    return err;
}

#define ENET_DEBUG

int listen_connections(ENetHost *server)
{
    ENetEvent event;

    // TODO: leaving this at 1 second for now, might want to lower the interval
    while (enet_host_service(server, &event, 1000) > -1)
    {
        // printf("lol %d\n", event.type);
        printf("lol\n");
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            printf("yo\n");
            printf("A new client connected from %x:%u.\n",
                   event.peer->address.host,
                   event.peer->address.port);
            // TODO: we don't really need this when we only allow 1 peer
            event.peer->data = "Client information";
            // TODO: race condition
            // s->client = event.peer;
            // if (s->connected_callback)
            //     s->connected_callback(&event);
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            printf("packet received\n");
            enet_packet_destroy(event.packet);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            // s->disconnected_callback(&event);
            event.peer->data = NULL;
            break;
        default:
            break;
        }

        enet_host_flush(server);
    }

    return 0;
}
