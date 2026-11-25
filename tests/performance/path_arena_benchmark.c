#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>
#include "utils/root_path.h"

/* Test-only byte arena. Returned strings are borrowed until batch reset. */
enum { BATCH = 64, ROUNDS = 64, RUNS = 3, MAX_PARENT = 1024 };
static const char name[] = "photo.jpg";
typedef struct { char *data; size_t capacity; size_t used; } Arena;

static char *arena_join(Arena *a, const char *parent, const char *child)
{
    if (!a || !a->data || a->used > a->capacity ||
        !parent || !*parent || !child || !*child)
        return NULL;
    size_t p = strlen(parent), c = strlen(child);
    size_t sep = parent[p - 1] == '/' ? 0U : 1U;
    if (p > SIZE_MAX - sep - 1 || c > SIZE_MAX - p - sep - 1)
        return NULL;
    size_t bytes = p + sep + c + 1;
    if (bytes > a->capacity - a->used)
        return NULL;
    char *out = a->data + a->used;
    memcpy(out, parent, p);
    if (sep)
        out[p] = '/';
    memcpy(out + p + sep, child, c + 1);
    a->used += bytes;
    return out;
}

static int guard_checks(void)
{
    char data[8];
    memset(data, '!', sizeof data);
    Arena a = {data, 4, 0};
    if (arena_join(&a, "", "b") || arena_join(&a, "a", "") ||
        arena_join(&a, NULL, "b") || a.used != 0)
        return -1;
    if (arena_join(&a, "ab", "b") || a.used != 0)
        return -1;
    for (size_t i = 0; i < sizeof data; ++i)
        if (data[i] != '!')
            return -1;
    char *p = arena_join(&a, "a", "b");
    if (!p || strcmp(p, "a/b") || a.used != 4)
        return -1;
    if (arena_join(&a, "a", "b") || a.used != 4 || strcmp(p, "a/b"))
        return -1;
    for (size_t i = 4; i < sizeof data; ++i)
        if (data[i] != '!')
            return -1;
    a.used = 0; /* No borrowed pointer may be used after this reset. */
    p = arena_join(&a, "a/", "b");
    return p && !strcmp(p, "a/b") && a.used == 4 ? 0 : -1;
}

static int sample(size_t length, int trailing_slash, int use_arena, int run)
{
    char parent[MAX_PARENT + 1];
    char expected[MAX_PARENT + sizeof name + 1];
    char *paths[BATCH] = {0};
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
    if (bytes > SIZE_MAX / BATCH)
        return -1;
    Arena a = {NULL, bytes * BATCH, 0};
    struct timespec start, end;
    size_t joins = 0, allocations = 0, frees = 0, resets = 0;
    int result = -1;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        return -1;
    if (use_arena)
    {
        a.data = malloc(a.capacity);
        if (!a.data)
            goto cleanup;
        ++allocations;
    }
    for (size_t round = 0; round < ROUNDS; ++round)
    {
        for (size_t i = 0; i < BATCH; ++i)
        {
            paths[i] = use_arena ? arena_join(&a, parent, name)
                                 : fc_join_path(parent, name);
            if (!paths[i])
                goto cleanup;
            if (!use_arena)
                ++allocations;
            ++joins;
            if (strcmp(paths[i], expected))
                goto cleanup;
        }
        if (use_arena && a.used != a.capacity)
            goto cleanup;
        paths[0][0] = 'X';
        for (size_t i = 1; i < BATCH; ++i)
            if (strcmp(paths[i], expected))
                goto cleanup;
        paths[0][0] = '/';
        for (size_t i = BATCH; i > 0; --i)
        {
            if (!use_arena)
            {
                free(paths[i - 1]);
                ++frees;
            }
            paths[i - 1] = NULL;
        }
        if (use_arena)
        {
            a.used = 0;
            ++resets;
        }
    }
    if (use_arena)
    {
        free(a.data);
        a.data = NULL;
        ++frees;
    }
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0 ||
        joins != BATCH * ROUNDS || allocations != frees ||
        allocations != (use_arena ? 1U : BATCH * ROUNDS) ||
        resets != (use_arena ? ROUNDS : 0U))
        goto cleanup;
    double ns = ((double)end.tv_sec - (double)start.tv_sec) * 1e9 +
                ((double)end.tv_nsec - (double)start.tv_nsec);
    if (ns <= 0)
        goto cleanup;
    printf("mode=%s parent_bytes=%zu trailing_slash=%d run=%d joins=%zu allocations=%zu frees=%zu resets=%zu peak_requested_bytes=%zu elapsed_ns=%.0f ns_per_join=%.3f\n",
           use_arena ? "arena" : "owned", length, trailing_slash, run,
           joins, allocations, frees, resets, a.capacity, ns, ns / joins);
    result = 0;
