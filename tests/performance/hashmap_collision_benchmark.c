#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "utils/hashmap.h"

/* Discover colliding keys through the real map, not a second hash implementation. */
enum { BASE = 64, MAX_CAPACITY = 256, KEYS = 64, MISSES = 8,
       TOTAL = KEYS + MISSES, LIMIT = 100000, LOOKUPS = 32768, RUNS = 3 };
static HashEntry entries[MAX_CAPACITY];
static char keys[TOTAL][MAX_KEY_LEN];
static char values[KEYS][MAX_VAL_LEN];

static int prepare(void)
{
    HashEntry probe[BASE];
    HashMap map;
    size_t found = 0;
    for (unsigned int i = 0; i < LIMIT && found < TOTAL; ++i)
    {
        char key[MAX_KEY_LEN];
        int n = snprintf(key, sizeof key, ".c%06u", i);
        if (n < 0 || (size_t)n >= sizeof key)
            return -1;
        hashmap_init(&map, probe, BASE);
        if (hashmap_put(&map, key, "probe") != 0 || map.size != 1)
            return -1;
        if (probe[BASE - 1].used)
        {
            if (strcmp(probe[BASE - 1].key, key) != 0)
                return -1;
            memcpy(keys[found++], key, (size_t)n + 1);
        }
    }
    if (found != TOTAL)
        return -1;
    for (size_t i = 0; i < KEYS; ++i)
    {
        int n = snprintf(values[i], sizeof values[i], "Folder%03zu/", i);
        if (n < 0 || (size_t)n >= sizeof values[i])
            return -1;
    }
    return 0;
}

static int verify_layout(HashMap *map, size_t count)
{
    if (map->size != count)
        return -1;
    if (map->capacity == BASE)
    {
        /* A collision at slot 63 must wrap to 0 without overwriting. */
        for (size_t i = 0; i < count; ++i)
        {
            const HashEntry *e = &map->entries[(BASE - 1 + i) % BASE];
            if (!e->used || strcmp(e->key, keys[i]) != 0 ||
                strcmp(e->value, values[i]) != 0)
                return -1;
        }
    }
    if (count == map->capacity)
    {
        if (hashmap_put(map, keys[KEYS], "unexpected") != -1 ||
            hashmap_get(map, keys[KEYS]) != NULL || map->size != count)
            return -1;
    }
    /* Existing-key updates, including a full table, must preserve size. */
    if (hashmap_put(map, keys[count - 1], "Updated/") != 0)
        return -1;
    const char *updated = hashmap_get(map, keys[count - 1]);
    if (!updated || strcmp(updated, "Updated/") != 0 || map->size != count ||
        hashmap_put(map, keys[count - 1], values[count - 1]) != 0)
        return -1;
    for (size_t i = 0; i < count; ++i)
    {
        const char *v = hashmap_get(map, keys[i]);
        if (!v || strcmp(v, values[i]) != 0)
            return -1;
    }
    return map->size == count ? 0 : -1;
}

static int sample(HashMap *map, size_t count, int hit, int run)
{
    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        return -1;
    for (size_t i = 0; i < LOOKUPS; ++i)
    {
        size_t index = i % count;
        const char *v = hashmap_get(map, hit ? keys[index] : keys[KEYS + i % MISSES]);
        if (hit ? (!v || strcmp(v, values[index]) != 0) : v != NULL)
            return -1;
    }
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0 || map->size != count)
        return -1;
    double ns = ((double)end.tv_sec - (double)start.tv_sec) * 1e9 +
                ((double)end.tv_nsec - (double)start.tv_nsec);
    if (ns <= 0)
        return -1;
    printf("capacity=%zu keys=%zu run=%d workload=%s lookups=%d logical_bytes=%zu elapsed_ns=%.0f ns_per_lookup=%.3f\n",
           map->capacity, count, run, hit ? "hit" : "miss", LOOKUPS,
           map->capacity * sizeof(HashEntry), ns, ns / LOOKUPS);
    return 0;
}

int main(void)
{
    const size_t counts[] = {32, KEYS};
    const size_t capacities[] = {BASE, 128, MAX_CAPACITY};
    if (prepare() != 0)
        return 1;
    for (size_t j = 0; j < sizeof counts / sizeof counts[0]; ++j)
        for (int run = 1; run <= RUNS; ++run)
            for (size_t x = 0; x < sizeof capacities / sizeof capacities[0]; ++x)
            {
                size_t cap = capacities[run % 2 ? x : 2 - x];
                HashMap map;
                hashmap_init(&map, entries, cap);
                for (size_t i = 0; i < counts[j]; ++i)
                    if (hashmap_put(&map, keys[i], values[i]) != 0)
                        return 1;
                if (verify_layout(&map, counts[j]) != 0 ||
                    sample(&map, counts[j], run % 2, run) != 0 ||
                    sample(&map, counts[j], !(run % 2), run) != 0)
                    return 1;
            }
    return 0;
}
