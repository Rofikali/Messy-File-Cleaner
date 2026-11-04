#ifndef LF_QUEUE_H
#define LF_QUEUE_H

#include <stdatomic.h>
#include "core/event.h"

#define QUEUE_SIZE 1024

/* SPSC only: exactly one producer and one consumer may access a queue.
 * Effective capacity is QUEUE_SIZE - 1 because one slot distinguishes full
 * from empty. Do not share this queue among multiple producers/consumers.
 */

typedef struct
{
    FileEvent buffer[QUEUE_SIZE];
    atomic_int head;
    atomic_int tail;
} LFQueue;

void lf_init(LFQueue *q);
int lf_enqueue(LFQueue *q, const FileEvent *e);
int lf_dequeue(LFQueue *q, FileEvent *e);
int lf_is_empty(LFQueue *q);

#endif

// // ./include/core/lf_queue.h
// // include/core/lf_queue.h

// #ifndef LF_QUEUE_H
// #define LF_QUEUE_H

// #include <stdatomic.h>
// #include <stdalign.h>
// #include "core/event.h"

// #define QUEUE_SIZE 1024

// // 1. Define the type FIRST
// typedef struct
// {
//     FileEvent buffer[QUEUE_SIZE];
//     alignas(64) atomic_int head;
//     alignas(64) atomic_int tail;
// } LFQueue;

// void lf_init(LFQueue *q);
// int lf_enqueue(LFQueue *q, const FileEvent *e);
// int lf_dequeue(LFQueue *q, FileEvent *e);

// #endif
