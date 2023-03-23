#ifndef RECEIVER_H_
#define RECEIVER_H_

#include <stdint.h>
#include "enet/enet.h"

struct Receiver {
    ENetHost *client;
};

struct Receiver *setup_receiver();
int destroy_receiver(struct Receiver *rcv);

#endif
