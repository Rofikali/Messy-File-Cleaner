#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "classifier/classifier.h"
#include "core/context.h"
#include "core/thread_pool.h"
#include "utils/hashmap.h"
#include "watcher/watcher.h"

static AppContext ctx;
static HashEntry entries[64];
static char test_root[128];

static void *watcher_entry(void *unused)
{
    (void)unused;
    int result = start_watcher(test_root, &ctx);
    if (result != 0)
        atomic_store(&ctx.watcher_error, errno ? errno : EIO);
    return NULL;
}

static void sleep_milliseconds(long milliseconds)
{
    struct timespec delay = {
        .tv_sec = milliseconds / 1000,
        .tv_nsec = (milliseconds % 1000) * 1000000L,
    };
    while (nanosleep(&delay, &delay) != 0 && errno == EINTR)
        ;
}

int main(void)
{
    hashmap_init(&ctx.cfg.ext_map, entries, 64);
    classifier_init(&ctx.classifier, &ctx.cfg.ext_map, "Others/");

    atomic_init(&ctx.running, 1);
    atomic_init(&ctx.active_workers, 0);
    atomic_init(&ctx.scanning_done, 0);
    atomic_init(&ctx.worker_error, 0);
    atomic_init(&ctx.watcher_error, 0);
    atomic_init(&ctx.dry_run, 1);
    atomic_init(&ctx.metrics.dry_run_moves, 0);
    atomic_init(&ctx.metrics.files_moved, 0);
    atomic_init(&ctx.metrics.move_failures, 0);
    atomic_init(&ctx.metrics.events_seen, 0);
    atomic_init(&ctx.metrics.events_enqueued, 0);
    atomic_init(&ctx.metrics.queue_backpressure_waits, 0);
    atomic_init(&ctx.metrics.watcher_events, 0);
    atomic_init(&ctx.metrics.watcher_idle_polls, 0);

    char missing[] = "/tmp/fc-watcher-missing-directory";
    errno = 0;
    assert(start_watcher(missing, &ctx) == -1);
    assert(errno == ENOENT);

    (void)snprintf(test_root, sizeof(test_root), "/tmp/fc-watcher-XXXXXX");
    assert(mkdtemp(test_root) != NULL);

    lf_init(&ctx.queues[0]);
    assert(start_workers(&ctx, 1) == 0);

    pthread_t watcher;
    assert(pthread_create(&watcher, NULL, watcher_entry, NULL) == 0);
    sleep_milliseconds(50);

    char file_path[256];
    (void)snprintf(file_path, sizeof(file_path), "%s/new-file.txt", test_root);
    int fd = open(file_path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    assert(fd >= 0);
    assert(write(fd, "watch test", 10) == 10);
    assert(close(fd) == 0);

    for (int i = 0; i < 2000 &&
         atomic_load(&ctx.metrics.dry_run_moves) == 0; i++)
        sleep_milliseconds(1);

    assert(atomic_load(&ctx.metrics.watcher_events) == 1);
    assert(atomic_load(&ctx.metrics.events_enqueued) == 1);
    assert(atomic_load(&ctx.metrics.dry_run_moves) == 1);
    assert(access(file_path, F_OK) == 0);

    atomic_store(&ctx.running, 0);
    assert(pthread_join(watcher, NULL) == 0);
    assert(atomic_load(&ctx.watcher_error) == 0);
    assert(join_workers(&ctx) == 0);

    assert(unlink(file_path) == 0);
    assert(rmdir(test_root) == 0);

    puts("watcher integration tests passed");
    return 0;
}
