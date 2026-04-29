// ./src/core/queue.c
// src/core/queue.c

#include "core/queue.h"
// EventQueue event_queue; // This actually allocates the memory for the queue

void enqueue(EventQueue *q, FileEvent *e)
{
    pthread_mutex_lock(&q->lock);
    q->buffer[q->tail] = *e;
    q->tail = (q->tail + 1) % QUEUE_SIZE;
}

int dequeue(EventQueue *q, FileEvent *e)
{
    pthread_mutex_lock(&q->lock);
    if (q->head == q->tail)
    {
        pthread_mutex_unlock(&q->lock);
        return 0;
    }

    *e = q->buffer[q->head];
    q->head = (q->head + 1) % QUEUE_SIZE;
    return 1;
}
