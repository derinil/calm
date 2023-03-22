#include "gui/client_gui.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *gui_thread()
{
    int err = start_client_gui();
    if (err)
        return err;
}

void *net_thread()
{
}

int start_client()
{
    struct client *client = malloc(sizeof(client));
    if (!client)
        return 1;

    getchar();

    return 0;
}
