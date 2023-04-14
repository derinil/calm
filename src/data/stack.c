#include "stack.h"
#include "uv.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

struct DStack *create_dstack(FreeElement freer) {
  struct DStack *ds = malloc(sizeof(*ds));
  if (!ds)
    return NULL;
  memset(ds, 0, sizeof(*ds));
  if (uv_mutex_init(&ds->mutex))
    return NULL;
  if (uv_cond_init(&ds->cond))
    return NULL;
  ds->freer = freer;
  return ds;
}

void dstack_push(struct DStack *ds, void *element, int remove_at) {
  uv_mutex_lock(&ds->mutex);
  if (ds->elements[ds->write_curr].exists != 0 && ds->freer != NULL) {
    ds->freer(ds->elements[ds->write_curr].actual);
    ds->elements[ds->write_curr] = (struct DSElement){0};
  }
  ds->elements[ds->write_curr] = (struct DSElement){
      .actual = element, .read_count = 0, .exists = 1, .remove_at = remove_at};
  ds->write_curr++;
  ds->length++;
  if (ds->write_curr == MAX_DS_LEN)
    ds->write_curr = 0;
  uv_cond_signal(&ds->cond);
  uv_mutex_unlock(&ds->mutex);
}

void *dstack_pop_nonblock(struct DStack *ds) {
  void *el = NULL;
  struct DSElement *element;
  uv_mutex_lock(&ds->mutex);
  if (ds->length == 0)
    goto end;
  element = &ds->elements[ds->read_curr];
  if (element->exists == 0)
    goto end;
  el = element->actual;
  element->read_count++;
  if (element->remove_at > element->read_count)
    goto end;
  ds->read_curr++;
  ds->length--;
  if (ds->read_curr == MAX_DS_LEN)
    ds->read_curr = 0;
  ds->elements[ds->read_curr] = (struct DSElement){0};
end:
  uv_mutex_unlock(&ds->mutex);
  return el;
}

void *dstack_pop_block(struct DStack *ds) {
  void *el = NULL;
  struct DSElement *element;
  uv_mutex_lock(&ds->mutex);
  uv_cond_wait(&ds->cond, &ds->mutex);
  if (ds->length == 0)
    goto end;
  element = &ds->elements[ds->read_curr];
  if (element->exists == 0)
    goto end;
  el = element->actual;
  element->read_count++;
  if (element->remove_at > element->read_count)
    goto end;
  ds->read_curr++;
  ds->length--;
  if (ds->read_curr == MAX_DS_LEN)
    ds->read_curr = 0;
  ds->elements[ds->read_curr] = (struct DSElement){0};
end:
  uv_mutex_unlock(&ds->mutex);
  return el;
}
