#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include "data/stack.h"
#include "net/server.h"
#include "gui/server_gui.h"
#include "capture/capture.h"
#include "decode/decode.h"
#include "uv.h"

struct Server *g_server;

void decompressed_frame_callback(struct DFrame *frame)
{
    printf("got decoded frame with length %lu\n", frame->data_length);
    dstack_push(g_server->stack, frame);
}

void decompress_frame(struct CFrame *frame)
{
    decode_frame(g_server->decoder, frame);
}

void frame_callback(struct CFrame *frame)
{
    if (!frame)
        printf("null frame!!\n");
    printf("received compressed frame with length %lu\n", frame->solid_frame_length);
    decompress_frame(frame);
}

void *capture_thread(void *args)
{
    int err;
    if ((err = start_capture(g_server->capturer)))
        printf("failed to start capture: %d\n", err);
    getchar();
    if ((err = stop_capture(g_server->capturer)))
        printf("failed to stop capture: %d\n", err);
    return NULL;
}

void *server_net_thread(void *args)
{
#if 1
    return NULL;
#endif
    int err;
    err = listen_connections(g_server->net_server->server);
    if (err)
        goto exit;
    err = destroy_server(g_server->net_server);
    if (err)
        goto exit;
exit:
    printf("net_thread finished with code %d\n", err);
    return NULL;
}

int start_server()
{
    struct Server *server;
    struct Capturer *capturer;
    struct Decoder *decoder;
    struct NetServer *net_server;
    struct DStack *stack;

    server = malloc(sizeof(*server));
    if (!server)
        return 1;
    memset(server, 0, sizeof(*server));

    capturer = setup_capturer(frame_callback);
    if (!capturer)
        return 2;

    net_server = setup_server();
    if (!net_server)
        return 3;

    stack = create_dstack();
    if (!stack)
        return 4;

    decoder = setup_decoder(decompressed_frame_callback);
    if (!decoder)
        return 5;

    server->stack = stack;
    server->decoder = decoder;
    server->capturer = capturer;
    server->net_server = net_server;

    g_server = server;

    pthread_create(&(g_server->net_thread), NULL, server_net_thread, NULL);
    pthread_create(&(g_server->capture_thread), NULL, capture_thread, NULL);

    int err = handle_server_gui(g_server->stack);
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
