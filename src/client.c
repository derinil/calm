#include "gui/client_gui.h"
#include "client.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "net/receiver.h"

void *net_thread(void *args)
{
    int err;
    struct Receiver *rcv;

    rcv = setup_receiver();
    if (!rcv)
    {
        err = 1;
        goto exit;
    }
    err = destroy_receiver(rcv);
    if (err)
        goto exit;
exit:
    printf("net_thread finished with code %d\n", err);
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
