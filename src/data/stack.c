#include "stack.h"
#include "../capture/capture.h"
#include "uv.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

struct DStack *create_dstack(FreeElement freer) {
  struct DStack *ds = calloc(1, sizeof(*ds));
  if (!ds)
    return NULL;
  if (uv_mutex_init(&ds->mutex))
    return NULL;
  if (uv_cond_init(&ds->cond))
    return NULL;
  ds->freer = freer;
  return ds;
}

void dstack_push(struct DStack *ds, void *element, int remove_at) {
  struct DSElement *el = NULL;
  uv_mutex_lock(&ds->mutex);
  if (ds->elements[ds->write_curr] && ds->freer != NULL) {
    ds->freer(ds->elements[ds->write_curr]->actual);
    free(ds->elements[ds->write_curr]);
    ds->elements[ds->write_curr] = NULL;
  }
  ds->elements[ds->write_curr] = calloc(1, sizeof(struct DSElement));
  ds->elements[ds->write_curr]->actual = element;
  ds->elements[ds->write_curr]->remove_at = remove_at;
  ds->write_curr++;
  ds->length++;
  if (ds->write_curr == MAX_DS_LEN)
    ds->write_curr = 0;
  uv_cond_signal(&ds->cond);
  uv_mutex_unlock(&ds->mutex);
}

void *dstack_pop_nonblock(struct DStack *ds) {
  void *el = NULL;
  struct DSElement *element = NULL;
  uv_mutex_lock(&ds->mutex);
  if (ds->length == 0)
    goto end;
  element = ds->elements[ds->read_curr];
  if (!element)
    goto end;
  el = element->actual;
  element->read_count++;
  if (element->remove_at > element->read_count)
    goto end;
  free(ds->elements[ds->read_curr]);
  ds->elements[ds->read_curr] = NULL;
  ds->read_curr++;
  ds->length--;
  if (ds->read_curr == MAX_DS_LEN)
    ds->read_curr = 0;
end:
  uv_mutex_unlock(&ds->mutex);
  return el;
}

void *dstack_pop_block(struct DStack *ds) {
  void *el = NULL;
  struct DSElement *element = NULL;
  uv_mutex_lock(&ds->mutex);
  // TODO: https://github.com/libuv/help/issues/134
  while (ds->length == 0)
    uv_cond_wait(&ds->cond, &ds->mutex);
  if (ds->length == 0)
    goto end;
  element = ds->elements[ds->read_curr];
  if (!element)
    goto end;
  el = element->actual;
  element->read_count++;
  if (element->remove_at > element->read_count)
    goto end;
  free(ds->elements[ds->read_curr]);
  ds->elements[ds->read_curr] = NULL;
  ds->read_curr++;
  ds->length--;
  if (ds->read_curr == MAX_DS_LEN)
    ds->read_curr = 0;
end:
  uv_mutex_unlock(&ds->mutex);
  return el;
}

struct Buffer dstack_pop_all(struct DStack *ds) {
  size_t length = 0;
  void **els = NULL;
  uv_mutex_lock(&ds->mutex);
  if (ds->length == 0)
    goto end;
  els = malloc(ds->length * sizeof(*els));
  for (size_t i = 0; i < ds->length; i++) {
    els[i] = ds->elements[ds->read_curr + i]->actual;
    free(ds->elements[ds->read_curr + i]);
    ds->elements[ds->read_curr + i] = NULL;
  }
  length = ds->length;
  ds->length = 0;
  ds->read_curr = 0;
  ds->write_curr = 0;
end:
  uv_mutex_unlock(&ds->mutex);
  return (struct Buffer){.elements = els, .length = length};
}
