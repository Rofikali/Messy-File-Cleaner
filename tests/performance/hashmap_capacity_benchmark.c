#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "utils/hashmap.h"

/* Fixed key sets across capacities; independent original test harness. */
enum { MAX_CAPACITY = 256, KEYS = 64, LOOKUPS = 32768, RUNS = 3 };
static HashEntry entries[MAX_CAPACITY];
static char keys[KEYS][MAX_KEY_LEN];
static char values[KEYS][MAX_VAL_LEN];
static char misses[KEYS][MAX_KEY_LEN];

static size_t longest_cluster(const HashMap *map)
{
    size_t empty = 0;
    while (empty < map->capacity && map->entries[empty].used)
        ++empty;
    if (empty == map->capacity)
        return map->capacity;
    size_t longest = 0, run = 0;
    for (size_t i = 1; i <= map->capacity; ++i)
    {
        size_t slot = (empty + i) & (map->capacity - 1);
        if (map->entries[slot].used)
        {
            ++run;
            if (run > longest)
                longest = run;
        }
        else
            run = 0;
    }
    return longest;
}

static int sample(HashMap *map, size_t count, int hit, int run)
{
    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        return -1;
    for (size_t i = 0; i < LOOKUPS; ++i)
    {
        size_t index = i % count;
        const char *result = hashmap_get(map, hit ? keys[index] : misses[i % KEYS]);
        if (hit ? (!result || strcmp(result, values[index]) != 0) : result != NULL)
            return -1;
    }
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0)
        return -1;
    double ns = ((double)end.tv_sec - (double)start.tv_sec) * 1e9 +
                ((double)end.tv_nsec - (double)start.tv_nsec);
    if (ns <= 0 || map->size != count)
        return -1;
    printf("capacity=%zu keys=%zu run=%d workload=%s lookups=%d elapsed_ns=%.0f ns_per_lookup=%.3f\n",
           map->capacity, count, run, hit ? "hit" : "miss", LOOKUPS, ns, ns / LOOKUPS);
    return 0;
}

int main(void)
{
    const char *default_keys[] = {".jpg", ".jpeg", ".png", ".html"};
    const char *default_values[] = {"Jpeg/", "Jpeg/", "Png/", "Html/"};
    const size_t counts[] = {4, KEYS};
    const size_t capacities[] = {64, 128, 256};
    for (size_t i = 0; i < KEYS; ++i)
    {
        int k = snprintf(keys[i], sizeof(keys[i]), ".ext%03zu", i);
        int v = snprintf(values[i], sizeof(values[i]), "Folder%03zu/", i);
        int m = snprintf(misses[i], sizeof(misses[i]), ".abs%03zu", i);
        if (k < 0 || (size_t)k >= sizeof(keys[i]) ||
            v < 0 || (size_t)v >= sizeof(values[i]) ||
            m < 0 || (size_t)m >= sizeof(misses[i]))
            return 1;
    }
    for (size_t i = 0; i < sizeof(default_keys) / sizeof(default_keys[0]); ++i)
    {
        int k = snprintf(keys[i], sizeof(keys[i]), "%s", default_keys[i]);
        int v = snprintf(values[i], sizeof(values[i]), "%s", default_values[i]);
        if (k < 0 || (size_t)k >= sizeof(keys[i]) ||
            v < 0 || (size_t)v >= sizeof(values[i]))
            return 1;
    }
    for (size_t j = 0; j < sizeof(counts) / sizeof(counts[0]); ++j)
    {
        size_t count = counts[j];
        /* Reverse capacity order in alternating samples. Build once per
         * capacity/sample so insertion order stays identical. */
        for (int r = 1; r <= RUNS; ++r)
        {
            for (size_t x = 0; x < sizeof(capacities) / sizeof(capacities[0]); ++x)
            {
                size_t cap = capacities[r % 2 ? x : 2 - x];
                HashMap map;
                hashmap_init(&map, entries, cap);
                for (size_t i = 0; i < count; ++i)
                    if (hashmap_put(&map, keys[i], values[i]) != 0)
                        return 1;
                if (map.size != count)
                    return 1;
                printf("layout capacity=%zu keys=%zu run=%d logical_bytes=%zu longest_cluster=%zu\n",
                       cap, count, r, cap * sizeof(HashEntry), longest_cluster(&map));
                if (sample(&map, count, r % 2, r) != 0 ||
                    sample(&map, count, !(r % 2), r) != 0)
                    return 1;
            }
        }
    }
    return 0;
}
