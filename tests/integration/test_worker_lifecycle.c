#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdint.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "classifier/classifier.h"
#include "core/context.h"
#include "core/thread_pool.h"
#include "utils/hashmap.h"

typedef struct
{
    AppContext *ctx;
    atomic_int started;
    atomic_int finished;
    int result;
    int error_code;
} JoinArgs;

static void *join_workers_entry(void *opaque)
{
    JoinArgs *args = opaque;
    atomic_store(&args->started, 1);
    errno = 0;
    args->result = join_workers(args->ctx);
    args->error_code = errno;
    atomic_store(&args->finished, 1);
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
    /* AppContext embeds large per-worker queues; keep it out of the small test stack. */
    static AppContext ctx;
    static HashEntry entries[64];
    hashmap_init(&ctx.cfg.ext_map, entries, 64);
    classifier_init(&ctx.classifier, &ctx.cfg.ext_map, "Others/");

    atomic_init(&ctx.running, 1);
    atomic_init(&ctx.active_workers, 0);
    atomic_init(&ctx.scanning_done, 0);
    atomic_init(&ctx.dry_run, 1);
    atomic_init(&ctx.worker_error, 0);
    atomic_init(&ctx.metrics.dry_run_moves, 0);
    atomic_init(&ctx.metrics.files_moved, 0);
    atomic_init(&ctx.metrics.move_failures, 0);

    /* Invalid metadata must fail before indexing arrays or stopping the pool.
     * No worker or descriptor is live in these negative cases. */
    const int invalid_counts[] = {-1, MAX_WORKERS + 1};
    for (size_t i = 0; i < sizeof(invalid_counts) / sizeof(invalid_counts[0]); ++i)
    {
        ctx.num_workers = invalid_counts[i];
        ctx.efd[0] = -1;
        errno = 0;
        assert(notify_worker(&ctx, MAX_WORKERS) == -1 && errno == EINVAL);
        errno = 0;
        assert(notify_worker(&ctx, 0) == -1 && errno == EINVAL);
        errno = 0;
        assert(join_workers(&ctx) == -1 && errno == EINVAL);
        assert(ctx.num_workers == invalid_counts[i]);
        assert(atomic_load(&ctx.running) == 1);
        assert(ctx.efd[0] == -1);
    }
    ctx.num_workers = 0;
    errno = 0;
    assert(notify_worker(NULL, 0) == -1 && errno == EINVAL);
    errno = 0;
    assert(join_workers(NULL) == -1 && errno == EINVAL);
    errno = 0;
    assert(notify_worker(&ctx, 0) == -1 && errno == EINVAL);
    assert(join_workers(&ctx) == 0);
    assert(atomic_load(&ctx.running) == 0);

    atomic_store(&ctx.running, 1);
    fc_test_fail_eventfd_after(0);
    errno = 0;
    assert(start_workers(&ctx, 2) == -1);
    assert(errno == EMFILE);
    assert(ctx.num_workers == 0);
    for (int i = 0; i < MAX_WORKERS; i++)
        assert(ctx.efd[i] == -1);

    atomic_store(&ctx.running, 1);
    fc_test_fail_pthread_create_after(1);
    errno = 0;
    assert(start_workers(&ctx, 3) == -1);
    assert(errno == EAGAIN);
    assert(ctx.num_workers == 0);
    assert(atomic_load(&ctx.running) == 0);
    for (int i = 0; i < MAX_WORKERS; i++)
        assert(ctx.efd[i] == -1);

    atomic_store(&ctx.running, 1);
    errno = 0;
    assert(start_workers(&ctx, 0) == -1);
    assert(errno == EINVAL);
    errno = 0;
    assert(start_workers(&ctx, MAX_WORKERS + 1) == -1);
    assert(errno == EINVAL);

    lf_init(&ctx.queues[0]);
    lf_init(&ctx.queues[1]);
    assert(start_workers(&ctx, 2) == 0);
    assert(ctx.num_workers == 2);
    int original_fd = ctx.efd[0];
    static AppContext other;
    atomic_init(&other.running, 1);
    other.num_workers = 0;
    errno = 0;
    assert(start_workers(&ctx, 1) == -1 && errno == EBUSY);
    errno = 0;
    assert(start_workers(&other, 1) == -1 && errno == EBUSY);
    errno = 0;
    assert(join_workers(&other) == -1 && errno == EBUSY);
    other.num_workers = 1;
    other.efd[0] = original_fd;
    errno = 0;
    assert(notify_worker(&other, 0) == -1 && errno == EINVAL);
    errno = 0;
    assert(join_workers(&other) == -1 && errno == EBUSY);
    other.num_workers = 0;
    other.efd[0] = -1;
    assert(ctx.num_workers == 2 && ctx.efd[0] == original_fd);
    assert(atomic_load(&ctx.running) == 1 && atomic_load(&other.running) == 1);
    errno = 0;
    assert(notify_worker(&ctx, 2) == -1);
    assert(errno == EINVAL);
    errno = 0;
    assert(notify_worker(&ctx, -1) == -1 && errno == EINVAL);
    errno = 0;
    assert(notify_worker(&ctx, MAX_WORKERS) == -1 && errno == EINVAL);

    /* A plausible but false live count must not permit partial join or
     * notification using inconsistent pool metadata. Workers do not read
     * this count; no producer is active during these serialized calls. */
    int second_fd = ctx.efd[1];
    const int mismatched_counts[] = {1, 3};
    for (size_t i = 0; i < sizeof(mismatched_counts) / sizeof(mismatched_counts[0]); ++i)
    {
        ctx.num_workers = mismatched_counts[i];
        errno = 0;
        assert(notify_worker(&ctx, 0) == -1 && errno == EINVAL);
        errno = 0;
        assert(join_workers(&ctx) == -1 && errno == EBUSY);
        assert(atomic_load(&ctx.running) == 1);
        assert(ctx.num_workers == mismatched_counts[i]);
        assert(ctx.efd[0] == original_fd && ctx.efd[1] == second_fd);
    }
    ctx.num_workers = 2;

    /* Queue many items before notifying: one eventfd wake drains the queue. */
    for (int i = 0; i < 32; i++)
    {
        FileEvent event;
        memset(&event, 0, sizeof(event));
        (void)snprintf(event.path, sizeof(event.path),
                       "/tmp/worker-lifecycle-%d.txt", i);
        event.type = FILE_FOUND;
        assert(lf_enqueue(&ctx.queues[i % 2], &event) == 1);
    }

    assert(notify_worker(&ctx, 0) == 0);
    assert(notify_worker(&ctx, 1) == 0);
    assert(join_workers(&ctx) == 0);
    assert(ctx.num_workers == 0);
    assert(atomic_load(&ctx.worker_error) == 0);
    assert(atomic_load(&ctx.metrics.dry_run_moves) == 32);
    assert(atomic_load(&ctx.active_workers) == 0);

    /* Ownership is released after successful join; a second context can start. */
    atomic_init(&other.active_workers, 0);
    atomic_init(&other.worker_error, 0);
    lf_init(&other.queues[0]);
    assert(start_workers(&other, 1) == 0);
    assert(join_workers(&other) == 0);
    assert(other.num_workers == 0);

    /* Empty workers must wake and join cleanly too. */
    atomic_store(&ctx.running, 1);
    assert(start_workers(&ctx, 1) == 0);
    assert(join_workers(&ctx) == 0);
    assert(atomic_load(&ctx.worker_error) == 0);

    /* A failed eventfd notification must be reported; shutdown must still join. */
    atomic_store(&ctx.running, 1);
    lf_init(&ctx.queues[0]);
    assert(start_workers(&ctx, 1) == 0);
    fc_test_fail_notify_after(0);
    errno = 0;
    assert(notify_worker(&ctx, 0) == -1);
    assert(errno == EIO);
    assert(join_workers(&ctx) == 0);
    assert(ctx.num_workers == 0);
    assert(atomic_load(&ctx.worker_error) == 0);

    /* A notification failure inside join must be returned even though
     * every worker is joined and ownership is released. */
    atomic_store(&ctx.running, 1);
    lf_init(&ctx.queues[0]);
    lf_init(&ctx.queues[1]);
    assert(start_workers(&ctx, 2) == 0);
    fc_test_fail_notify_after(0);
    errno = 0;
    assert(join_workers(&ctx) == -1 && errno == EIO);
    assert(ctx.num_workers == 0);
    assert(ctx.efd[0] == -1 && ctx.efd[1] == -1);
    assert(atomic_load(&ctx.running) == 0);
    assert(atomic_load(&ctx.active_workers) == 0);
    assert(atomic_load(&ctx.worker_error) == 0);
    atomic_store(&ctx.running, 1);
    assert(start_workers(&ctx, 1) == 0);
    assert(join_workers(&ctx) == 0);

    /*
     * Hold a worker after dequeue and active_workers++.
     * Shutdown must wait for in-flight processing rather than return early.
     */
    atomic_store(&ctx.running, 1);
    lf_init(&ctx.queues[0]);
    fc_test_pause_worker_before_process();
    assert(start_workers(&ctx, 1) == 0);

    FileEvent active_event;
    memset(&active_event, 0, sizeof(active_event));
    (void)snprintf(active_event.path, sizeof(active_event.path),
                   "/tmp/worker-active-shutdown.txt");
    active_event.type = FILE_FOUND;
    assert(lf_enqueue(&ctx.queues[0], &active_event) == 1);
    assert(notify_worker(&ctx, 0) == 0);
    assert(fc_test_wait_for_worker_pause(2000) == 0);
    assert(atomic_load(&ctx.active_workers) == 1);

    JoinArgs join_args = {.ctx = &ctx, .result = -2, .error_code = 0};
    atomic_init(&join_args.started, 0);
    atomic_init(&join_args.finished, 0);
    pthread_t shutdown_thread;
    assert(pthread_create(&shutdown_thread, NULL, join_workers_entry,
                          &join_args) == 0);

    for (int i = 0; i < 2000 &&
         (!atomic_load(&join_args.started) || atomic_load(&ctx.running) != 0); i++)
        sleep_milliseconds(1);
    assert(atomic_load(&join_args.started) == 1);
    assert(atomic_load(&ctx.running) == 0);
    sleep_milliseconds(50);
    assert(atomic_load(&join_args.finished) == 0);

    fc_test_release_worker();
    assert(pthread_join(shutdown_thread, NULL) == 0);
    assert(join_args.result == 0);
    assert(join_args.error_code == 0);
    assert(atomic_load(&join_args.finished) == 1);
    assert(atomic_load(&ctx.active_workers) == 0);
    assert(atomic_load(&ctx.metrics.dry_run_moves) == 33);
    assert(ctx.num_workers == 0);

    /* A malformed event must never reach classification, diagnostics or a mover.
       Preload disposable events before starting the consumer, then check drain. */
    atomic_store(&ctx.running, 1);
    lf_init(&ctx.queues[0]);
    FileEvent empty_event = {0};
    FileEvent unterminated_event = {0};
    memset(unterminated_event.path, 'x', sizeof(unterminated_event.path));
    assert(lf_enqueue(&ctx.queues[0], &empty_event) == 1);
    assert(lf_enqueue(&ctx.queues[0], &unterminated_event) == 1);
    assert(lf_enqueue(&ctx.queues[0], &active_event) == 1);
    assert(start_workers(&ctx, 1) == 0);
    errno = 0;
    assert(join_workers(&ctx) == -1 && errno == EIO);
    assert(atomic_load(&ctx.worker_error) == 1);
    assert(atomic_load(&ctx.metrics.move_failures) == 2);
    assert(atomic_load(&ctx.metrics.dry_run_moves) == 34);
    assert(atomic_load(&ctx.active_workers) == 0);
    assert(lf_is_empty(&ctx.queues[0]));

    /* Invalid classification must also fail before dry-run or move handling. */
    const char *invalid_folders[] = {NULL, ""};
    for (size_t i = 0; i < sizeof(invalid_folders) / sizeof(invalid_folders[0]); ++i)
    {
        classifier_init(&ctx.classifier, &ctx.cfg.ext_map, invalid_folders[i]);
        atomic_store(&ctx.running, 1);
        lf_init(&ctx.queues[0]);
        assert(lf_enqueue(&ctx.queues[0], &active_event) == 1);
        assert(start_workers(&ctx, 1) == 0);
        errno = 0;
        assert(join_workers(&ctx) == -1 && errno == EIO);
        assert(atomic_load(&ctx.worker_error) == 1);
        assert(atomic_load(&ctx.metrics.move_failures) == 3 + i);
        assert(atomic_load(&ctx.metrics.dry_run_moves) == 34);
    }
    classifier_init(&ctx.classifier, &ctx.cfg.ext_map, "Others/");

    /* Real eventfd saturation, not an injected errno. Hold the sole
     * consumer at the existing test gate before filling its counter. */
    atomic_store(&ctx.running, 1);
    lf_init(&ctx.queues[0]);
    const uint64_t moves_before = atomic_load(&ctx.metrics.dry_run_moves);
    fc_test_pause_worker_before_process();
    assert(lf_enqueue(&ctx.queues[0], &active_event) == 1);
    assert(start_workers(&ctx, 1) == 0);
    assert(fc_test_wait_for_worker_pause(2000) == 0);
    assert(atomic_load(&ctx.active_workers) == 1);

    const int saturated_fd = ctx.efd[0];
    const uint64_t limit = UINT64_MAX - UINT64_C(1);
    const uint64_t increment = 1;
    assert(write(saturated_fd, &limit, sizeof(limit)) == (ssize_t)sizeof(limit));
    errno = 0;
    assert(write(saturated_fd, &increment, sizeof(increment)) == -1 &&
           errno == EAGAIN);
    struct pollfd saturated = { .fd = saturated_fd, .events = POLLIN };
    assert(poll(&saturated, 1, 0) == 1 && (saturated.revents & POLLIN));
    assert(lf_enqueue(&ctx.queues[0], &active_event) == 1);
    assert(notify_worker(&ctx, 0) == 0);

    /* The counter remains saturated and the queued item remains pending. */
    errno = 0;
    assert(write(saturated_fd, &increment, sizeof(increment)) == -1 &&
           errno == EAGAIN);
    assert(!lf_is_empty(&ctx.queues[0]));
    fc_test_release_worker();

    /* Do not request shutdown until the live worker has processed both
     * items and consumed the saturated counter. Observe readiness only;
     * the test thread must not read or reset the worker's eventfd. */
    int drained_live = 0;
    for (int i = 0; i < 2000; ++i) {
        assert(atomic_load(&ctx.running) == 1);
        assert(atomic_load(&ctx.worker_error) == 0);
        saturated.revents = 0;
        int ready = poll(&saturated, 1, 0);
        assert(ready >= 0);
        assert(!(saturated.revents & (POLLERR | POLLHUP | POLLNVAL)));
        if (ready == 0 &&
            atomic_load(&ctx.metrics.dry_run_moves) == moves_before + 2 &&
            atomic_load(&ctx.active_workers) == 0) {
            drained_live = 1;
            break;
        }
        sleep_milliseconds(1);
    }
    assert(drained_live == 1);
    assert(lf_is_empty(&ctx.queues[0]));

    /* A new ordinary notification must be accepted after saturation clears.
     * This verifies continued processing, not wakeup latency: the worker
     * also has an existing bounded polling timeout. */
    assert(lf_enqueue(&ctx.queues[0], &active_event) == 1);
    assert(notify_worker(&ctx, 0) == 0);
    int recovered_live = 0;
    for (int i = 0; i < 2000; ++i) {
        assert(atomic_load(&ctx.running) == 1);
        assert(atomic_load(&ctx.worker_error) == 0);
        saturated.revents = 0;
        int ready = poll(&saturated, 1, 0);
        assert(ready >= 0);
        assert(!(saturated.revents & (POLLERR | POLLHUP | POLLNVAL)));
        if (ready == 0 &&
            atomic_load(&ctx.metrics.dry_run_moves) == moves_before + 3 &&
            atomic_load(&ctx.active_workers) == 0) {
            recovered_live = 1;
            break;
        }
        sleep_milliseconds(1);
    }
    assert(recovered_live == 1);
    assert(lf_is_empty(&ctx.queues[0]));
    assert(join_workers(&ctx) == 0);
    assert(ctx.num_workers == 0 && ctx.efd[0] == -1);
    assert(atomic_load(&ctx.worker_error) == 0);
    assert(atomic_load(&ctx.active_workers) == 0);
    assert(lf_is_empty(&ctx.queues[0]));
    assert(atomic_load(&ctx.metrics.dry_run_moves) == moves_before + 3);
    puts("eventfd_saturation=PASS notify_coalesced=PASS live_drain=PASS rearm=PASS queue_drained=PASS");

    /* Distinct worker eventfds must retain independent readiness even
     * when one counter is saturated. Both consumers are held at the
     * existing gate after dequeuing their initial dry-run items. */
    atomic_store(&ctx.running, 1);
    lf_init(&ctx.queues[0]);
    lf_init(&ctx.queues[1]);
    const uint64_t isolation_before = atomic_load(&ctx.metrics.dry_run_moves);
    fc_test_pause_worker_before_process();
    assert(lf_enqueue(&ctx.queues[0], &active_event) == 1);
    assert(lf_enqueue(&ctx.queues[1], &active_event) == 1);
    assert(start_workers(&ctx, 2) == 0);
    assert(fc_test_wait_for_worker_pause(2000) == 0);
    for (int i = 0; i < 2000 && atomic_load(&ctx.active_workers) != 2; ++i)
        sleep_milliseconds(1);
    assert(atomic_load(&ctx.active_workers) == 2);
    assert(ctx.efd[0] >= 0 && ctx.efd[1] >= 0 && ctx.efd[0] != ctx.efd[1]);
    struct pollfd independent[2] = {
        { .fd = ctx.efd[0], .events = POLLIN | POLLOUT },
        { .fd = ctx.efd[1], .events = POLLIN | POLLOUT }
    };
    assert(poll(independent, 2, 0) == 2);
    assert(independent[0].revents == POLLOUT);
    assert(independent[1].revents == POLLOUT);
    assert(write(ctx.efd[0], &limit, sizeof(limit)) == (ssize_t)sizeof(limit));
    assert(poll(independent, 2, 0) == 2);
    assert(independent[0].revents == POLLIN);
    assert(independent[1].revents == POLLOUT);
    assert(lf_enqueue(&ctx.queues[0], &active_event) == 1);
    assert(lf_enqueue(&ctx.queues[1], &active_event) == 1);
    assert(notify_worker(&ctx, 0) == 0);
    assert(poll(independent, 2, 0) == 2);
    assert(independent[0].revents == POLLIN);
    assert(independent[1].revents == POLLOUT);
    assert(notify_worker(&ctx, 1) == 0);
    assert(poll(independent, 2, 0) == 2);
    assert(independent[0].revents == POLLIN);
    assert(independent[1].revents == (POLLIN | POLLOUT));
    errno = 0;
    assert(write(ctx.efd[0], &increment, sizeof(increment)) == -1 &&
           errno == EAGAIN);
    fc_test_release_worker();
    int independent_drained = 0;
    independent[0].events = POLLIN;
    independent[1].events = POLLIN;
    for (int i = 0; i < 2000; ++i) {
        assert(atomic_load(&ctx.running) == 1);
        assert(atomic_load(&ctx.worker_error) == 0);
        int ready = poll(independent, 2, 0);
        assert(ready >= 0);
        assert(!(independent[0].revents & (POLLERR | POLLHUP | POLLNVAL)));
        assert(!(independent[1].revents & (POLLERR | POLLHUP | POLLNVAL)));
        if (ready == 0 &&
            atomic_load(&ctx.metrics.dry_run_moves) == isolation_before + 4 &&
            atomic_load(&ctx.active_workers) == 0) {
            independent_drained = 1;
            break;
        }
        sleep_milliseconds(1);
    }
    assert(independent_drained == 1);
    assert(lf_is_empty(&ctx.queues[0]) && lf_is_empty(&ctx.queues[1]));
    assert(join_workers(&ctx) == 0);
    assert(ctx.num_workers == 0 && ctx.efd[0] == -1 && ctx.efd[1] == -1);
    assert(atomic_load(&ctx.worker_error) == 0);
    assert(atomic_load(&ctx.active_workers) == 0);
    assert(atomic_load(&ctx.metrics.dry_run_moves) == isolation_before + 4);
    puts("eventfd_worker_isolation=PASS distinct_readiness=PASS live_drain=PASS");

    /* Combine actual eventfd saturation with the documented SPSC queue
     * capacity. The sole consumer is gated after its first dequeue. */
    atomic_store(&ctx.running, 1);
    lf_init(&ctx.queues[0]);
    const uint64_t full_before = atomic_load(&ctx.metrics.dry_run_moves);
    fc_test_pause_worker_before_process();
    assert(lf_enqueue(&ctx.queues[0], &active_event) == 1);
    assert(start_workers(&ctx, 1) == 0);
    assert(fc_test_wait_for_worker_pause(2000) == 0);
    assert(atomic_load(&ctx.active_workers) == 1);
    assert(write(ctx.efd[0], &limit, sizeof(limit)) == (ssize_t)sizeof(limit));
    for (int i = 0; i < QUEUE_SIZE - 1; ++i)
        assert(lf_enqueue(&ctx.queues[0], &active_event) == 1);
    /* Full queue rejects an additional event without transferring it. */
    assert(lf_enqueue(&ctx.queues[0], &active_event) == 0);
    assert(notify_worker(&ctx, 0) == 0);
    errno = 0;
    assert(write(ctx.efd[0], &increment, sizeof(increment)) == -1 &&
           errno == EAGAIN);
    struct pollfd full_counter = { .fd = ctx.efd[0], .events = POLLIN };
    assert(poll(&full_counter, 1, 0) == 1 &&
           full_counter.revents == POLLIN);
    fc_test_release_worker();
    int full_drained = 0;
    for (int i = 0; i < 5000; ++i) {
        assert(atomic_load(&ctx.running) == 1);
        assert(atomic_load(&ctx.worker_error) == 0);
        int ready = poll(&full_counter, 1, 0);
        assert(ready >= 0);
        assert(!(full_counter.revents & (POLLERR | POLLHUP | POLLNVAL)));
        if (ready == 0 &&
            atomic_load(&ctx.metrics.dry_run_moves) ==
                full_before + QUEUE_SIZE &&
            atomic_load(&ctx.active_workers) == 0) {
            full_drained = 1;
            break;
        }
        sleep_milliseconds(1);
    }
    assert(full_drained == 1);
    assert(lf_is_empty(&ctx.queues[0]));
    /* The rejected enqueue did not consume capacity or inflate metrics.
     * Verify a fresh enqueue after wrap-around is accepted and drained. */
    assert(lf_enqueue(&ctx.queues[0], &active_event) == 1);
    assert(notify_worker(&ctx, 0) == 0);
    assert(join_workers(&ctx) == 0);
    assert(ctx.num_workers == 0 && ctx.efd[0] == -1);
    assert(atomic_load(&ctx.worker_error) == 0);
    assert(atomic_load(&ctx.active_workers) == 0);
    assert(lf_is_empty(&ctx.queues[0]));
    assert(atomic_load(&ctx.metrics.dry_run_moves) ==
           full_before + QUEUE_SIZE + 1);
    puts("eventfd_full_queue=PASS excess_rejected=PASS live_drain=PASS capacity_recovered=PASS");

    puts("worker lifecycle tests passed");
    return 0;
}
