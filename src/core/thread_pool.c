#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "core/thread_pool.h"
#include "mover/mover.h"
#include "classifier/classifier.h"

// 🔥 WORKER LOOP (CONFIG-DRIVEN BATCHING)
void *worker(void *arg)
{
    WorkerArgs *w = (WorkerArgs *)arg;
    LFQueue *q = w->queue;
    AppContext *ctx = w->ctx;

    int BATCH = ctx->cfg.batch_size;

    FileEvent *batch = malloc(sizeof(FileEvent) * BATCH);
    int count = 0;

    struct timespec sleep_ts;
    sleep_ts.tv_sec = 0;
    sleep_ts.tv_nsec = ctx->cfg.idle_sleep_ms * 1000000L;

    while (1)
    {
        FileEvent e;

        // ── fill batch ──
        while (count < BATCH && lf_dequeue(q, &e))
        {
            batch[count++] = e;
        }

        if (count > 0)
        {
            for (int i = 0; i < count; i++)
            {
                const char *folder =
                    classify(&ctx->classifier, batch[i].path);

                if (move_file(batch[i].path, folder) == 0)
                    fprintf(stderr, "Moved: %s\n", batch[i].path);
                else
                    fprintf(stderr, "Failed: %s\n", batch[i].path);
            }

            count = 0;
        }
        else
        {
            nanosleep(&sleep_ts, NULL);
        }
    }

    return NULL;
}

void start_workers(AppContext *ctx, int num_threads)
{
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    WorkerArgs *args = malloc(sizeof(WorkerArgs) * num_threads);

    ctx->num_workers = num_threads;

    for (int i = 0; i < num_threads; i++)
    {
        args[i].queue = &ctx->queues[i];
        args[i].ctx = ctx;

        pthread_create(&threads[i], NULL, worker, &args[i]);
    }
}

// #define _POSIX_C_SOURCE 200809L

// #include <pthread.h>
// #include <time.h>
// #include <stdio.h>
// #include <stdlib.h>

// #include "core/thread_pool.h"
// #include "mover/mover.h"
// #include "classifier/classifier.h"

// // ─────────────────────────────────────────────
// // 🔥 BATCHED WORKER (RUNTIME CONFIG)
// // ─────────────────────────────────────────────
// void *worker(void *arg)
// {
//     WorkerArgs *w = (WorkerArgs *)arg;
//     LFQueue *q = w->queue;
//     AppContext *ctx = w->ctx;

//     int batch_size = ctx->config->batch_size > 0
//                          ? ctx->config->batch_size
//                          : 64;

//     FileEvent *batch = malloc(sizeof(FileEvent) * batch_size);
//     int count = 0;

//     struct timespec sleep_1ms = {0, 1000000L};

//     while (1)
//     {
//         FileEvent e;

//         // ── Fill batch ─────────────────
//         while (count < batch_size && lf_dequeue(q, &e))
//         {
//             batch[count++] = e;
//         }

//         // ── Process batch ──────────────
//         if (count > 0)
//         {
//             for (int i = 0; i < count; i++)
//             {
//                 const char *folder =
//                     classify(&ctx->classifier, batch[i].path);

//                 if (move_file(batch[i].path, folder) == 0)
//                 {
//                     fprintf(stderr, "Moved: %s\n", batch[i].path);
//                 }
//                 else
//                 {
//                     fprintf(stderr, "Failed: %s\n", batch[i].path);
//                 }
//             }

//             count = 0;
//         }
//         else
//         {
//             nanosleep(&sleep_1ms, NULL);
//         }
//     }

//     return NULL;
// }
