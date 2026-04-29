// include/core/context.h

#ifndef CONTEXT_H
#define CONTEXT_H

#include "core/lf_queue.h"
#include "classifier/classifier.h"

#define MAX_WORKERS 8

typedef struct
{
    LFQueue queues[MAX_WORKERS];
    int num_workers;
    Classifier classifier;
} AppContext;

#endif