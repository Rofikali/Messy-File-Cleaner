#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "scanner/scanner.h"
#include "watcher/watcher.h"
#include "core/thread_pool.h"
#include "core/context.h"
#include "config/config.h"

AppContext ctx;

void *watcher_thread(void *arg)
{
    start_watcher((const char *)arg, &ctx);
    return NULL;
}

void enqueue_event(const char *path)
{
    FileEvent e;

    strncpy(e.path, path, sizeof(e.path) - 1);
    e.path[sizeof(e.path) - 1] = '\0';
    e.type = FILE_CREATED;

    int idx = dispatch(&ctx, path);
    lf_enqueue(&ctx.queues[idx], &e);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    // ── LOAD CONFIG ─────────────────────────
    if (load_config("configs/default.conf", &ctx.cfg) != 0)
    {
        printf("Failed to load config\n");
        return 1;
    }

    // ── INIT HASHMAP INSIDE CONFIG ─────────
    static HashEntry buffer[64];
    hashmap_init(&ctx.cfg.ext_map, buffer, 64);

    // ── INIT CLASSIFIER ────────────────────
    classifier_init(&ctx.classifier, &ctx.cfg.ext_map, "Others/");

    // ── INIT QUEUES ───────────────────────
    for (int i = 0; i < ctx.cfg.worker_threads; i++)
        lf_init(&ctx.queues[i]);

    // ── START WORKERS ─────────────────────
    start_workers(&ctx, ctx.cfg.worker_threads);

    // ── WATCHER THREAD ────────────────────
    pthread_t tid;
    pthread_create(&tid, NULL, watcher_thread, argv[1]);

    // ── INITIAL SCAN ──────────────────────
    scan_directory(argv[1], enqueue_event);

    pthread_join(tid, NULL);
    return 0;
}

// // ./src/main.c
// #define _POSIX_C_SOURCE 200809L

// #include <stdio.h>
// #include <pthread.h>
// #include <string.h>

// #include "scanner/scanner.h"
// #include "utils/hashmap.h"
// #include "classifier/classifier.h"
// #include "watcher/watcher.h"
// #include "core/thread_pool.h"
// #include "core/context.h"
// #include "core/lf_queue.h"
// #include "config/config.h"

// #define MAP_CAP 32

// // ─────────────────────────────────────────────
// // GLOBALS
// // ─────────────────────────────────────────────
// static HashEntry buffer[MAP_CAP];
// AppContext ctx;
// Config cfg;

// // ─────────────────────────────────────────────
// // WATCHER THREAD
// // ─────────────────────────────────────────────
// void *watcher_thread(void *arg)
// {
//     start_watcher((const char *)arg, &ctx);
//     return NULL;
// }

// // ─────────────────────────────────────────────
// // CONFIG + SYSTEM INIT
// // ─────────────────────────────────────────────
// void init_system()
// {
//     // 1. Init hashmap inside config
//     hashmap_init(&cfg.ext_map, buffer, MAP_CAP);

//     // 2. Load config file
//     if (load_config("configs/default.conf", &cfg) != 0)
//     {
//         fprintf(stderr, "Failed to load config\n");
//     }

//     // 3. Init classifier using config
//     classifier_init(&ctx.classifier, &cfg.ext_map, "Others/");
// }

// // ─────────────────────────────────────────────
// // EVENT → QUEUE
// // ─────────────────────────────────────────────
// void enqueue_event(const char *path)
// {
//     FileEvent e;

//     strncpy(e.path, path, sizeof(e.path) - 1);
//     e.path[sizeof(e.path) - 1] = '\0';
//     e.type = FILE_CREATED;

//     int idx = dispatch(&ctx, path);

//     lf_enqueue(&ctx.queues[idx], &e);
// }

// // ─────────────────────────────────────────────
// // MAIN
// // ─────────────────────────────────────────────
// int main(int argc, char *argv[])
// {
//     if (argc < 2)
//     {
//         printf("Usage: %s <directory>\n", argv[0]);
//         return 1;
//     }

//     // 1. Init system (config + classifier)
//     init_system();

//     // 2. Init queues (per-core)
//     int workers = cfg.worker_threads > 0 ? cfg.worker_threads : 2;

//     for (int i = 0; i < workers; i++)
//     {
//         lf_init(&ctx.queues[i]);
//     }

//     // 3. Start workers
//     start_workers(&ctx, workers);

//     // 4. Start watcher thread
//     pthread_t tid;
//     pthread_create(&tid, NULL, watcher_thread, argv[1]);

//     // 5. Initial scan (batch ingestion)
//     scan_directory(argv[1], enqueue_event);

//     // 6. Keep program alive
//     pthread_join(tid, NULL);

//     return 0;
// }
