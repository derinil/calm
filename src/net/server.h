#ifndef NET_SERVER_H_
#define NET_SERVER_H_

#include "uv.h"
#include <stddef.h>
#include <stdint.h>

// TODO: we can increase this for the server later on to allow multiple clients
#define NUM_PEERS 1
#define NUM_CHANNELS 2
#define CALM_PORT 58912

struct NetServer {
  uv_loop_t *loop;
  uv_tcp_t *tcp_server;
};

struct NetServer *net_setup_server();
int net_start_server(struct NetServer *server);
int send_reliable(struct NetServer *s, uint8_t *data, size_t len);
int net_destroy_server(struct NetServer *s);

#endif
