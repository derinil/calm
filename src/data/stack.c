#include <stdlib.h>
#include <pthread.h>
#include <strings.h>
#include "stack.h"
#include <stdio.h>

struct DStack *create_dstack()
{
    struct DStack *ds = malloc(sizeof(*ds));
    if (!ds)
        return NULL;
    memset(ds, 0, sizeof(*ds));
    if (pthread_mutex_init(&ds->lock, NULL))
        return NULL;
    if (pthread_cond_init(&ds->ready, NULL))
        return NULL;
    return ds;
}

void dstack_push(struct DStack *ds, void *element)
{
    pthread_mutex_lock(&ds->lock);
    // TODO: memory leak! should have a callback thing to release an element before overriding it
    // TODO: stops working after max len
    ds->elements[ds->write_curr] = element;
    ds->write_curr++;
    ds->length++;
    if (ds->write_curr == MAX_DS_LEN)
        ds->write_curr = 0;
    // pthread_cond_signal(&ds->ready);
    pthread_mutex_unlock(&ds->lock);
}

// TODO: look into a cond vars
int dstack_ready(struct DStack *ds)
{
    return ds->read_curr == ds->write_curr;
}

// pop waits until there is anything to pop
void *dstack_pop(struct DStack *ds, int should_remove)
{
    void *el = NULL;
    // pthread_cond_wait(&ds->ready, &ds->lock);
    pthread_mutex_lock(&ds->lock);
    if (ds->length == 0)
        goto end;
    el = ds->elements[ds->read_curr];
    if (!should_remove)
        goto end;
    ds->read_curr++;
    ds->length--;
    if (ds->read_curr == MAX_DS_LEN)
        ds->read_curr = 0;
end:
    pthread_mutex_unlock(&ds->lock);
    return el;
}
