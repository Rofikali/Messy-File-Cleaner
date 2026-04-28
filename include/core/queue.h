#ifndef QUEUE_H
#define QUEUE_H

#include "event.h"

#define QUEUE_SIZE 1024

typedef struct {
    FileEvent buffer[QUEUE_SIZE];
    int head;
    int tail;
} EventQueue;

void enqueue(EventQueue* q, FileEvent* e);
int dequeue(EventQueue* q, FileEvent* e);

#endif