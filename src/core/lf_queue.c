// ./src/core/lf_queue.c
#include "core/lf_queue.h"
#include <string.h>

// LFQueue queue;

void lf_init(LFQueue *q)
{
    atomic_init(&q->head, 0);
    atomic_init(&q->tail, 0);
}

int lf_enqueue(LFQueue *q, const FileEvent *e)
{
    int tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    int next = (tail + 1) % QUEUE_SIZE;
    int head = atomic_load_explicit(&q->head, memory_order_acquire);

    if (next == head)
        return 0; // Full

    q->buffer[tail] = *e;
    atomic_store_explicit(&q->tail, next, memory_order_release);
    return 1;
}

int lf_dequeue(LFQueue *q, FileEvent *e)
{
    int head = atomic_load_explicit(&q->head, memory_order_relaxed);
    int tail = atomic_load_explicit(&q->tail, memory_order_acquire);

    if (head == tail)
        return 0; // Empty

    *e = q->buffer[head];
    atomic_store_explicit(&q->head, (head + 1) % QUEUE_SIZE, memory_order_release);
    return 1;
}

// #include "core/lf_queue.h"

// void lf_init(LFQueue *q)
// {
//     atomic_store(&q->head, 0);
//     atomic_store(&q->tail, 0);
// }

// int lf_enqueue(LFQueue *q, const FileEvent *e)
// {
//     int tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
//     int next = (tail + 1) % QUEUE_SIZE;

//     int head = atomic_load_explicit(&q->head, memory_order_acquire);

//     if (next == head)
//         return 0; // full

//     q->buffer[tail] = *e;

//     atomic_store_explicit(&q->tail, next, memory_order_release);
//     return 1;
// }

// int lf_dequeue(LFQueue *q, FileEvent *e)
// {
//     int head = atomic_load_explicit(&q->head, memory_order_relaxed);

//     int tail = atomic_load_explicit(&q->tail, memory_order_acquire);

//     if (head == tail)
//         return 0; // empty

//     *e = q->buffer[head];

//     atomic_store_explicit(&q->head, (head + 1) % QUEUE_SIZE, memory_order_release);
//     return 1;
// }