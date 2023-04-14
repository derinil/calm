#ifndef NET_SERVER_H_
#define NET_SERVER_H_

#include "uv.h"
#include <stddef.h>
#include <stdint.h>

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
