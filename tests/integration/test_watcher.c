#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <poll.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/stat.h>

#include "classifier/classifier.h"
#include "core/context.h"
#include "core/thread_pool.h"
#include "utils/hashmap.h"
#include "watcher/watcher.h"

static AppContext ctx;
static HashEntry entries[64];
static char test_root[128];

static void *watcher_entry(void *root)
{
    int result = start_watcher((const char *)root, &ctx);
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
    /* All terminal watch invalidations stop processing before a following
     * actionable record. Exercise each flag and overflow precedence. */
    const uint32_t terminal_masks[] = {IN_IGNORED, IN_DELETE_SELF, IN_MOVE_SELF, IN_UNMOUNT};
    char batch[2 * sizeof(struct inotify_event) + 16] = {0};
    struct inotify_event following = {.wd = 1, .mask = IN_CREATE, .len = 16};
    memcpy(batch + sizeof(header), &following, sizeof(following));
    memcpy(batch + 2 * sizeof(header), "later.txt", sizeof("later.txt"));
    for (size_t i = 0; i < sizeof(terminal_masks) / sizeof(terminal_masks[0]); ++i)
    {
        header.mask = terminal_masks[i];
        memcpy(record, &header, sizeof(header));
        expect_bad_record(record, sizeof(header), ESTALE);
        memcpy(batch, &header, sizeof(header));
        expect_bad_record(batch, sizeof(batch), ESTALE);
        header.mask |= IN_Q_OVERFLOW;
        memcpy(batch, &header, sizeof(header));
        expect_bad_record(batch, sizeof(batch), ENOSPC);
    }
    header.mask = IN_Q_OVERFLOW;
    memcpy(batch, &header, sizeof(header));
    expect_bad_record(batch, sizeof(batch), ENOSPC);
    assert(atomic_load(&ctx.running) == 1);
    assert(atomic_load(&ctx.worker_error) == 0);

    /* A pending stop must prevent decoding and handing off buffered work. */
    header.mask = IN_CREATE;
    header.len = 16;
    memcpy(record, &header, sizeof(header));
    memset(name, 0, 16);
    memcpy(name, "pending.txt", sizeof("pending.txt"));
    atomic_store(&ctx.stop_signal, 1);
    expect_bad_record(record, sizeof(header) + 16, ECANCELED);
    /* Stop takes precedence over a not-yet-decoded record. */
    expect_bad_record(record, sizeof(header) - 1, ECANCELED);
    atomic_store(&ctx.stop_signal, 0);

    header.mask = IN_CREATE | IN_ISDIR;
    header.len = 16;
    memcpy(record, &header, sizeof(header));
    memset(name, 0, 16);
    memcpy(name, "folder", sizeof("folder"));
    assert(fc_test_process_watcher_buffer(record, sizeof(header) + 16,
                                          "/unused", &ctx) == 0);
    assert(atomic_load(&ctx.metrics.events_seen) == 0);
}

/* Real kernel invalidation, not a synthetic parser record. Independent raw
 * watch illustrates that a watch can remain attached to the renamed object. */
