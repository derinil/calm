#include "client.h"
#include "data/stack.h"
#include "gui/client_gui.h"
#include "net/client.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

struct ThreadArgs {
  void *args;
  int ret;
};

void *net_thread(void *vargs) {
  int err;
  struct ThreadArgs *args = (struct ThreadArgs *)vargs;
  err = connect_client((struct NetClient *)(args->args), "127.0.0.1");
  args->ret = err;
  return NULL;
}

int start_client() {
  int net_ret = 0;
  struct Client *client;
  struct DStack *stack;
  struct NetClient *net_client;

  client = malloc(sizeof(*client));
  if (!client)
    return 1;
  memset(client, 0, sizeof(*client));

  stack = create_dstack(NULL);
  if (!stack)
    return 2;

  net_client = setup_client(stack);
  if (!net_client)
    return 3;

  client->net_client = net_client;

  struct ThreadArgs net_args =
      (struct ThreadArgs){.args = net_client, .ret = 0};

  pthread_create(&client->net_thread, NULL, net_thread, (void *)&net_args);

  int err = handle_client_gui(stack);
  if (err)
    return err;

  pthread_join(client->net_thread, NULL);

  if (net_args.ret != 0)
    printf("net thread failed with code %d\n", net_args.ret);

  err = destroy_client(client->net_client);
  if (err)
    return err;

  return 0;
}
