#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#include "classifier/classifier.h"
#include "core/context.h"
#include "core/thread_pool.h"
#include "utils/hashmap.h"

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
    errno = 0;
    assert(notify_worker(&ctx, 2) == -1);
    assert(errno == EINVAL);

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

    /* Empty workers must wake and join cleanly too. */
    atomic_store(&ctx.running, 1);
    assert(start_workers(&ctx, 1) == 0);
    assert(join_workers(&ctx) == 0);
    assert(atomic_load(&ctx.worker_error) == 0);

    puts("worker lifecycle tests passed");
    return 0;
}
