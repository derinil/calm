#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include "net/server.h"
#include "gui/client_gui.h"
#include "capture/capture.h"
#include "uv.h"

uv_loop_t *loop;
uv_async_t server_itc;

void frame_callback(char *data, size_t length)
{
    uv_work_t *req;
            uv_queue_work(loop, &req[i], fib, NULL);

    printf("received compressed frame %d %d %d %d %d with length %lu\n", data[0], data[1], data[2], data[3], data[4], length);
}

void *capture_thread(void *args)
{
    int err;
    struct Capturer *capturer;
    capturer = setup(frame_callback);
    if (!capturer)
        printf("failed to setup capturer\n");
    if ((err = start_capture(capturer)))
        printf("failed to start capture: %d\n", err);
    getchar();
    if ((err = stop_capture(capturer)))
        printf("failed to stop capture: %d\n", err);
    return 0;
}

void *server_net_thread(void *args)
{
    int err;
    struct NetServer *s;

    s = setup_server();
    if (!s)
    {
        err = 1;
        goto exit;
    }
    err = listen_connections(s);
    if (err)
        goto exit;
    err = destroy_server(s);
    if (err)
        goto exit;
exit:
    printf("net_thread finished with code %d\n", err);
    return NULL;
}

int start_server()
{
    struct Server *server = malloc(sizeof(*server));
    if (!server)
        return 1;

    loop = uv_default_loop();

    pthread_create(&server->net_thread, NULL, server_net_thread, NULL);
    pthread_create(&server->capture_thread, NULL, capture_thread, NULL);

    int err = handle_client_gui();
    if (err)
        return err;

    // TODO: clean up this horribleness
    void *thread_err;
    pthread_join(server->net_thread, &thread_err);
    if ((int *)(thread_err) && *(int *)(thread_err))
        return *(int *)(thread_err);
    pthread_join(server->capture_thread, &thread_err);
    if ((int *)(thread_err) && *(int *)(thread_err))
        return *(int *)(thread_err);
    return 0;
}
