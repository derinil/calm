#include "server.h"
#include "capture/capture.h"
#include "common.h"
#include "data/stack.h"
#include "decode/decode.h"
#include "gui/dummy_gui.h"
#include "gui/server_gui.h"
#include "net/server.h"
#include "uv.h"
#include <stdio.h>
#include <stdlib.h>

static struct Server *g_server;

void server_decompressed_frame_callback(struct DFrame *frame) {
  dstack_push(g_server->decompressed_stack, frame, 1);
  void_cframe_releaser(frame->ctx);
}

void frame_callback(struct CFrame *frame) {
  retain_cframe(frame);
  retain_cframe(frame);
  dstack_push(g_server->compressed_stack, frame, 1);
  // Lower priority on the server
  decode_frame(g_server->decoder, frame);
  // release_cframe(frame);
}

void capture_thread(void *vargs) {
  int err;
  struct ThreadArgs *args = (struct ThreadArgs *)vargs;
  err = start_capture(g_server->capturer);
  if (err)
    goto exit;
  getchar();
exit:
  args->ret = err;
}

void server_net_thread(void *vargs) {
  int err;
  struct ThreadArgs *args = (struct ThreadArgs *)vargs;
  err = net_start_server(g_server->net_server);
  if (err)
    goto exit;
exit:
  args->ret = err;
}

int start_server() {
  int err;
  struct Server *server;
  struct Decoder *decoder;
  struct Capturer *capturer;
  struct NetServer *net_server;
  struct DStack *compressed_stack;
  struct DStack *decompressed_stack;
  struct ThreadArgs net_ret = {0}, cap_ret = {0};

  server = malloc(sizeof(*server));
  if (!server)
    return 1;
  memset(server, 0, sizeof(*server));

  capturer = setup_capturer(frame_callback);
  if (!capturer)
    return 2;

  compressed_stack = create_dstack(void_cframe_releaser);
  if (!compressed_stack)
    return 3;

  decompressed_stack = create_dstack(void_dframe_releaser);
  if (!decompressed_stack)
    return 4;

  net_server = net_setup_server(compressed_stack);
  if (!net_server)
    return 5;

  decoder = setup_decoder(server_decompressed_frame_callback);
  if (!decoder)
    return 6;

  server->decoder = decoder;
  server->capturer = capturer;
  server->net_server = net_server;
  server->compressed_stack = compressed_stack;
  server->decompressed_stack = decompressed_stack;

  g_server = server;

  uv_thread_create(&server->net_thread, server_net_thread, (void *)&net_ret);
  uv_thread_create(&server->net_thread, capture_thread, (void *)&net_ret);

#if 0
  err = handle_server_gui(server->decompressed_stack);
  if (err)
    return err;
#else
  run_dummy_gui(server->decompressed_stack);
#endif

  uv_thread_join(&server->net_thread);
  uv_thread_join(&server->capture_thread);

  if (net_ret.ret)
    printf("net thread failed with code %d\n", net_ret.ret);
  if (cap_ret.ret)
    printf("capture thread failed with code %d\n", cap_ret.ret);

  err = net_destroy_server(server->net_server);
  if (err)
    return err;
  err = stop_capture(server->capturer);
  if (err)
    return err;

  return 0;
}
