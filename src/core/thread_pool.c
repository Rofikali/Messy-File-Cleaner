#define _GNU_SOURCE

#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <stdatomic.h>

#include "core/thread_pool.h"
#include "mover/mover.h"
#include "classifier/classifier.h"

static WorkerArgs args_storage[16];
static pthread_t threads_storage[16];

void *worker(void *arg)
{
    WorkerArgs *w = (WorkerArgs *)arg;
    AppContext *ctx = w->ctx;
    LFQueue *q = w->queue;
    int efd = w->efd;

    FileEvent *batch = malloc(sizeof(FileEvent) * QUEUE_SIZE);
    if (!batch)
        return NULL;

    while (atomic_load(&ctx->running) || !lf_is_empty(q))
    {
        uint64_t counter;

        if (lf_is_empty(q) && read(efd, &counter, sizeof(counter)) <= 0)
            continue;

        atomic_fetch_add(&ctx->active_workers, 1);

        int count = 0;
        FileEvent e;

        while (lf_dequeue(q, &e))
        {
            if (count >= QUEUE_SIZE)
                break;

            batch[count++] = e;
        }

        if (count == 0)
        {
            atomic_fetch_sub(&ctx->active_workers, 1);
            continue;
        }

        for (int i = 0; i < count; i++)
        {
            const char *folder =
                classify(&ctx->classifier, batch[i].path);

            if (atomic_load(&ctx->dry_run))
            {
                fprintf(stderr, "Would move: %s -> %s\n", batch[i].path, folder);
                atomic_fetch_add(&ctx->metrics.dry_run_moves, 1);
                continue;
            }

            MoveResult result;
            if (move_file_journaled(batch[i].path, folder, &ctx->journal, &result) == 0)
            {
                fprintf(stderr, "Moved: %s -> %s\n", batch[i].path, result.destination);
                atomic_fetch_add(&ctx->metrics.files_moved, 1);
            }
            else
            {
                fprintf(stderr, "Move failed: %s -> %s (errno=%d)\n",
                        batch[i].path, folder, result.error_code);
                atomic_fetch_add(&ctx->metrics.move_failures, 1);
            }
        }

        atomic_fetch_sub(&ctx->active_workers, 1);
    }

    free(batch);
    return NULL;
}

void start_workers(AppContext *ctx, int num_threads)
{
    ctx->num_workers = num_threads;

    for (int i = 0; i < num_threads; i++)
    {
        ctx->efd[i] = eventfd(0, 0);

        args_storage[i].queue = &ctx->queues[i];
        args_storage[i].ctx = ctx;
        args_storage[i].efd = ctx->efd[i];

        pthread_create(&threads_storage[i], NULL, worker, &args_storage[i]);
    }
}

void join_workers(AppContext *ctx)
{
    for (int i = 0; i < ctx->num_workers; i++)
    {
        pthread_join(threads_storage[i], NULL);
        close(ctx->efd[i]);
    }
}
