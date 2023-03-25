#include "server.h"
#include "enet/enet.h"
#include <stdlib.h>
#include <stdio.h>

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
    printf("5\n");
    if (!s)
        printf("null server\n");
    // TODO: how is it getting past this?
    if (!s->client)
        return 0;
    packet = enet_packet_create(data, len, ENET_PACKET_FLAG_RELIABLE | ENET_PACKET_FLAG_NO_ALLOCATE);
    if (!packet)
        goto fail;
    printf("6\n");
    err = enet_peer_send(s->client, 0, packet);
    if (err)
        goto fail;
    return 0;
fail:
printf("7\n");
    enet_packet_destroy(packet);
    printf("8\n");
    return err;
}

int listen_connections(struct NetServer *s)
{
    ENetEvent event;

    // TODO: leaving this at 1 second for now, might want to lower the interval
    while (enet_host_service(s->server, &event, 1000) > -1)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            printf("A new client connected from %x:%u.\n",
                   event.peer->address.host,
                   event.peer->address.port);
            // TODO: we don't really need this when we only allow 1 peer
            // event.peer->data = "Client information";
            s->client = event.peer;
            s->connected_callback(&event);
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            printf("packet received\n");
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            s->disconnected_callback(&event);
            event.peer->data = NULL;
            break;
        default:
            break;
        }

        enet_packet_destroy(event.packet);
    }

    return 0;
}
