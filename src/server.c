#include "server.h"
#include "capture/capture.h"
#include "common.h"
#include "cyborg/control.h"
#include "data/stack.h"
#include "decode/decode.h"
#include "gui/dummy_gui.h"
#include "gui/server_gui.h"
#include "net/server.h"
#include "uv.h"
#include <stdio.h>
#include <stdlib.h>

static struct Server *g_server;

#define MARSHAL_SERVERSIDE 0
#define DECODE_SERVERSIDE 0
#define RUN_SERVER_GUI 0

static void server_decompressed_frame_callback(struct DFrame *dframe) {
  dstack_push(g_server->decompressed_stack, dframe, 1);
  release_cloned_cframe((struct CFrame *)dframe->ctx);
}

static void frame_callback(struct CFrame *frame) {
#if MARSHAL_SERVERSIDE
  // This crashes once stack tries to free old ones
  struct SerializedCFrame *ser = serialize_cframe(frame);
  release_cframe(&frame);
  frame = unmarshal_cframe(ser->buffer, ser->length);
#endif
#if DECODE_SERVERSIDE
  struct CFrame *clone = clone_cframe(frame);
  decode_frame(g_server->decoder, clone);
#endif
#if 0
  if (frame->parameter_sets_count) {
    for (uint64_t j = 0; j < frame->parameter_sets_count; j++) {
      for (uint64_t i = 0; i < frame->parameter_sets_lengths[j]; i++)
        printf("%x-", frame->parameter_sets[j][i]);
      printf("\n");
    }
  }
#endif
  dstack_push(g_server->compressed_stack, frame, 1);
}

void control_thread(void *vargs) {
  int err;
  struct Control *ctrl;
  struct Buffer ctrls;
  struct ThreadArgs *args = (struct ThreadArgs *)vargs;
  struct DStack *ctrl_stack = args->args;
  while (1) {
    ctrls = dstack_pop_all(ctrl_stack);
    for (uint32_t x = 0; x < ctrls.length; x++) {
      ctrl = ctrls.elements[x];
      inject_control(ctrl);
    }
  }
  args->ret = err;
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
  struct DStack *control_stack;
  struct DStack *compressed_stack;
  struct DStack *decompressed_stack;
  struct ThreadArgs net_ret = {0}, cap_ret = {0}, ctrl_ret = {0};

  server = malloc(sizeof(*server));
  if (!server)
    return 1;
  memset(server, 0, sizeof(*server));

  capturer = setup_capturer(frame_callback);
  if (!capturer)
    return 2;

  compressed_stack = create_dstack(void_release_cframe);
  if (!compressed_stack)
    return 3;

  decompressed_stack = create_dstack(void_release_dframe);
  if (!decompressed_stack)
    return 4;

  control_stack = create_dstack(void_release_control);
  if (!control_stack)
    return 7;

  net_server = net_setup_server(compressed_stack, control_stack);
  if (!net_server)
    return 5;

  decoder = setup_decoder(server_decompressed_frame_callback);
  if (!decoder)
    return 6;

  server->decoder = decoder;
  server->capturer = capturer;
  server->net_server = net_server;
  server->control_stack = control_stack;
  server->compressed_stack = compressed_stack;
  server->decompressed_stack = decompressed_stack;

  g_server = server;

  uv_thread_create(&server->net_thread, server_net_thread, (void *)&net_ret);
  uv_thread_create(&server->net_thread, capture_thread, (void *)&net_ret);
  ctrl_ret.args = control_stack;
  uv_thread_create(&server->control_thread, control_thread, (void *)&ctrl_ret);

#if RUN_SERVER_GUI
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
