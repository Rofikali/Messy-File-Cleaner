#ifndef CONTEXT_H
#define CONTEXT_H

#include "core/lf_queue.h"
#include "classifier/classifier.h"
#include "config/config.h"

// 🔥 SINGLE SOURCE OF TRUTH

typedef struct
{
    LFQueue *queue;
    struct AppContext *ctx;
} WorkerArgs;

typedef struct AppContext
{
    LFQueue queues[8]; // max workers (tune if needed)
    int num_workers;

    Classifier classifier;
    Config cfg;

} AppContext;

#endif
// // include/core/context.h

// #ifndef CONTEXT_H
// #define CONTEXT_H

// #include "core/lf_queue.h"
// #include "classifier/classifier.h"

// #define MAX_WORKERS 8

// typedef struct
// {
//     LFQueue queues[MAX_WORKERS];
//     int num_workers;
//     Classifier classifier;
// } AppContext;

// #endif
