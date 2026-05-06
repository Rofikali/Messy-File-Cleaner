#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include "core/context.h"
#include "core/lf_queue.h"

typedef struct
{
  LFQueue *queue;
  AppContext *ctx;
  int efd;
} WorkerArgs;

void start_workers(AppContext *ctx, int num_threads);
void join_workers(AppContext *ctx);

#endif
