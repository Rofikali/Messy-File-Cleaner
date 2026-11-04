#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

#include "core/thread_pool.h"
#include "watcher/watcher.h"

#define EVENT_BUF_LEN (1024U * (sizeof(struct inotify_event) + 16U))
#define WATCH_MASK (IN_CREATE | IN_MOVED_TO | IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT)

static void queue_backoff(AppContext *ctx)
{
    atomic_fetch_add(&ctx->metrics.queue_backpressure_waits, 1);
    const struct timespec delay = {.tv_sec = 0, .tv_nsec = 1000000};
    while (nanosleep(&delay, NULL) != 0 && errno == EINTR)
        ;
}

static void enqueue_path(const char *path, AppContext *ctx)
{
    atomic_fetch_add(&ctx->metrics.events_seen, 1);
    atomic_fetch_add(&ctx->metrics.watcher_events, 1);

    FileEvent event = {0};
    size_t path_length = strlen(path);
    if (path_length >= sizeof(event.path))
    {
        atomic_store(&ctx->watcher_error, ENAMETOOLONG);
        atomic_store(&ctx->running, 0);
        return;
    }
    memcpy(event.path, path, path_length + 1);
    event.type = FILE_CREATED;

    int index = dispatch(ctx, path);
    while (atomic_load(&ctx->running) && !lf_enqueue(&ctx->queues[index], &event))
        queue_backoff(ctx);

    if (!atomic_load(&ctx->running))
        return;

    atomic_fetch_add(&ctx->metrics.events_enqueued, 1);
    if (notify_worker(ctx, index) != 0)
    {
        int saved_errno = errno;
        fprintf(stderr, "Worker notification failed: %s\n", strerror(saved_errno));
        atomic_store(&ctx->worker_error, 1);
        atomic_store(&ctx->running, 0);
    }
}

int dispatch(AppContext *ctx, const char *path)
{
    if (!ctx || !path || ctx->num_workers <= 0)
    {
        errno = EINVAL;
        return -1;
    }

    unsigned long hash = 5381UL;
    for (const unsigned char *p = (const unsigned char *)path; *p; p++)
        hash = ((hash << 5) + hash) + *p;

    return (int)(hash % (unsigned long)ctx->num_workers);
}

static int process_event_buffer(const char *buffer, size_t length,
                                const char *root, AppContext *ctx)
{
    size_t offset = 0;
    while (offset < length)
    {
        size_t remaining = length - offset;
        if (remaining < sizeof(struct inotify_event))
        {
            errno = EBADMSG;
            return -1;
        }

        const struct inotify_event *event =
            (const struct inotify_event *)(const void *)(buffer + offset);
        if ((size_t)event->len > remaining - sizeof(*event))
        {
            errno = EBADMSG;
            return -1;
        }

        size_t record_size = sizeof(*event) + (size_t)event->len;
        if (event->mask & IN_Q_OVERFLOW)
        {
            /* Events have been lost; do not pretend watch mode is complete. */
            errno = ENOSPC;
            return -1;
        }
        if (event->mask & (IN_IGNORED | IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT))
        {
            errno = ESTALE;
            return -1;
        }

        if (event->len != 0 && !(event->mask & IN_ISDIR) &&
            (event->mask & (IN_CREATE | IN_MOVED_TO)))
        {
            size_t name_length = strnlen(event->name, event->len);
            if (name_length == event->len)
            {
                errno = EBADMSG;
                return -1;
            }

            size_t root_length = strlen(root);
            size_t name_offset = root_length;
            int separator = root_length > 0 && root[root_length - 1] != '/';
            if (root_length > SIZE_MAX - (size_t)separator - name_length - 1)
            {
                errno = EOVERFLOW;
                return -1;
            }

            size_t needed = root_length + (size_t)separator + name_length + 1;
            char *full_path = malloc(needed);
            if (!full_path)
                return -1;

            memcpy(full_path, root, root_length);
            if (separator)
                full_path[name_offset++] = '/';
            memcpy(full_path + name_offset, event->name, name_length + 1);

            enqueue_path(full_path, ctx);
            free(full_path);

            if (!atomic_load(&ctx->running))
            {
                errno = atomic_load(&ctx->watcher_error);
                if (errno == 0)
                    errno = ECANCELED;
                return -1;
            }
        }

        offset += record_size;
    }
    return 0;
}

int start_watcher(const char *path, AppContext *ctx)
{
    if (!path || path[0] == '\0' || !ctx)
    {
        errno = EINVAL;
        return -1;
    }

    int fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (fd < 0)
        return -1;

    int watch_descriptor = inotify_add_watch(fd, path, WATCH_MASK);
    if (watch_descriptor < 0)
    {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }

    _Alignas(struct inotify_event) char buffer[EVENT_BUF_LEN];
    int status = 0;
    int saved_errno = 0;

    while (atomic_load(&ctx->running))
    {
        struct pollfd descriptor = {
            .fd = fd,
            .events = POLLIN,
            .revents = 0,
        };

        int ready = poll(&descriptor, 1, 250);
        if (ready < 0)
        {
            if (errno == EINTR)
                continue;
            status = -1;
            saved_errno = errno;
            break;
        }
        if (ready == 0)
            continue;
        if (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))
        {
            status = -1;
            saved_errno = EIO;
            break;
        }
        if (!(descriptor.revents & POLLIN))
            continue;

        ssize_t count;
        do
        {
            count = read(fd, buffer, sizeof(buffer));
        } while (count < 0 && errno == EINTR);

        if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            continue;
        if (count <= 0)
        {
            status = -1;
            saved_errno = count == 0 ? EIO : errno;
            break;
        }

        if (process_event_buffer(buffer, (size_t)count, path, ctx) != 0)
        {
            status = -1;
            saved_errno = errno;
            break;
        }
    }

    if (inotify_rm_watch(fd, watch_descriptor) != 0 &&
        errno != EINVAL && status == 0)
    {
        status = -1;
        saved_errno = errno;
    }
    if (close(fd) != 0 && status == 0)
    {
        status = -1;
        saved_errno = errno;
    }

    if (status != 0)
    {
        atomic_store(&ctx->watcher_error, saved_errno ? saved_errno : EIO);
        errno = saved_errno ? saved_errno : EIO;
        return -1;
    }
    return 0;
}
