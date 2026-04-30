// ./src/core/thread_pool.c

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#include "core/thread_pool.h"
#include "mover/mover.h"
#include "classifier/classifier.h"

void *worker(void *arg)
{
    WorkerArgs *w = (WorkerArgs *)arg;
    LFQueue *q = w->queue;
    AppContext *ctx = w->ctx;

    FileEvent event;

    while (1)
    {
        if (lf_dequeue(q, &event))
        {
            const char *folder =
                classify(&ctx->classifier, event.path);

            if (move_file(event.path, folder) == 0)
                fprintf(stderr, "Moved: %s\n", event.path);
            else
                fprintf(stderr, "Failed: %s\n", event.path);
        }
        else
        {
            usleep(1000);
        }
    }

    return NULL;
}

void start_workers(AppContext *ctx, int num_threads)
{
    pthread_t threads[num_threads];
    WorkerArgs args[num_threads]; // ⚠️ stack is fine here

    ctx->num_workers = num_threads; // ✅ CRITICAL

    for (int i = 0; i < num_threads; i++)
    {
        args[i].queue = &ctx->queues[i];
        args[i].ctx = ctx;

        pthread_create(&threads[i], NULL, worker, &args[i]);
    }
}