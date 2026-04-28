#include "queue/queue.h"

void enqueue(EventQueue* q, FileEvent* e) {
    q->buffer[q->tail] = *e;
    q->tail = (q->tail + 1) % QUEUE_SIZE;
}

int dequeue(EventQueue* q, FileEvent* e) {
    if (q->head == q->tail) return 0;

    *e = q->buffer[q->head];
    q->head = (q->head + 1) % QUEUE_SIZE;
    return 1;
}