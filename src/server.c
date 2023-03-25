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
struct Server *g_server;

void send_frame_callback(uv_work_t *req)
{
    struct CFrame *frame = (struct CFrame *)req->data;
    if (!frame)
        printf("null frame!!\n");
    printf("1\n");
    send_reliable(g_server->net_server, frame->data, frame->length);
    printf("4\n");
}

void frame_callback(struct CFrame *frame)
{
    if (!frame)
        printf("null frame!!\n");
    uv_work_t *req;
    printf("received compressed frame with length %lu\n", frame->length);
    req = malloc(sizeof(*req));
    if (!req)
        return;
    req->data = (void *)frame;
    printf("2\n");
    uv_queue_work(loop, req, send_frame_callback, NULL);
    printf("3\n");
}

void *capture_thread(void *args)
{
    int err;
    struct Capturer *capturer;
    capturer = setup_capturer(frame_callback);
    if (!capturer)
        printf("failed to setup capturer\n");
    // TODO:
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
    // TODO: race condition :DD
    g_server->net_server = s;
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
    g_server = malloc(sizeof(*g_server));
    if (!g_server)
        return 1;

    loop = uv_default_loop();

    pthread_create(&(g_server->net_thread), NULL, server_net_thread, NULL);
    pthread_create(&(g_server->capture_thread), NULL, capture_thread, NULL);

    int err = handle_client_gui();
    if (err)
        return err;

    // TODO: clean up this horribleness
    void *thread_err;
    pthread_join(g_server->net_thread, &thread_err);
    if ((int *)(thread_err) && *(int *)(thread_err))
        return *(int *)(thread_err);
    pthread_join(g_server->capture_thread, &thread_err);
    if ((int *)(thread_err) && *(int *)(thread_err))
        return *(int *)(thread_err);
    return 0;
}
