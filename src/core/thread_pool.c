#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "core/thread_pool.h"
#include "mover/mover.h"
#include "classifier/classifier.h"

static WorkerArgs args_storage[MAX_WORKERS];
static pthread_t threads_storage[MAX_WORKERS];

int notify_worker(AppContext *ctx, int worker_index)
{
    if (!ctx || worker_index < 0 || worker_index >= ctx->num_workers ||
        ctx->efd[worker_index] < 0)
    {
        errno = EINVAL;
        return -1;
    }

    const uint64_t increment = 1;
    for (;;)
    {
        ssize_t written = write(ctx->efd[worker_index],
                                &increment, sizeof(increment));
        if (written == (ssize_t)sizeof(increment))
            return 0;
        if (written < 0 && errno == EINTR)
            continue;
        /* A saturated eventfd is already readable; a wakeup is pending. */
        if (written < 0 && errno == EAGAIN)
            return 0;
        if (written >= 0)
            errno = EIO;
        return -1;
    }
}

static int wait_for_work(int event_fd)
{
    struct pollfd descriptor = {
        .fd = event_fd,
        .events = POLLIN,
        .revents = 0,
    };

    for (;;)
    {
        int ready = poll(&descriptor, 1, 250);
        if (ready < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (ready == 0)
            return 0;

        if (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))
        {
            errno = EBADF;
            return -1;
        }

        if (descriptor.revents & POLLIN)
        {
            uint64_t counter;
            ssize_t count;
            do
            {
                count = read(event_fd, &counter, sizeof(counter));
            } while (count < 0 && errno == EINTR);

            if (count == (ssize_t)sizeof(counter))
                return 1;
            if (count < 0 && errno == EAGAIN)
                return 0;
            if (count >= 0)
                errno = EIO;
            return -1;
        }
    }
}

static void process_event(AppContext *ctx, const FileEvent *event)
{
    const char *folder = classify(&ctx->classifier, event->path);

    if (atomic_load(&ctx->dry_run))
    {
        fprintf(stderr, "Would move: %s -> %s\n", event->path, folder);
        atomic_fetch_add(&ctx->metrics.dry_run_moves, 1);
        return;
    }

    MoveResult result;
    if (move_file_journaled(event->path, folder, &ctx->journal, &result) == 0)
    {
        fprintf(stderr, "Moved: %s -> %s\n", event->path, result.destination);
        atomic_fetch_add(&ctx->metrics.files_moved, 1);
    }
    else
    {
        fprintf(stderr, "Move failed: %s -> %s (errno=%d)\n",
                event->path, folder, result.error_code);
        atomic_fetch_add(&ctx->metrics.move_failures, 1);
    }
}

static void *worker(void *arg)
{
    WorkerArgs *worker_args = arg;
    AppContext *ctx = worker_args->ctx;
    LFQueue *queue = worker_args->queue;

    for (;;)
    {
        FileEvent event;
        if (lf_dequeue(queue, &event))
        {
            atomic_fetch_add(&ctx->active_workers, 1);
            process_event(ctx, &event);
            atomic_fetch_sub(&ctx->active_workers, 1);
            continue;
        }

        if (!atomic_load(&ctx->running))
            break;

        if (wait_for_work(worker_args->efd) < 0)
        {
            atomic_store(&ctx->worker_error, 1);
            atomic_store(&ctx->running, 0);
            break;
        }
    }

    return NULL;
}

static void cleanup_started_workers(AppContext *ctx, int started)
{
    atomic_store(&ctx->running, 0);
    ctx->num_workers = started;

    for (int i = 0; i < started; i++)
        (void)notify_worker(ctx, i);

    for (int i = 0; i < started; i++)
    {
        (void)pthread_join(threads_storage[i], NULL);
        close(ctx->efd[i]);
        ctx->efd[i] = -1;
    }

    ctx->num_workers = 0;
}

int start_workers(AppContext *ctx, int num_threads)
{
    if (!ctx || num_threads < 1 || num_threads > MAX_WORKERS)
    {
        errno = EINVAL;
        return -1;
    }

    atomic_store(&ctx->active_workers, 0);
    atomic_store(&ctx->worker_error, 0);
    ctx->num_workers = 0;
    for (int i = 0; i < MAX_WORKERS; i++)
        ctx->efd[i] = -1;

    for (int i = 0; i < num_threads; i++)
    {
        int event_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (event_fd < 0)
        {
            int saved_errno = errno;
            cleanup_started_workers(ctx, i);
            errno = saved_errno;
            return -1;
        }

        ctx->efd[i] = event_fd;
        args_storage[i].queue = &ctx->queues[i];
        args_storage[i].ctx = ctx;
        args_storage[i].efd = event_fd;

        int create_error = pthread_create(&threads_storage[i], NULL,
                                          worker, &args_storage[i]);
        if (create_error != 0)
        {
            close(event_fd);
            ctx->efd[i] = -1;
            cleanup_started_workers(ctx, i);
            errno = create_error;
            return -1;
        }

        ctx->num_workers = i + 1;
    }

    return 0;
}

int join_workers(AppContext *ctx)
{
    if (!ctx)
    {
        errno = EINVAL;
        return -1;
    }

    atomic_store(&ctx->running, 0);
    int status = 0;
    int saved_errno = 0;

    for (int i = 0; i < ctx->num_workers; i++)
    {
        if (notify_worker(ctx, i) != 0 && status == 0)
        {
            status = -1;
            saved_errno = errno;
        }
    }

    for (int i = 0; i < ctx->num_workers; i++)
    {
        int join_error = pthread_join(threads_storage[i], NULL);
        if (join_error != 0 && status == 0)
        {
            status = -1;
            saved_errno = join_error;
        }

        if (close(ctx->efd[i]) != 0 && status == 0)
        {
            status = -1;
            saved_errno = errno;
        }
        ctx->efd[i] = -1;
    }

    ctx->num_workers = 0;
    if (atomic_load(&ctx->worker_error) != 0 && status == 0)
    {
        status = -1;
        saved_errno = EIO;
    }

    if (status != 0)
        errno = saved_errno;
    return status;
}