static void test_real_root_invalidation(void)
{
    char parent[] = "/tmp/fc-watch-invalidation-XXXXXX";
    assert(mkdtemp(parent) != NULL);
    char root[256], relocated[256], old_file[256], new_file[256];
    int n = snprintf(root, sizeof root, "%s/root", parent);
    assert(n > 0 && (size_t)n < sizeof root);
    n = snprintf(relocated, sizeof relocated, "%s/relocated", parent);
    assert(n > 0 && (size_t)n < sizeof relocated);
    n = snprintf(old_file, sizeof old_file, "%s/original.txt", relocated);
    assert(n > 0 && (size_t)n < sizeof old_file);
    n = snprintf(new_file, sizeof new_file, "%s/replacement.txt", root);
    assert(n > 0 && (size_t)n < sizeof new_file);
    assert(mkdir(root, 0700) == 0);
    int raw = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    assert(raw >= 0);
    int wd = inotify_add_watch(raw, root, IN_CREATE | IN_MOVED_TO |
                               IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT |
                               IN_ONLYDIR | IN_DONT_FOLLOW);
    assert(wd >= 0);

    /* Workers have already joined; no file events should reach production. */
    unsigned long long seen = atomic_load(&ctx.metrics.events_seen);
    unsigned long long queued = atomic_load(&ctx.metrics.events_enqueued);
    unsigned long long moves = atomic_load(&ctx.metrics.dry_run_moves);
    atomic_store(&ctx.watcher_error, 0);
    atomic_store(&ctx.stop_signal, 0);
    atomic_store(&ctx.metrics.watcher_idle_polls, 0);
    atomic_store(&ctx.running, 1);
    pthread_t watcher;
    assert(pthread_create(&watcher, NULL, watcher_entry, root) == 0);
    for (int i = 0; i < 5000 &&
         atomic_load(&ctx.metrics.watcher_idle_polls) == 0 &&
         atomic_load(&ctx.watcher_error) == 0; ++i)
        sleep_milliseconds(1);
    assert(atomic_load(&ctx.watcher_error) == 0);
    assert(atomic_load(&ctx.metrics.watcher_idle_polls) > 0);

    assert(rename(root, relocated) == 0);
    for (int i = 0; i < 5000 && atomic_load(&ctx.watcher_error) == 0; ++i)
        sleep_milliseconds(1);
    assert(atomic_load(&ctx.watcher_error) == ESTALE);
    assert(pthread_join(watcher, NULL) == 0);
    atomic_store(&ctx.running, 0);

    /* Recreate the former name, then generate an event in each object.
     * The app has stopped; the raw watch should see only the old object. */
    assert(mkdir(root, 0700) == 0);
    int fd = open(new_file, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    assert(fd >= 0 && close(fd) == 0);
    fd = open(old_file, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    assert(fd >= 0 && close(fd) == 0);
    struct pollfd ready = {.fd = raw, .events = POLLIN};
    int polled;
    do { polled = poll(&ready, 1, 1000); } while (polled < 0 && errno == EINTR);
    assert(polled == 1 && (ready.revents & POLLIN));
    char buffer[4096];
    size_t records = 0;
    for (;;)
    {
        ssize_t count = read(raw, buffer, sizeof buffer);
        if (count < 0 && errno == EINTR)
            continue;
        if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            break;
        assert(count > 0);
        size_t length = (size_t)count, offset = 0;
        while (offset < length)
        {
            struct inotify_event event;
            assert(length - offset >= sizeof event);
            memcpy(&event, buffer + offset, sizeof event);
            assert((size_t)event.len <= length - offset - sizeof event);
            assert(event.wd == wd);
            if (records == 0)
            {
                assert(event.mask == IN_MOVE_SELF && event.len == 0);
            }
            else
            {
                assert(records == 1 && event.mask == IN_CREATE && event.len > 0);
                const char *name = buffer + offset + sizeof event;
                assert(strnlen(name, event.len) < event.len);
                assert(strcmp(name, "original.txt") == 0);
            }
            ++records;
            offset += sizeof event + (size_t)event.len;
        }
    }
    assert(records == 2);
    assert(atomic_load(&ctx.metrics.events_seen) == seen);
    assert(atomic_load(&ctx.metrics.events_enqueued) == queued);
    assert(atomic_load(&ctx.metrics.dry_run_moves) == moves);
    assert(access(old_file, F_OK) == 0 && access(new_file, F_OK) == 0);
    assert(inotify_rm_watch(raw, wd) == 0);
    assert(close(raw) == 0);
    assert(unlink(old_file) == 0 && unlink(new_file) == 0);
    assert(rmdir(relocated) == 0 && rmdir(root) == 0 && rmdir(parent) == 0);
    puts("watcher_real_root_invalidation=PASS; production ESTALE; raw original identity");
}

/* Capture actual kernel records before draining, independently verify their
 * names and masks, then submit the same bytes to the production parser.
 * The real worker runs in dry-run mode; no production watcher read-loop claim. */
enum { BATCH_FILES = 64 };
static void test_real_event_batch(void)
{
    char parent[] = "/tmp/fc-watch-batch-XXXXXX";
    assert(mkdtemp(parent) != NULL);
    char root[256], staging[256], folder[256], names[BATCH_FILES][32];
    char paths[BATCH_FILES][256];
    int n = snprintf(root, sizeof root, "%s/root", parent);
    assert(n > 0 && (size_t)n < sizeof root);
    n = snprintf(staging, sizeof staging, "%s/staging", parent);
    assert(n > 0 && (size_t)n < sizeof staging);
    n = snprintf(folder, sizeof folder, "%s/ignored-directory", root);
    assert(n > 0 && (size_t)n < sizeof folder);
    assert(mkdir(root, 0700) == 0 && mkdir(staging, 0700) == 0);
    int raw = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    assert(raw >= 0);
    int wd = inotify_add_watch(raw, root, IN_CREATE | IN_MOVED_TO |
                               IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT |
                               IN_ONLYDIR | IN_DONT_FOLLOW);
    assert(wd >= 0);

    for (size_t i = 0; i < BATCH_FILES; ++i)
    {
        n = snprintf(names[i], sizeof names[i], "batch-%02zu.txt", i);
        assert(n > 0 && (size_t)n < sizeof names[i]);
        n = snprintf(paths[i], sizeof paths[i], "%s/%s", root, names[i]);
        assert(n > 0 && (size_t)n < sizeof paths[i]);
        char source[256];
        n = snprintf(source, sizeof source, "%s/%s", staging, names[i]);
        assert(n > 0 && (size_t)n < sizeof source);
        int fd = open(i % 2 ? source : paths[i],
                      O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        assert(fd >= 0);
        assert(write(fd, "keep", 4) == 4);
        assert(close(fd) == 0);
        if (i % 2)
            assert(rename(source, paths[i]) == 0);
    }
    assert(mkdir(folder, 0700) == 0);

    unsigned long before_seen = atomic_load(&ctx.metrics.events_seen);
    unsigned long before_queued = atomic_load(&ctx.metrics.events_enqueued);
    unsigned long before_watcher = atomic_load(&ctx.metrics.watcher_events);
    unsigned long before_dry = atomic_load(&ctx.metrics.dry_run_moves);
    unsigned long before_moved = atomic_load(&ctx.metrics.files_moved);
    unsigned long before_failed = atomic_load(&ctx.metrics.move_failures);
    atomic_store(&ctx.running, 1);
    atomic_store(&ctx.stop_signal, 0);
    atomic_store(&ctx.watcher_error, 0);
    assert(atomic_load(&ctx.dry_run) == 1);
    lf_init(&ctx.queues[0]);
    assert(start_workers(&ctx, 1) == 0);

    unsigned char seen[BATCH_FILES] = {0};
    size_t files = 0, directories = 0, reads = 0, max_records = 0;
    char buffer[4096];
    for (;;)
    {
        ssize_t count = read(raw, buffer, sizeof buffer);
        if (count < 0 && errno == EINTR)
            continue;
        if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            break;
        assert(count > 0);
        ++reads;
        size_t offset = 0, length = (size_t)count, records = 0;
        while (offset < length)
        {
            struct inotify_event event;
            assert(length - offset >= sizeof event);
            memcpy(&event, buffer + offset, sizeof event);
            assert((size_t)event.len <= length - offset - sizeof event);
            assert(event.wd == wd && event.len > 0);
            const char *name = buffer + offset + sizeof event;
            assert(strnlen(name, event.len) < event.len);
            if (event.mask == (IN_CREATE | IN_ISDIR))
            {
                assert(strcmp(name, "ignored-directory") == 0);
                ++directories;
            }
            else
            {
                size_t i = 0;
                while (i < BATCH_FILES && strcmp(name, names[i]) != 0)
                    ++i;
                assert(i < BATCH_FILES && seen[i] == 0);
                assert(event.mask == (i % 2 ? IN_MOVED_TO : IN_CREATE));
                seen[i] = 1;
                ++files;
            }
            ++records;
            offset += sizeof event + (size_t)event.len;
        }
        if (records > max_records)
            max_records = records;
        assert(fc_test_process_watcher_buffer(buffer, length, root, &ctx) == 0);
    }
    assert(reads > 0 && max_records > 1);
    assert(files == BATCH_FILES && directories == 1);
    for (size_t i = 0; i < BATCH_FILES; ++i)
        assert(seen[i] == 1);
    assert(join_workers(&ctx) == 0);
    assert(atomic_load(&ctx.metrics.events_seen) == before_seen + BATCH_FILES);
    assert(atomic_load(&ctx.metrics.events_enqueued) == before_queued + BATCH_FILES);
    assert(atomic_load(&ctx.metrics.watcher_events) == before_watcher + BATCH_FILES);
    assert(atomic_load(&ctx.metrics.dry_run_moves) == before_dry + BATCH_FILES);
    assert(atomic_load(&ctx.metrics.files_moved) == before_moved);
    assert(atomic_load(&ctx.metrics.move_failures) == before_failed);
    assert(atomic_load(&ctx.watcher_error) == 0 && lf_is_empty(&ctx.queues[0]));
    assert(inotify_rm_watch(raw, wd) == 0 && close(raw) == 0);
    for (size_t i = 0; i < BATCH_FILES; ++i)
    {
        char bytes[5] = {0};
        int fd = open(paths[i], O_RDONLY | O_CLOEXEC);
        assert(fd >= 0 && read(fd, bytes, sizeof bytes) == 4);
        assert(memcmp(bytes, "keep", 4) == 0 && close(fd) == 0);
        assert(unlink(paths[i]) == 0);
    }
    assert(rmdir(folder) == 0 && rmdir(root) == 0);
    assert(rmdir(staging) == 0 && rmdir(parent) == 0);
    printf("watcher_batch_files=%d ignored_dirs=%zu reads=%zu max_records=%zu dry_run=PASS\n",
           BATCH_FILES, directories, reads, max_records);
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

    (void)snprintf(test_root, sizeof(test_root), "/tmp/fc-watcher-XXXXXX");
    assert(mkdtemp(test_root) != NULL);
    char missing[256];
    (void)snprintf(missing, sizeof(missing), "%s/missing", test_root);
    errno = 0;
    assert(start_watcher(missing, &ctx) == -1);
    assert(errno == ENOENT);

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
    char suffixed_root[256];
    for (int separators = 1; separators <= 3; ++separators)
    {
        int n = snprintf(suffixed_root, sizeof(suffixed_root), "%s%.*s",
                         link, separators, "///");
        assert(n > 0 && (size_t)n < sizeof(suffixed_root));
        errno = 0;
        assert(start_watcher(suffixed_root, &ctx) == -1);
        assert(errno == ENOTDIR || errno == ELOOP);

        n = snprintf(suffixed_root, sizeof(suffixed_root), "%s%.*s",
                     test_root, separators, "///");
        assert(n > 0 && (size_t)n < sizeof(suffixed_root));
        assert(start_watcher(suffixed_root, &ctx) == 0);
    }
    assert(atomic_load(&ctx.metrics.events_seen) == 0);
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
    assert(pthread_create(&watcher, NULL, watcher_entry, suffixed_root) == 0);
    /* An observed idle timeout proves registration and poll-loop entry.
     * Avoid a fixed startup sleep that can race a slow CI scheduler. */
    for (int i = 0; i < 5000 &&
         atomic_load(&ctx.metrics.watcher_idle_polls) == 0 &&
         atomic_load(&ctx.watcher_error) == 0; ++i)
        sleep_milliseconds(1);
    assert(atomic_load(&ctx.watcher_error) == 0);
    assert(atomic_load(&ctx.metrics.watcher_idle_polls) > 0);
    assert(atomic_load(&ctx.metrics.events_seen) == 0);
    assert(atomic_load(&ctx.metrics.events_enqueued) == 0);

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

    test_real_root_invalidation();
    test_real_event_batch();

    puts("watcher integration tests passed");
    return 0;
}
