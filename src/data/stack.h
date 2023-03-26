#ifndef DATA_STACK_H_
#define DATA_STACK_H_

#include <pthread.h>
#include <stddef.h>

#define MAX_DS_LEN 2048

// Stack
// Designed for 1 thread writing, 1 thread reading
// Reading thread should be able to wait for an update
// Having a mutex at this point will not be too much of a bottleneck
struct DStack {
  size_t write_curr;
  size_t read_curr;
  void *elements[MAX_DS_LEN];
  pthread_mutex_t lock;
  pthread_cond_t ready;
};

struct DStack *create_dstack();
void dstack_push(struct DStack *ds, void *element);
void *dstack_pop(struct DStack *ds);
int dstack_ready(struct DStack *ds);

#endif
