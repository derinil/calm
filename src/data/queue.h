#ifndef DATA_QUEUE_H_
#define DATA_QUEUE_H_

#include <stdatomic.h>

#define MAX_QUEUE 2048

// FIFO stack
struct Stack {
    void *elements[MAX_QUEUE];
    // For insertions
    atomic_int head;
    // For retrievals
    atomic_int tail;
};

#endif
