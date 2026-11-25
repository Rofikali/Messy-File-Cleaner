#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>
#include "utils/root_path.h"

/* Actual lexical helper, bounded batches of simultaneously owned copies. */
enum { BATCH = 64, ROUNDS = 64, RUNS = 3, MAX_PARENT = 1024 };
static const char name[] = "photo.jpg";

static int sample(size_t length, int trailing_slash, int run)
{
    char parent[MAX_PARENT + 1];
    char expected[MAX_PARENT + sizeof name + 1];
    char *owned[BATCH] = {0};
    size_t peak_bytes = 0, calls = 0, released = 0;
    memset(parent, 'p', length);
    parent[0] = '/';
    if (trailing_slash)
        parent[length - 1] = '/';
    parent[length] = '\0';
    int n = snprintf(expected, sizeof expected, "%s%s%s", parent,
                     trailing_slash ? "" : "/", name);
    if (n < 0 || (size_t)n >= sizeof expected)
        return -1;
    size_t bytes = (size_t)n + 1;
    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        return -1;
    int result = -1;
    for (size_t round = 0; round < ROUNDS; ++round)
    {
        for (size_t i = 0; i < BATCH; ++i)
        {
            owned[i] = fc_join_path(parent, name);
            if (!owned[i])
                goto cleanup;
            ++calls;
            if (strcmp(owned[i], expected) != 0)
                goto cleanup;
        }
        /* Confirm that simultaneously live copies are independent. */
        owned[0][0] = 'X';
        for (size_t i = 1; i < BATCH; ++i)
            if (strcmp(owned[i], expected) != 0)
                goto cleanup;
        owned[0][0] = '/';
        peak_bytes = BATCH * bytes;
        for (size_t i = BATCH; i > 0; --i)
        {
            free(owned[i - 1]);
            owned[i - 1] = NULL;
            ++released;
        }
    }
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0 ||
        calls != BATCH * ROUNDS || calls != released)
        goto cleanup;
    double ns = ((double)end.tv_sec - (double)start.tv_sec) * 1e9 +
                ((double)end.tv_nsec - (double)start.tv_nsec);
    if (ns <= 0)
        goto cleanup;
    printf("parent_bytes=%zu trailing_slash=%d run=%d allocations=%zu frees=%zu requested_bytes=%zu peak_live_requested_bytes=%zu elapsed_ns=%.0f ns_per_join_free=%.3f\n",
           length, trailing_slash, run, calls, released, calls * bytes,
           peak_bytes, ns, ns / calls);
    result = 0;
cleanup:
    for (size_t i = 0; i < BATCH; ++i)
        free(owned[i]);
    return result;
}

int main(void)
{
    const size_t lengths[] = {32, 256, MAX_PARENT};
    for (int r = 1; r <= RUNS; ++r)
        for (size_t j = 0; j < sizeof lengths / sizeof lengths[0]; ++j)
        {
            size_t length = lengths[r % 2 ? j : 2 - j];
            if (sample(length, r % 2, r) != 0 ||
                sample(length, !(r % 2), r) != 0)
                return 1;
        }
    return 0;
}
