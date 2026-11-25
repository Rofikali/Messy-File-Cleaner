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

/* Starts all requested workers or cleans up every partially started worker.
 * Only one pool may own the process-wide thread storage. Caller initializes
 * ctx->num_workers to zero and ctx->running; duplicate starts fail with EBUSY.
 * Serialize start/join across contexts; stop producers before join.
 * This function never overrides a stop request.
 */
int start_workers(AppContext *ctx, int num_threads);

/* Stops intake, wakes workers, drains queued work, joins threads, and closes fds.
 * Requires an exclusively managed pool and a stable count in [0, MAX_WORKERS].
 * Invalid count fails with EINVAL; a non-owner or inconsistent ownership fails
 * with EBUSY before changing state. Caller retains ownership on rejection.
 */
int join_workers(AppContext *ctx);

/* Wake the single consumer assigned to a queue after a successful enqueue.
 * Requires the owning live pool with a stable count in [1, MAX_WORKERS].
 */
int notify_worker(AppContext *ctx, int worker_index);

#ifdef FC_TESTING
/* Deterministic fault injection for worker-start cleanup tests only. */
void fc_test_fail_eventfd_after(int successful_calls);
void fc_test_fail_pthread_create_after(int successful_calls);
void fc_test_fail_notify_after(int successful_calls);
void fc_test_pause_worker_before_process(void);
int fc_test_wait_for_worker_pause(int timeout_ms);
void fc_test_release_worker(void);
#endif

#endif
