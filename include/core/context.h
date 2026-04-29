// ./include/core/context.h
// include/core/context.h

#ifndef CONTEXT_H
#define CONTEXT_H

#include "core/lf_queue.h"
#include "classifier/classifier.h"

typedef struct
{
    LFQueue queue;
    Classifier classifier;
} AppContext;

#endif