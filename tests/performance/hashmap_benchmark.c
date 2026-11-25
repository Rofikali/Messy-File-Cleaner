#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "utils/hashmap.h"

enum { CAPACITY = 256, LOOKUPS = 32768, RUNS = 3 };
static HashEntry entries[CAPACITY];
static char keys[CAPACITY][MAX_KEY_LEN];
static char values[CAPACITY][MAX_VAL_LEN];
static char misses[CAPACITY][MAX_KEY_LEN];

static double elapsed_ns(struct timespec a, struct timespec b)
{
    return ((double)b.tv_sec - (double)a.tv_sec) * 1e9 +
           ((double)b.tv_nsec - (double)a.tv_nsec);
}

static int sample(HashMap *map, size_t occupied, int hit, int run)
{
    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        return -1;
    for (size_t i = 0; i < LOOKUPS; ++i)
    {
        size_t index = i % occupied;
        const char *result = hashmap_get(map, hit ? keys[index] : misses[i % CAPACITY]);
        if (hit ? (!result || strcmp(result, values[index]) != 0) : result != NULL)
            return -1;
    }
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0)
        return -1;
    double ns = elapsed_ns(start, end);
    if (ns <= 0.0 || map->size != occupied)
        return -1;
    printf("occupied=%zu run=%d workload=%s lookups=%d elapsed_ns=%.0f ns_per_lookup=%.3f lookups_per_second=%.3f\n",
           occupied, run, hit ? "hit" : "miss", LOOKUPS,
           ns, ns / LOOKUPS, LOOKUPS * 1e9 / ns);
    return 0;
}

int main(void)
{
    const size_t counts[] = {64, 128, 192, 256};
    printf("hashmap baseline: capacity=%d entry_bytes=%zu storage_bytes=%zu runs=%d\n",
           CAPACITY, sizeof(HashEntry), sizeof(entries), RUNS);
    for (size_t i = 0; i < CAPACITY; ++i)
    {
        int k = snprintf(keys[i], sizeof(keys[i]), ".ext%03zu", i);
        int v = snprintf(values[i], sizeof(values[i]), "Folder%03zu/", i);
        int m = snprintf(misses[i], sizeof(misses[i]), ".abs%03zu", i);
        if (k < 0 || (size_t)k >= sizeof(keys[i]) ||
            v < 0 || (size_t)v >= sizeof(values[i]) ||
            m < 0 || (size_t)m >= sizeof(misses[i]))
            return 1;
    }
    for (size_t j = 0; j < sizeof(counts) / sizeof(counts[0]); ++j)
    {
        HashMap map;
        size_t count = counts[j];
        hashmap_init(&map, entries, CAPACITY);
        for (size_t i = 0; i < count; ++i)
            if (hashmap_put(&map, keys[i], values[i]) != 0)
                return 1;
        if (map.size != count)
            return 1;
        for (int r = 1; r <= RUNS; ++r)
        {
            /* Alternate hit/miss order to reduce systematic ordering bias. */
            if (sample(&map, count, r % 2, r) != 0 ||
                sample(&map, count, !(r % 2), r) != 0)
                return 1;
        }
    }
    return 0;
}
