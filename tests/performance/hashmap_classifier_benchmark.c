#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "classifier/classifier.h"

/* Synthetic path mix; no filesystem access or production code changes. */
enum { MAX_CAPACITY = 256, KEYS = 64, KINDS = 6, PATH_LEN = 128,
       LOOKUPS = 32768, RUNS = 3 };
static HashEntry entries[MAX_CAPACITY];
static char keys[KEYS][MAX_KEY_LEN];
static char values[KEYS][MAX_VAL_LEN];
static char paths[KEYS][KINDS][PATH_LEN];
static const char fallback[] = "Others/";

static int prepare(void)
{
    const char *defaults[] = {".jpg", ".jpeg", ".png", ".html"};
    const char *folders[] = {"Jpeg/", "Jpeg/", "Png/", "Html/"};
    for (size_t i = 0; i < KEYS; ++i)
    {
        int k = i < 4 ? snprintf(keys[i], sizeof keys[i], "%s", defaults[i])
                      : snprintf(keys[i], sizeof keys[i], ".ext%03zu", i);
        int v = i < 4 ? snprintf(values[i], sizeof values[i], "%s", folders[i])
                      : snprintf(values[i], sizeof values[i], "Folder%03zu/", i);
        if (k < 0 || (size_t)k >= sizeof keys[i] ||
            v < 0 || (size_t)v >= sizeof values[i])
            return -1;
        int n[KINDS];
        n[0] = snprintf(paths[i][0], PATH_LEN, "/fixture/dir.x/photo%03zu%s", i, keys[i]);
        n[1] = snprintf(paths[i][1], PATH_LEN, "/fixture/dir.x/photo%03zu.abs%03zu", i, i);
        n[2] = snprintf(paths[i][2], PATH_LEN, "/fixture/dir.x/plain%03zu", i);
        n[3] = snprintf(paths[i][3], PATH_LEN, "/fixture/dir.x%s", "/.profile");
        n[4] = snprintf(paths[i][4], PATH_LEN, "/fixture/dir.x/name%03zu.", i);
        n[5] = snprintf(paths[i][5], PATH_LEN, "/fixture/dir.x/archive%03zu.tar%s", i, keys[i]);
        for (size_t j = 0; j < KINDS; ++j)
            if (n[j] < 0 || n[j] >= PATH_LEN)
                return -1;
    }
    return 0;
}

static int sample(HashMap *map, size_t count, int run)
{
    Classifier c;
    classifier_init(&c, map, fallback);
    size_t seen[KINDS] = {0};
    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        return -1;
    for (size_t i = 0; i < LOOKUPS; ++i)
    {
        size_t kind = i % KINDS;
        size_t key = (i / KINDS) % count;
        const char *expected = (kind == 0 || kind == 5) ? values[key] : fallback;
        const char *actual = classify(&c, paths[key][kind]);
        if (!actual || strcmp(actual, expected) != 0)
            return -1;
        ++seen[kind];
    }
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0 || map->size != count)
        return -1;
    for (size_t j = 0; j < KINDS; ++j)
        if (seen[j] != LOOKUPS / KINDS + (j < LOOKUPS % KINDS ? 1U : 0U))
            return -1;
    double ns = ((double)end.tv_sec - (double)start.tv_sec) * 1e9 +
                ((double)end.tv_nsec - (double)start.tv_nsec);
    if (ns <= 0)
        return -1;
    printf("capacity=%zu keys=%zu run=%d lookups=%d logical_bytes=%zu elapsed_ns=%.0f ns_per_classification=%.3f\n",
           map->capacity, count, run, LOOKUPS, map->capacity * sizeof(HashEntry),
           ns, ns / LOOKUPS);
    return 0;
}

int main(void)
{
    const size_t counts[] = {4, KEYS};
    const size_t capacities[] = {64, 128, 256};
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
                if (map.size != counts[j] || sample(&map, counts[j], run) != 0)
                    return 1;
            }
    return 0;
}
