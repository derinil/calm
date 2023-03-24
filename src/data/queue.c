#include "queue.h"
#include <stdlib.h>

struct Stack *ds_create()
{
    struct Stack *s = malloc(sizeof(*s));
    if (!s)
        return NULL;
    s->head = 0;
    s->tail = 0;
    return s;
}

void ds_push(struct Stack *s, void *element)
{
    s->elements[s->head++] = element;
    if (s->head == MAX_QUEUE)
        s->head = 0;
}

void *ds_pop(struct Stack *s)
{
    void *element = s->elements[s->tail++];
    if (s->tail == MAX_QUEUE)
        s->tail = 0;
    return element;
}
