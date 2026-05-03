// ./include/core/queue.h
// include/core/queue.h

#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include "event.h"

#define QUEUE_SIZE 1024

typedef struct
{
    FileEvent buffer[QUEUE_SIZE];
    int head, tail;
    pthread_mutex_t lock;
} EventQueue;

int enqueue(EventQueue *q, const FileEvent *e);
int dequeue(EventQueue *q, FileEvent *e);

#endif // QUEUE_H
