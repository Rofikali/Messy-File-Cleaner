// ./src/watcher/watcher.c

#include <sys/inotify.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "watcher/watcher.h"

#define EVENT_BUF_LEN (1024 * (sizeof(struct inotify_event) + 16))

static void enqueue_path(const char *path, AppContext *ctx)
{
    FileEvent e;
    strncpy(e.path, path, sizeof(e.path) - 1);
    e.path[sizeof(e.path) - 1] = '\0';
    e.type = FILE_CREATED;

    int idx = dispatch(ctx, path);
    lf_enqueue(&ctx->queues[idx], &e);
}

int dispatch(AppContext *ctx, const char *path)
{
    unsigned long h = 5381;
    for (const char *p = path; *p; p++)
        h = ((h << 5) + h) + *p;

    return h % ctx->num_workers;
}

void start_watcher(const char *path, AppContext *ctx)
{
    int fd = inotify_init();
    if (fd < 0)
        return;

    inotify_add_watch(fd, path, IN_CREATE | IN_MOVED_TO);

    char buffer[EVENT_BUF_LEN];

    while (1)
    {
        int length = read(fd, buffer, EVENT_BUF_LEN);
        if (length <= 0)
            continue;

        int i = 0;
        while (i < length)
        {
            struct inotify_event *event =
                (struct inotify_event *)&buffer[i];

            if (event->len && !(event->mask & IN_ISDIR))
            {
                char full_path[PATH_MAX];
                snprintf(full_path, PATH_MAX, "%s/%s", path, event->name);

                enqueue_path(full_path, ctx);
            }

            i += sizeof(struct inotify_event) + event->len;
        }
    }
}