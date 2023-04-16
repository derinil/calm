#include "client.h"
#include "capture/capture.h"
#include "common.h"
#include "data/stack.h"
#include "decode/decode.h"
#include "gui/client_gui.h"
#include "net/client.h"
#include "uv.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

static struct Client *g_client;

// TODO: add a data field for callbacks, to both pass the stack and the cframe
// to free
void client_decompressed_frame_callback(struct DFrame *frame) {
  printf("got decoded frame\n");
#if 1
  struct DFrame *dframe = frame;
  FILE *f = fopen("test.ppm", "w+");
  fprintf(f, "P3\n%lu %lu\n255\n", dframe->width, dframe->height);
  for (size_t x = 0; x < dframe->data_length; x += 4) {
    fprintf(f, "%u %u %u\n", dframe->data[x + 2], dframe->data[x + 1],
            dframe->data[x]);
  }
  fflush(f);
  fclose(f);
  exit(0);
#endif
  dstack_push(g_client->decompressed_stack, frame, 1);
}

void decode_thread(void *vargs) {
  int err;
  struct CFrame *cframe;
  struct ThreadArgs *args = (struct ThreadArgs *)vargs;
  struct Client *client = (struct Client *)args->args;
  // TODO: mechanism to stop this
  while (1) {
    printf("popping\n");
    cframe = (struct CFrame *)dstack_pop_block(client->compressed_stack);
    printf("popped\n");
    decode_frame(client->decoder, cframe);
    // TODO: free(cframe);
  }
  args->ret = err;
}

void net_thread(void *vargs) {
  int err;
  struct ThreadArgs *args = (struct ThreadArgs *)vargs;
  err = connect_client((struct NetClient *)(args->args), "127.0.0.1");
  args->ret = err;
}

int start_client() {
  int net_ret = 0;
  struct Client *client;
  struct Decoder *decoder;
  struct NetClient *net_client;
  struct DStack *compressed_stack;
  struct DStack *decompressed_stack;

  client = malloc(sizeof(*client));
  if (!client)
    return 1;
  memset(client, 0, sizeof(*client));

  compressed_stack = create_dstack(void_cframe_releaser);
  if (!compressed_stack)
    return 2;

  decompressed_stack = create_dstack(void_dframe_releaser);
  if (!decompressed_stack)
    return 3;

  // TODO: rename to net_setup_client
  net_client = setup_client(compressed_stack);
  if (!net_client)
    return 4;

  decoder = setup_decoder(client_decompressed_frame_callback);
  if (!decoder)
    return 5;

  client->decoder = decoder;
  client->net_client = net_client;
  client->compressed_stack = compressed_stack;
  client->decompressed_stack = decompressed_stack;

  struct ThreadArgs net_args =
      (struct ThreadArgs){.args = net_client, .ret = 0};
  uv_thread_create(&client->net_thread, net_thread, (void *)&net_args);

  struct ThreadArgs decode_args = (struct ThreadArgs){.args = client, .ret = 0};
  uv_thread_create(&client->decode_thread, decode_thread, (void *)&decode_args);

  int err = handle_client_gui(decompressed_stack);
  if (err)
    return err;

  uv_thread_join(&client->net_thread);
  uv_thread_join(&client->decode_thread);

  if (net_args.ret != 0)
    printf("net thread failed with code %d\n", net_args.ret);

  if (decode_args.ret != 0)
    printf("decode thread failed with code %d\n", decode_args.ret);

  err = destroy_client(client->net_client);
  if (err)
    return err;

  return 0;
}
