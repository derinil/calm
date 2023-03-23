#include "receiver.h"
#include <stdio.h>
#include "enet/enet.h"

int setup_receiver()
{
    int err = enet_initialize();
    if (err)
        return err;
    printf("hellol\n");
    return 0;
}

int destroy_receiver()
{
    enet_deinitialize();
    return 0;
}
