// ./src/core/thread_pool.c
#include <pthread.h>
#include <stdio.h>
#include "core/queue.h"
#include "classifier/classifier.h"
#include "mover/mover.h"
#include "core/lf_queue.h"
#include <unistd.h>
#include "core/context.h"

void *worker(void *arg)
{
    AppContext *ctx = (AppContext *)arg;
    FileEvent event;

    while (1)
    {
        if (lf_dequeue(&ctx->queue, &event))
        {
            const char *folder = classify(&ctx->classifier, event.path);

            if (move_file(event.path, folder) != 0)
            {
                fprintf(stderr, "Failed: %s\n", event.path);
            }
            else
            {
                fprintf(stderr, "Moved: %s\n", event.path);
            }
        }
        else
        {
            usleep(1000); // prevent CPU burn
        }
    }

    return NULL;
}

void start_workers(AppContext *ctx, int num_threads)
{
    pthread_t threads[num_threads];

    for (int i = 0; i < num_threads; i++)
    {
        pthread_create(&threads[i], NULL, worker, ctx);
    }
}