#include "server.h"
#include "capture/capture.h"
#include "decode/decode.h"
#include "gui/server_gui.h"
#include "net/server.h"
#include "uv.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "data/stack.h"
#include "gui/dummy_gui.h"

struct Server *g_server;

void decompressed_frame_callback(struct DFrame *frame) {
  dstack_push(g_server->stack, frame);
}

void frame_callback(struct CFrame *frame) {
  decode_frame(g_server->decoder, frame);
  release_cframe(frame);
}

void *capture_thread(void *args) {
  int err;
  if ((err = start_capture(g_server->capturer)))
    printf("failed to start capture: %d\n", err);
  getchar();
  if ((err = stop_capture(g_server->capturer)))
    printf("failed to stop capture: %d\n", err);
  return NULL;
}

void *server_net_thread(void *args) {
  int err;
  err = net_start_server(g_server->net_server);
  if (err)
    goto exit;
  err = net_destroy_server(g_server->net_server);
  if (err)
    goto exit;
exit:
  printf("net_thread finished with code %d\n", err);
  return NULL;
}

void void_dframe_releaser(void *vdf) { release_dframe((struct DFrame *)vdf); }

int start_server() {
  struct DStack *stack;
  struct Server *server;
  struct Decoder *decoder;
  struct Capturer *capturer;
  struct NetServer *net_server;

  server = malloc(sizeof(*server));
  if (!server)
    return 1;
  memset(server, 0, sizeof(*server));

  capturer = setup_capturer(frame_callback);
  if (!capturer)
    return 2;

  net_server = net_setup_server();
  if (!net_server)
    return 3;

  stack = create_dstack(void_dframe_releaser);
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

#if 1
    int err = handle_server_gui(g_server->stack);
    if (err)
        return err;
#else
  run_dummy_gui(stack);
#endif

  // What a pleasant block of code :)
  void *thread_err;
  pthread_join(g_server->net_thread, &thread_err);
  if ((int *)(thread_err) && *(int *)(thread_err))
    return *(int *)(thread_err);
  pthread_join(g_server->capture_thread, &thread_err);
  if ((int *)(thread_err) && *(int *)(thread_err))
    return *(int *)(thread_err);
  return 0;
}
