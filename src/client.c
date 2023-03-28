#include "gui/client_gui.h"
#include "client.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "net/client.h"
#include <strings.h>

void *net_thread(void *args)
{
    int err;
    struct NetClient *c;

    c = setup_client();
    if (!c)
    {
        err = 1;
        goto exit;
    }
    err = connect_client(c, "127.0.0.1");
    if (err)
        goto exit;
    getchar();
    err = destroy_client(c);
    if (err)
        goto exit;
exit:
    printf("net_thread finished with code %d\n", err);
    return NULL;
}

int start_client()
{
    struct Client *client = malloc(sizeof(*client));
    if (!client)
        return 1;
    memset(client, 0, sizeof(*client));

    pthread_create(&client->net_thread, NULL, net_thread, NULL);

    int err = handle_client_gui(NULL);
    if (err)
        return err;

    void *net_err;
    pthread_join(client->net_thread, &net_err);
    if ((int *)(net_err) && *(int *)(net_err))
        return *(int *)(net_err);
    return 0;
}
