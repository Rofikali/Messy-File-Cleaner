#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "core/lf_queue.h"

/* Independent bounded baseline: no files, threads, affinity or privileges.
 * Measures batched enqueue/dequeue plus payload verification, not concurrent
 * transfer latency or end-to-end filesystem throughput. */
enum { BATCH = 256, ROUNDS = 256, RUNS = 3 };
_Static_assert(BATCH < QUEUE_SIZE, "batch must fit in the queue");
static LFQueue queue;
static FileEvent inputs[BATCH];

static double elapsed_ns(struct timespec start, struct timespec end)
{
    return ((double)end.tv_sec - (double)start.tv_sec) * 1e9 +
           ((double)end.tv_nsec - (double)start.tv_nsec);
}

static int run_batch(void)
{
    for (int i = 0; i < BATCH; ++i)
        if (!lf_enqueue(&queue, &inputs[i]))
            return -1;
    for (int i = 0; i < BATCH; ++i)
    {
        FileEvent output;
        if (!lf_dequeue(&queue, &output) ||
            output.type != inputs[i].type ||
            memcmp(output.path, inputs[i].path, sizeof(output.path)) != 0)
            return -1;
    }
    return lf_is_empty(&queue) ? 0 : -1;
}

int main(void)
{
    for (int i = 0; i < BATCH; ++i)
    {
        memset(inputs[i].path, 'A' + i % 26, sizeof(inputs[i].path));
        int n = snprintf(inputs[i].path, sizeof(inputs[i].path), "fixture-%d", i);
        if (n < 0 || (size_t)n >= sizeof(inputs[i].path))
            return 1;
        inputs[i].type = i % 2 ? FILE_FOUND : FILE_CREATED;
    }
    const int pairs = BATCH * ROUNDS;
    printf("queue baseline: batch=%d pairs_per_run=%d runs=%d event_bytes=%zu\n",
           BATCH, pairs, RUNS, sizeof(FileEvent));
    for (int r = 0; r < RUNS; ++r)
    {
        lf_init(&queue);
        if (run_batch() != 0)
            return 1;
        struct timespec start, end;
        if (clock_gettime(CLOCK_MONOTONIC, &start) != 0)
            return 1;
        for (int i = 0; i < ROUNDS; ++i)
            if (run_batch() != 0)
                return 1;
        if (clock_gettime(CLOCK_MONOTONIC, &end) != 0)
            return 1;
        double ns = elapsed_ns(start, end);
        if (ns <= 0.0)
            return 1;
        printf("run=%d pairs=%d elapsed_ns=%.0f ns_per_pair=%.3f pairs_per_second=%.3f\n",
               r + 1, pairs, ns, ns / pairs, pairs * 1e9 / ns);
    }
    return 0;
}
