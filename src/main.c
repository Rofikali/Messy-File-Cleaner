#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "scanner/scanner.h"
#include "utils/hashmap.h"
#include "classifier/classifier.h"
#include "watcher/watcher.h"
#include "core/thread_pool.h"
#include "core/context.h"

#define MAP_CAP 32

static HashEntry buffer[MAP_CAP];
static HashMap map;

AppContext ctx;

void *watcher_thread(void *arg)
{
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

    init_system();

    int workers = 2;

    for (int i = 0; i < workers; i++)
        lf_init(&ctx.queues[i]);

    start_workers(&ctx, workers);

    pthread_t tid;
    pthread_create(&tid, NULL, watcher_thread, argv[1]);

    scan_directory(argv[1], enqueue_event);

    pthread_join(tid, NULL);
    return 0;
}