#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <limits.h>
#include "scanner/scanner.h"
#include "utils/hashmap.h"
#include "classifier/classifier.h"
#include "mover/mover.h"
#include "watcher/watcher.h"
#include "core/thread_pool.h"
#include "core/lf_queue.h"
#include "core/context.h"

#define MAP_CAP 32

static HashEntry buffer[MAP_CAP];
static HashMap map;

// 1. Just one global ctx. No need for g_ctx pointer.
AppContext ctx;

void *watcher_thread(void *arg)
{
    // 2. Pass the global ctx to the watcher
    start_watcher((const char *)arg, &ctx);
    return NULL;
}

void init_system()
{
    hashmap_init(&map, buffer, MAP_CAP);
    hashmap_put(&map, ".jpg", "Jpeg/");
    hashmap_put(&map, ".jpeg", "Jpeg/");
    hashmap_put(&map, ".png", "Png/");

    classifier_init(&ctx.classifier, &map, "Others/");
}

// 3. Keep exactly ONE argument so it matches scan_directory's requirements
void enqueue_event(const char *path)
{
    FileEvent e;
    strncpy(e.path, path, sizeof(e.path) - 1);
    e.path[sizeof(e.path) - 1] = '\0';
    e.type = FILE_CREATED;

    // 4. Use the global ctx directly
    lf_enqueue(&ctx.queue, &e);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    init_system();
    lf_init(&ctx.queue);

    // 5. If start_workers needs ctx, pass it here
    start_workers(&ctx, 2);

    pthread_t tid;
    pthread_create(&tid, NULL, watcher_thread, argv[1]);

    // 6. This now matches the signature void (*)(const char *)
    scan_directory(argv[1], enqueue_event);

    pthread_join(tid, NULL);
    return 0;
}

// // ./src/main.c
// #include <stdio.h>
// #include <pthread.h>
// #include <string.h>
// #include <limits.h> // <--- ADD THIS for PATH_MAX
// #include "scanner/scanner.h"
// #include "utils/hashmap.h"
// #include "classifier/classifier.h"
// #include "mover/mover.h"
// #include "watcher/watcher.h"
// #include "core/thread_pool.h"
// #include "core/lf_queue.h"
// #include "core/context.h"

// #define MAP_CAP 32

// static HashEntry buffer[MAP_CAP];
// static HashMap map;
// AppContext ctx;

// void *watcher_thread(void *arg)
// {
//     start_watcher((const char *)arg);
//     return NULL;
// }

// void init_system()
// {
//     hashmap_init(&map, buffer, MAP_CAP);

//     hashmap_put(&map, ".jpg", "Jpeg/");
//     hashmap_put(&map, ".jpeg", "Jpeg/");
//     hashmap_put(&map, ".png", "Png/");

//     classifier_init(&ctx.classifier, &map, "Others/");
// }

// // void enqueue_event(const char *path)
// void enqueue_event(const char *path, AppContext *ctx)
// {
//     FileEvent e;
//     // Use sizeof(e.path) instead of PATH_MAX
//     strncpy(e.path, path, sizeof(e.path) - 1);
//     e.path[sizeof(e.path) - 1] = '\0'; // Manually ensure null termination
//     e.type = FILE_CREATED;

//     lf_enqueue(&ctx.queue, &e);
// }

// int main(int argc, char *argv[])
// {
//     if (argc < 2)
//     {
//         printf("Usage: %s <directory>\n", argv[0]);
//         return 1;
//     }

//     init_system();

//     lf_init(&ctx.queue);
//     start_workers(2);

//     pthread_t tid;
//     pthread_create(&tid, NULL, watcher_thread, argv[1]);

//     // This scans existing files and puts them in the queue
//     scan_directory(argv[1], enqueue_event);

//     // 🔥 CRITICAL: Keep main alive so threads can work
//     pthread_join(tid, NULL);

//     return 0;
// }
