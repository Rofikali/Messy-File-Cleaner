#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "core/context.h"
#include "core/lf_queue.h"

typedef struct
{
    LFQueue *queue;
    AppContext *ctx;
    int efd;
} WorkerArgs;

/* Starts all requested workers or cleans up every partially started worker. */
int start_workers(AppContext *ctx, int num_threads);

/* Stops intake, wakes workers, drains queued work, joins threads, and closes fds. */
int join_workers(AppContext *ctx);

/* Wake the single consumer assigned to a queue after a successful enqueue. */
int notify_worker(AppContext *ctx, int worker_index);

#endif
