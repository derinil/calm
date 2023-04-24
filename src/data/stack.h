#ifndef DATA_STACK_H_
#define DATA_STACK_H_

#include "uv.h"
#include <stddef.h>

#define MAX_DS_LEN 2048

// Internal representation of a element,
// we still accept and return (void *)
struct DSElement {
  void *actual;
  int read_count;
  int remove_at;
  int pop_count;
};

struct Buffer {
  void **elements;
  size_t length;
};

typedef void (*FreeElement)(void *element);

// Stack
// Designed for 1 thread writing, 1 thread reading
// Reading thread should be able to wait for an update
// Having a mutex at this point will not be too much of a bottleneck
struct DStack {
  size_t length;
  size_t read_curr;
  size_t write_curr;
  FreeElement freer;
  uv_mutex_t mutex;
  uv_cond_t cond;
  struct DSElement *elements[MAX_DS_LEN];
};

struct DStack *create_dstack(FreeElement freer);
// remove_at should be <= 1 to be removed off the stack at first read
void dstack_push(struct DStack *ds, void *element, int remove_at);
// Returns NULL if no element is found
void *dstack_pop_nonblock(struct DStack *ds);
// Blocks using a condvar
void *dstack_pop_block(struct DStack *ds);
// Empty the stack, useful for control instructions
struct Buffer dstack_pop_all(struct DStack *ds);

// TODO: destroy_dstack

#endif
