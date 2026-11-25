#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "core/lf_queue.h"

/* Independent test-only baseline. One producer and one consumer; no
 * filesystem, CPU affinity or production queue changes. */
enum { EVENTS = 65536, RUNS = 3, RETRY_CHECK = 256 };
static LFQueue queue;
static atomic_int failed;
static struct timespec start;

static double elapsed_ns(struct timespec a, struct timespec b)
{
    return ((double)b.tv_sec - (double)a.tv_sec) * 1e9 +
           ((double)b.tv_nsec - (double)a.tv_nsec);
}

static FileEvent make_event(uint32_t sequence)
{
    FileEvent event = {0};
    memset(event.path, 'A' + (int)(sequence % 26), sizeof(event.path));
    event.path[0] = 'x';
    event.path[1] = '\0';
    /* Unique sequence in the synthetic payload beyond the NUL. */
    memcpy(event.path + sizeof(event.path) - sizeof(sequence),
           &sequence, sizeof(sequence));
    event.type = sequence % 2 ? FILE_FOUND : FILE_CREATED;
    return event;
}

static int retry(unsigned int *attempts)
{
    if (atomic_load_explicit(&failed, memory_order_relaxed))
        return -1;
    if (++*attempts == RETRY_CHECK)
    {
        struct timespec now;
        *attempts = 0;
        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0 ||
            elapsed_ns(start, now) >= 5e9)
        {
            atomic_store_explicit(&failed, 1, memory_order_relaxed);
            return -1;
        }
    }
    (void)sched_yield();
    return 0;
}

static void *consume(void *arg)
{
    (void)arg;
    unsigned int attempts = 0;
    for (uint32_t i = 0; i < EVENTS; ++i)
    {
        FileEvent output;
        while (!lf_dequeue(&queue, &output))
            if (retry(&attempts) != 0)
                return NULL;
        FileEvent expected = make_event(i);
        if (output.type != expected.type ||
            memcmp(output.path, expected.path, sizeof(output.path)) != 0)
        {
            atomic_store_explicit(&failed, 1, memory_order_relaxed);
            return NULL;
        }
    }
    return NULL;
}

int main(void)
{
    printf("queue layout: bytes=%zu alignment=%zu head_offset=%zu tail_offset=%zu head_mod64=%zu tail_mod64=%zu\n",
           sizeof(LFQueue), (size_t)_Alignof(LFQueue),
           offsetof(LFQueue, head), offsetof(LFQueue, tail),
           (size_t)((uintptr_t)&queue.head % 64u),
           (size_t)((uintptr_t)&queue.tail % 64u));
#ifdef FC_QUEUE_PADDED_EXPERIMENT
    if ((uintptr_t)&queue.head % 64u != 0 ||
        (uintptr_t)&queue.tail % 64u != 0 ||
        offsetof(LFQueue, tail) - offsetof(LFQueue, head) < 64u)
        return 1;
#endif
    printf("queue concurrent baseline: events_per_run=%d runs=%d event_bytes=%zu\n",
           EVENTS, RUNS, sizeof(FileEvent));
    for (int r = 0; r < RUNS; ++r)
    {
        lf_init(&queue);
        atomic_init(&failed, 0);
        pthread_t consumer;
        if (clock_gettime(CLOCK_MONOTONIC, &start) != 0 ||
            pthread_create(&consumer, NULL, consume, NULL) != 0)
            return 1;
        unsigned int attempts = 0;
        for (uint32_t i = 0; i < EVENTS; ++i)
        {
            if (atomic_load_explicit(&failed, memory_order_relaxed))
                break;
            FileEvent event = make_event(i);
            while (!lf_enqueue(&queue, &event))
                if (retry(&attempts) != 0)
                    break;
        }
        if (pthread_join(consumer, NULL) != 0 ||
            atomic_load_explicit(&failed, memory_order_relaxed) ||
            !lf_is_empty(&queue))
            return 1;
        struct timespec end;
        if (clock_gettime(CLOCK_MONOTONIC, &end) != 0)
            return 1;
        double ns = elapsed_ns(start, end);
        if (ns <= 0.0)
            return 1;
        printf("run=%d events=%d elapsed_ns=%.0f ns_per_transfer=%.3f events_per_second=%.3f\n",
               r + 1, EVENTS, ns, ns / EVENTS, EVENTS * 1e9 / ns);
    }
    return 0;
}