cleanup:
    if (!use_arena)
        for (size_t i = 0; i < BATCH; ++i)
            free(paths[i]);
    free(a.data);
    return result;
}


/* Model nested directory paths and transient sibling paths, not filesystem I/O.
 * Borrowed arena pointers are released in strict LIFO order. */
enum { DEPTH = 16, SIBLINGS = 8, NEST_ROUNDS = 64, NEST_CAP = 32768,
       COMPONENT_CAP = 48, EXPECTED_CAP = 1024 };

static char *nested_join(Arena *a, int arena, const char *parent, const char *child)
{
    return arena ? arena_join(a, parent, child) : fc_join_path(parent, child);
}

static int nested_sample(int use_arena, int run)
{
    char components[DEPTH][COMPONENT_CAP];
    char leaves[DEPTH][SIBLINGS][COMPONENT_CAP];
    char expected[DEPTH + 1][EXPECTED_CAP];
    char leaf_expected[DEPTH][SIBLINGS][EXPECTED_CAP];
    char *frames[DEPTH] = {0};
    size_t marks[DEPTH] = {0}, frame_bytes[DEPTH] = {0};
    char *leaf = NULL;
    size_t live = 0, peak = 0, joins = 0, releases = 0;
    size_t allocations = 0, frees = 0;
    Arena a = {NULL, NEST_CAP, 0};
    memcpy(expected[0], "/fixture", sizeof "/fixture");
    for (size_t depth = 0; depth < DEPTH; ++depth)
    {
        int n = snprintf(components[depth], COMPONENT_CAP, "dir%02zu_%0*zu",
                         depth, (int)(depth % 24 + 1), depth);
        if (n < 0 || n >= COMPONENT_CAP)
            return -1;
        n = snprintf(expected[depth + 1], EXPECTED_CAP, "%s/%s",
                     expected[depth], components[depth]);
        if (n < 0 || n >= EXPECTED_CAP)
            return -1;
        for (size_t j = 0; j < SIBLINGS; ++j)
        {
            n = snprintf(leaves[depth][j], COMPONENT_CAP, "file%zu_%0*zu.jpg",
                         j, (int)((depth + j) % 30 + 1), j);
            if (n < 0 || n >= COMPONENT_CAP)
                return -1;
            n = snprintf(leaf_expected[depth][j], EXPECTED_CAP, "%s/%s",
                         expected[depth], leaves[depth][j]);
            if (n < 0 || n >= EXPECTED_CAP)
                return -1;
        }
    }
    struct timespec start, end;
    int result = -1;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        return -1;
    if (use_arena)
    {
        a.data = malloc(a.capacity);
        if (!a.data)
            goto cleanup;
        ++allocations;
    }
    for (size_t round = 0; round < NEST_ROUNDS; ++round)
    {
        for (size_t depth = 0; depth < DEPTH; ++depth)
        {
            const char *parent = depth ? frames[depth - 1] : expected[0];
            for (size_t j = 0; j < SIBLINGS; ++j)
            {
                size_t mark = a.used;
                leaf = nested_join(&a, use_arena, parent, leaves[depth][j]);
                if (!leaf || strcmp(leaf, leaf_expected[depth][j]))
                    goto cleanup;
                ++joins;
                if (!use_arena)
                    ++allocations;
                size_t bytes = strlen(leaf_expected[depth][j]) + 1;
                if (live + bytes > peak)
                    peak = live + bytes;
                if (use_arena)
                {
                    if (a.used != mark + bytes)
                        goto cleanup;
                    leaf = NULL;
                    a.used = mark;
                }
                else
                {
                    free(leaf);
                    leaf = NULL;
                    ++frees;
                }
                ++releases;
                for (size_t k = 0; k < depth; ++k)
                    if (strcmp(frames[k], expected[k + 1]))
                        goto cleanup;
            }
            marks[depth] = a.used;
            frames[depth] = nested_join(&a, use_arena, parent, components[depth]);
            if (!frames[depth] || strcmp(frames[depth], expected[depth + 1]))
                goto cleanup;
            ++joins;
            if (!use_arena)
                ++allocations;
            frame_bytes[depth] = strlen(expected[depth + 1]) + 1;
            live += frame_bytes[depth];
            if (live > peak)
                peak = live;
            if (use_arena && a.used != live)
                goto cleanup;
        }
        for (size_t depth = DEPTH; depth > 0; --depth)
        {
            size_t i = depth - 1;
            if (strcmp(frames[i], expected[depth]))
                goto cleanup;
            live -= frame_bytes[i];
            if (use_arena)
            {
                frames[i] = NULL;
                a.used = marks[i];
                if (a.used != live)
                    goto cleanup;
            }
            else
            {
                free(frames[i]);
                frames[i] = NULL;
                ++frees;
            }
            ++releases;
            for (size_t k = 0; k < i; ++k)
                if (strcmp(frames[k], expected[k + 1]))
                    goto cleanup;
        }
        if (live != 0 || a.used != 0)
            goto cleanup;
    }
    if (use_arena)
    {
        free(a.data);
        a.data = NULL;
        ++frees;
    }
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0 ||
        joins != NEST_ROUNDS * DEPTH * (SIBLINGS + 1) ||
        joins != releases || allocations != frees ||
        allocations != (use_arena ? 1U : joins))
        goto cleanup;
    double ns = ((double)end.tv_sec - (double)start.tv_sec) * 1e9 +
                ((double)end.tv_nsec - (double)start.tv_nsec);
    if (ns <= 0)
        goto cleanup;
    printf("nested mode=%s run=%d joins=%zu allocations=%zu frees=%zu peak_live_payload_bytes=%zu reserved_arena_bytes=%zu elapsed_ns=%.0f ns_per_join=%.3f\n",
           use_arena ? "arena" : "owned", run, joins, allocations, frees,
           peak, use_arena ? a.capacity : 0U, ns, ns / joins);
    result = 0;
cleanup:
    if (!use_arena)
    {
        free(leaf);
        for (size_t i = 0; i < DEPTH; ++i)
            free(frames[i]);
    }
    free(a.data);
    return result;
}

int main(void)
{
    const size_t lengths[] = {32, 256, MAX_PARENT};
    if (guard_checks() != 0)
        return 1;
    for (int r = 1; r <= RUNS; ++r)
        for (size_t j = 0; j < sizeof lengths / sizeof lengths[0]; ++j)
            for (int s = 0; s < 2; ++s)
            {
                size_t length = lengths[r % 2 ? j : 2 - j];
                int slash = (s + r) % 2, first = (s + r + (int)j) % 2;
                if (sample(length, slash, first, r) != 0 ||
                    sample(length, slash, !first, r) != 0)
                    return 1;
            }
    for (int r = 1; r <= RUNS; ++r)
        if (nested_sample(r % 2, r) != 0 ||
            nested_sample(!(r % 2), r) != 0)
            return 1;
    return 0;
}
