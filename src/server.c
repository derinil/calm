#include "server.h"
#include "capture/capture.h"
#include "data/stack.h"
#include "decode/decode.h"
#include "gui/dummy_gui.h"
#include "gui/server_gui.h"
#include "net/server.h"
#include "uv.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static struct Server *g_server;

struct ThreadArgs {
  void *args;
  int *ret;
};

void void_dframe_releaser(void *vdf) { release_dframe((struct DFrame *)vdf); }

void decompressed_frame_callback(struct DFrame *frame) {
  dstack_push(g_server->decompressed_stack, frame, 1);
}

void frame_callback(struct CFrame *frame) {
  decode_frame(g_server->decoder, frame);
  dstack_push(g_server->compressed_stack, frame, 1);
}

void *capture_thread(void *vargs) {
  int err;
  struct ThreadArgs *args = (struct ThreadArgs *)vargs;
  err = start_capture(g_server->capturer);
  if (err)
    goto exit;
  getchar();
exit:
  *args->ret = err;
  return NULL;
}

void *server_net_thread(void *vargs) {
  int err;
  struct ThreadArgs *args = (struct ThreadArgs *)vargs;
  err = net_start_server(g_server->net_server);
  if (err)
    goto exit;
exit:
  *args->ret = err;
  return NULL;
}

int start_server() {
  int net_ret = 0, cap_ret = 0, err = 0;
  struct Server *server;
  struct Decoder *decoder;
  struct Capturer *capturer;
  struct NetServer *net_server;
  struct DStack *compressed_stack;
  struct DStack *decompressed_stack;

  server = malloc(sizeof(*server));
  if (!server)
    return 1;
  memset(server, 0, sizeof(*server));

  capturer = setup_capturer(frame_callback);
  if (!capturer)
    return 2;

  compressed_stack = create_dstack(NULL);
  if (!compressed_stack)
    return 3;

  decompressed_stack = create_dstack(void_dframe_releaser);
  if (!decompressed_stack)
    return 4;

  net_server = net_setup_server(compressed_stack);
  if (!net_server)
    return 5;

  decoder = setup_decoder(decompressed_frame_callback);
  if (!decoder)
    return 6;

  server->decoder = decoder;
  server->capturer = capturer;
  server->net_server = net_server;
  server->compressed_stack = compressed_stack;
  server->decompressed_stack = decompressed_stack;

  g_server = server;

  pthread_create(&(server->net_thread), NULL, server_net_thread,
                 (void *)&net_ret);
  pthread_create(&(server->capture_thread), NULL, capture_thread,
                 (void *)&cap_ret);

#if 1
  err = handle_server_gui(server->decompressed_stack);
  if (err)
    return err;
#else
  run_dummy_gui(stack);
#endif

  pthread_join(server->net_thread, NULL);
  if (net_ret)
    printf("net thread failed with code %d\n", net_ret);
  pthread_join(server->capture_thread, NULL);
  if (cap_ret)
    printf("capture thread failed with code %d\n", cap_ret);

  err = net_destroy_server(server->net_server);
  if (err)
    return err;

  err = stop_capture(server->capturer);
  if (err)
    return err;

  return 0;
}
