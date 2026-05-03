// ./src/core/queue.c

#include "core/queue.h"
// EventQueue event_queue; // This actually allocates the memory for the queue

int enqueue(EventQueue *q, const FileEvent *e)
{
    pthread_mutex_lock(&q->lock);
    int next = (q->tail + 1) % QUEUE_SIZE;

    if (next == q->head)
    {
        pthread_mutex_unlock(&q->lock);
        return 0;
    }

    q->buffer[q->tail] = *e;
    q->tail = next;
    pthread_mutex_unlock(&q->lock);
    return 1;
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
    pthread_mutex_unlock(&q->lock);
    return 1;
}
