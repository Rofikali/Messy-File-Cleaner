// ./src/watcher/watcher.c

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <sys/inotify.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>

#include "utils/limits.h" // ✅ your central limits
#include "watcher/watcher.h"

#define EVENT_BUF_LEN (1024 * (sizeof(struct inotify_event) + 16))

static void queue_backoff(AppContext *ctx)
{
    atomic_fetch_add(&ctx->metrics.queue_backpressure_waits, 1);

    const struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = 1000000,
    };
    nanosleep(&delay, NULL);
}

static void idle_backoff(AppContext *ctx)
{
    atomic_fetch_add(&ctx->metrics.watcher_idle_polls, 1);

    const struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = 1000000,
    };
    nanosleep(&delay, NULL);
}

static void enqueue_path(const char *path, AppContext *ctx)
{
    atomic_fetch_add(&ctx->metrics.events_seen, 1);
    atomic_fetch_add(&ctx->metrics.watcher_events, 1);

    FileEvent e;

    strncpy(e.path, path, sizeof(e.path) - 1);
    e.path[sizeof(e.path) - 1] = '\0';
    e.type = FILE_CREATED;

    int idx = dispatch(ctx, path);
    while (atomic_load(&ctx->running) && !lf_enqueue(&ctx->queues[idx], &e))
        queue_backoff(ctx);

    if (!atomic_load(&ctx->running))
        return;

    atomic_fetch_add(&ctx->metrics.events_enqueued, 1);

    uint64_t one = 1;
    write(ctx->efd[idx], &one, sizeof(one));
}

int dispatch(AppContext *ctx, const char *path)
{
    unsigned long h = 5381;

    for (const char *p = path; *p; p++)
        h = ((h << 5) + h) + *p;

    if (ctx->num_workers <= 0)
        return 0;

    return h % ctx->num_workers;
}

void start_watcher(const char *path, AppContext *ctx)
{
    int fd = inotify_init1(IN_NONBLOCK);
    if (fd < 0)
    {
        perror("inotify_init");
        return;
    }

    int wd = inotify_add_watch(fd, path, IN_CREATE | IN_MOVED_TO);
    if (wd < 0)
    {
        perror("inotify_add_watch");
        close(fd);
        return;
    }

    char buffer[EVENT_BUF_LEN];

    // while (1)
    while (atomic_load(&ctx->running))
    {
        ssize_t length = read(fd, buffer, EVENT_BUF_LEN);
        if (length <= 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            {
                idle_backoff(ctx);
                continue;
            }
            continue;
        }

        int i = 0;

        while (i < length)
        {
            struct inotify_event *event =
                (struct inotify_event *)&buffer[i];

            if (event->len && !(event->mask & IN_ISDIR))
            {
                char full_path[PATH_MAX];

                int written = snprintf(full_path, sizeof(full_path),
                                       "%s/%s", path, event->name);

                if (written < 0 || written >= (int)sizeof(full_path))
                {
                    fprintf(stderr, "Path too long, skipping\n");
                }
                else
                {
                    enqueue_path(full_path, ctx);
                }
            }

            i += sizeof(struct inotify_event) + event->len;
        }
    }

    inotify_rm_watch(fd, wd);
    close(fd);
}
