// ./include/core/lf_queue.h
// include/core/lf_queue.h

#ifndef LF_QUEUE_H
#define LF_QUEUE_H

#include <stdatomic.h>
#include <stdalign.h>
#include "core/event.h"

#define QUEUE_SIZE 1024

// 1. Define the type FIRST
typedef struct
{
    FileEvent buffer[QUEUE_SIZE];
    alignas(64) atomic_int head;
    alignas(64) atomic_int tail;
} LFQueue;

void lf_init(LFQueue *q);
int lf_enqueue(LFQueue *q, const FileEvent *e);
int lf_dequeue(LFQueue *q, FileEvent *e);

#endif
