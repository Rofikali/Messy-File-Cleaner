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
#include <sys/inotify.h>

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

static void test_dispatch_bounds(void)
{
    const int invalid_counts[] = {-1, 0, MAX_WORKERS + 1};
    for (size_t i = 0; i < sizeof(invalid_counts) / sizeof(invalid_counts[0]); ++i)
    {
        ctx.num_workers = invalid_counts[i];
        errno = 0;
        assert(dispatch(&ctx, "fixture.jpg") == -1);
        assert(errno == EINVAL);
        assert(ctx.num_workers == invalid_counts[i]);
    }
    ctx.num_workers = 1;
    errno = 0;
    assert(dispatch(NULL, "fixture.jpg") == -1 && errno == EINVAL);
    errno = 0;
    assert(dispatch(&ctx, NULL) == -1 && errno == EINVAL);
    errno = 0;
    assert(dispatch(&ctx, "") == -1 && errno == EINVAL);

    for (int workers = 1; workers <= MAX_WORKERS; ++workers)
    {
        ctx.num_workers = workers;
        for (int i = 0; i < 128; ++i)
        {
            char path[64];
            (void)snprintf(path, sizeof(path), "fixture-%d.jpg", i);
            int index = dispatch(&ctx, path);
            assert(index >= 0 && index < workers);
            assert(dispatch(&ctx, path) == index);
        }
    }
    ctx.num_workers = 0;
}

static void expect_bad_record(const char *buffer, size_t length, int error)
{
    errno = 0;
    assert(fc_test_process_watcher_buffer(buffer, length, "/unused", &ctx) == -1);
    assert(errno == error);
    assert(atomic_load(&ctx.metrics.events_enqueued) == 0);
    assert(atomic_load(&ctx.metrics.events_seen) == 0);
}

static void test_event_records(void)
{
    /* Intentionally offset the buffer: parser must copy the header. */
    _Alignas(struct inotify_event) char storage[1 + sizeof(struct inotify_event) + 16] = {0};
    char *record = storage + 1;
    struct inotify_event header = {.wd = 1, .mask = IN_CREATE, .len = 16};
    memcpy(record, &header, sizeof(header));
    expect_bad_record(record, sizeof(header) - 1, EBADMSG);
    expect_bad_record(record, sizeof(header) + 15, EBADMSG);

    char *name = record + sizeof(header);
    memset(name, 'a', 16);
    expect_bad_record(record, sizeof(header) + 16, EBADMSG);
    memset(name, 0, 16);
    expect_bad_record(record, sizeof(header) + 16, EBADMSG);
    memcpy(name, "../outside", sizeof("../outside"));
    expect_bad_record(record, sizeof(header) + 16, EBADMSG);
    memcpy(name, "..", sizeof(".."));
    expect_bad_record(record, sizeof(header) + 16, EBADMSG);

    header.len = 0;
    memcpy(record, &header, sizeof(header));
    expect_bad_record(record, sizeof(header), EBADMSG);
    header.mask = IN_Q_OVERFLOW;
    memcpy(record, &header, sizeof(header));
    expect_bad_record(record, sizeof(header), ENOSPC);
    header.mask = IN_MOVE_SELF;
    memcpy(record, &header, sizeof(header));
    expect_bad_record(record, sizeof(header), ESTALE);

    header.mask = IN_CREATE | IN_ISDIR;
    header.len = 16;
    memcpy(record, &header, sizeof(header));
    memset(name, 0, 16);
    memcpy(name, "folder", sizeof("folder"));
    assert(fc_test_process_watcher_buffer(record, sizeof(header) + 16,
                                          "/unused", &ctx) == 0);
    assert(atomic_load(&ctx.metrics.events_seen) == 0);
}

int main(void)
{
    test_dispatch_bounds();
    hashmap_init(&ctx.cfg.ext_map, entries, 64);
    classifier_init(&ctx.classifier, &ctx.cfg.ext_map, "Others/");

    atomic_init(&ctx.running, 1);
    atomic_init(&ctx.active_workers, 0);
    atomic_init(&ctx.scanning_done, 0);
    atomic_init(&ctx.worker_error, 0);
    atomic_init(&ctx.watcher_error, 0);
    atomic_init(&ctx.stop_signal, 0);
    atomic_init(&ctx.dry_run, 1);
    atomic_init(&ctx.metrics.dry_run_moves, 0);
    atomic_init(&ctx.metrics.files_moved, 0);
    atomic_init(&ctx.metrics.move_failures, 0);
    atomic_init(&ctx.metrics.events_seen, 0);
    atomic_init(&ctx.metrics.events_enqueued, 0);
    atomic_init(&ctx.metrics.queue_backpressure_waits, 0);
    atomic_init(&ctx.metrics.watcher_events, 0);
    atomic_init(&ctx.metrics.watcher_idle_polls, 0);
    test_event_records();

    char missing[] = "/tmp/fc-watcher-missing-directory";
    errno = 0;
    assert(start_watcher(missing, &ctx) == -1);
    assert(errno == ENOENT);

    (void)snprintf(test_root, sizeof(test_root), "/tmp/fc-watcher-XXXXXX");
    assert(mkdtemp(test_root) != NULL);

    /* Exercise direct watcher registration without starting worker threads.
     * With running cleared, even an incorrectly accepted watch cannot hang. */
    char regular[256], link[256];
    (void)snprintf(regular, sizeof(regular), "%s/not-directory", test_root);
    (void)snprintf(link, sizeof(link), "%s-link", test_root);
    int regular_fd = open(regular, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    assert(regular_fd >= 0);
    assert(write(regular_fd, "keep", 4) == 4);
    assert(close(regular_fd) == 0);
    assert(symlink(test_root, link) == 0);

    atomic_store(&ctx.running, 0);
    errno = 0;
    assert(start_watcher(regular, &ctx) == -1);
    assert(errno == ENOTDIR);
    errno = 0;
    assert(start_watcher(link, &ctx) == -1);
    assert(errno == ENOTDIR);
    assert(start_watcher(test_root, &ctx) == 0);
    assert(atomic_load(&ctx.metrics.events_enqueued) == 0);
    char preserved[4];
    regular_fd = open(regular, O_RDONLY | O_CLOEXEC);
    assert(regular_fd >= 0);
    assert(read(regular_fd, preserved, sizeof(preserved)) == (ssize_t)sizeof(preserved));
    assert(memcmp(preserved, "keep", sizeof(preserved)) == 0);
    assert(close(regular_fd) == 0);
    assert(unlink(link) == 0);
    assert(unlink(regular) == 0);
    atomic_store(&ctx.running, 1);

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
