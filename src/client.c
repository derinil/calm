#include "gui/client_gui.h"
#include "client.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *net_thread(void *args)
{
    pthread_exit(NULL);
    return NULL;
}

int start_client()
{
    struct client *client = malloc(sizeof(*client));
    if (!client)
        return 1;

    pthread_create(&client->net_thread, NULL, net_thread, NULL);

    int err = handle_client_gui();
    if (err)
        return err;

    void *net_err;
    pthread_join(client->net_thread, &net_err);
    if ((int *)(net_err) && *(int *)(net_err))
        return *(int *)(net_err);

    return 0;
}
