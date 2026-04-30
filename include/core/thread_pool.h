#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "core/context.h"

typedef struct
{
    LFQueue *queue;
    AppContext *ctx;
} WorkerArgs;

void start_workers(AppContext *ctx, int num_threads);

#endif