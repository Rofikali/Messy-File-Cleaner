#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <time.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/event.h"

/* Independent read-only representation experiment; not a queue or worker. */
struct packed_event {
    size_t offset;
    size_t length;
    EventType type;
};

static size_t path_length(size_t i, int mixed)
{
    static const size_t lengths[] = {15, 63, 255, 1023};
    return mixed ? lengths[i % 4U] : lengths[i % 2U];
}

static unsigned char path_byte(size_t i, size_t j)
{
    return (unsigned char)('a' + (i % 26U + j % 26U) % 26U);
}

static uint64_t accumulate(uint64_t hash, const char *path, EventType type)
{
    for (size_t i = 0; path[i] != '\0'; ++i)
    {
        hash ^= (unsigned char)path[i];
        hash *= UINT64_C(1099511628211);
    }
    hash ^= (uint64_t)type + 1U;
    return hash * UINT64_C(1099511628211);
}

static int64_t elapsed_ns(struct timespec start, struct timespec end)
{
    return (int64_t)(end.tv_sec - start.tv_sec) * INT64_C(1000000000)
           + (int64_t)end.tv_nsec - (int64_t)start.tv_nsec;
}

/* Build an independently owned snapshot from the same validated fixed
 * input. Compare full fixed copies against packed allocation and copying,
 * including sizing in the packed construction interval. */
static int measure(const FileEvent *input, size_t count, int mixed,
                   int packed, unsigned int round, uint64_t expected_hash)
{
    FileEvent *fixed = NULL;
    struct packed_event *index = NULL;
    char *paths = NULL;
    size_t bytes = 0;
    int result = 1;
    struct timespec build_start, build_end, scan_start, scan_end;
    if (count == 0 || count > 4096U || count > SIZE_MAX / sizeof *fixed ||
        count > SIZE_MAX / sizeof *index ||
        clock_gettime(CLOCK_MONOTONIC, &build_start) != 0)
        return 1;
    if (!packed)
    {
        fixed = malloc(count * sizeof *fixed);
        if (!fixed)
            goto cleanup;
        memcpy(fixed, input, count * sizeof *fixed);
    }
    else
    {
        for (size_t i = 0; i < count; ++i)
        {
            /* Input is already bounded and validated by sample(). */
            size_t n = strlen(input[i].path);
            if (n >= sizeof input[i].path || n + 1U > SIZE_MAX - bytes)
                goto cleanup;
            bytes += n + 1U;
        }
        if (bytes > SIZE_MAX - count * sizeof *index)
            goto cleanup;
        index = malloc(count * sizeof *index);
        paths = malloc(bytes);
        if (!index || !paths)
            goto cleanup;
        size_t cursor = 0;
        for (size_t i = 0; i < count; ++i)
        {
            size_t n = strlen(input[i].path);
            if (cursor >= bytes || n >= bytes - cursor)
                goto cleanup;
            index[i] = (struct packed_event){cursor, n, input[i].type};
            memcpy(paths + cursor, input[i].path, n + 1U);
            cursor += n + 1U;
        }
        if (cursor != bytes)
            goto cleanup;
    }
    if (clock_gettime(CLOCK_MONOTONIC, &build_end) != 0)
        goto cleanup;
    /* Verify every event and offset outside both timing intervals. */
    size_t cursor = 0;
    for (size_t i = 0; i < count; ++i)
    {
        size_t n = path_length(i, mixed);
        if (packed)
        {
            if (cursor >= bytes || n >= bytes - cursor ||
                index[i].offset != cursor || index[i].length != n ||
                index[i].type != input[i].type ||
                memcmp(paths + cursor, input[i].path, n + 1U) != 0)
                goto cleanup;
            cursor += n + 1U;
        }
        else if (fixed[i].type != input[i].type ||
                 memcmp(fixed[i].path, input[i].path, sizeof fixed[i].path) != 0)
            goto cleanup;
    }
    if (packed && cursor != bytes)
        goto cleanup;
    uint64_t hash = UINT64_C(14695981039346656037);
    if (clock_gettime(CLOCK_MONOTONIC, &scan_start) != 0)
        goto cleanup;
    if (packed)
        for (size_t i = 0; i < count; ++i)
            hash = accumulate(hash, paths + index[i].offset, index[i].type);
    else
        for (size_t i = 0; i < count; ++i)
            hash = accumulate(hash, fixed[i].path, fixed[i].type);
    if (clock_gettime(CLOCK_MONOTONIC, &scan_end) != 0 || hash != expected_hash)
        goto cleanup;
    int64_t build = elapsed_ns(build_start, build_end);
    int64_t scan = elapsed_ns(scan_start, scan_end);
    if (build < 0 || scan < 0)
        goto cleanup;
    printf("cache_layout_timing count=%zu distribution=%s mode=%s round=%u build_ns=%" PRId64
           " scan_ns=%" PRId64 " digest=%" PRIx64 " PASS\n",
           count, mixed ? "mixed" : "short", packed ? "packed" : "fixed",
           round, build, scan, hash);
    result = 0;
cleanup:
    free(paths);
    free(index);
    free(fixed);
    return result;
}

static int sample(size_t count, int mixed)
{
    FileEvent *fixed = NULL;
    struct packed_event *index = NULL;
    char *paths = NULL;
    size_t path_bytes = 0;
    int result = 1;
    if (count == 0 || count > 4096U ||
        count > SIZE_MAX / sizeof *fixed || count > SIZE_MAX / sizeof *index)
        return 1;
    for (size_t i = 0; i < count; ++i)
    {
        size_t n = path_length(i, mixed);
        if (n >= sizeof fixed[0].path || n + 1U > SIZE_MAX - path_bytes)
            return 1;
        path_bytes += n + 1U;
    }
    size_t fixed_bytes = count * sizeof *fixed;
    size_t index_bytes = count * sizeof *index;
    if (path_bytes > SIZE_MAX - index_bytes)
        return 1;
    fixed = calloc(count, sizeof *fixed);
    index = calloc(count, sizeof *index);
    paths = malloc(path_bytes);
    if (!fixed || !index || !paths)
        goto cleanup;
    size_t cursor = 0;
    for (size_t i = 0; i < count; ++i)
    {
        size_t n = path_length(i, mixed);
        fixed[i].type = i % 2U ? FILE_CREATED : FILE_FOUND;
        for (size_t j = 0; j < n; ++j)
            fixed[i].path[j] = (char)path_byte(i, j);
        fixed[i].path[n] = '\0';
        index[i] = (struct packed_event){cursor, n, fixed[i].type};
        memcpy(paths + cursor, fixed[i].path, n + 1U);
        cursor += n + 1U;
    }
    if (cursor != path_bytes)
        goto cleanup;
    uint64_t fixed_hash = UINT64_C(14695981039346656037);
    uint64_t packed_hash = fixed_hash;
    cursor = 0;
    for (size_t i = 0; i < count; ++i)
    {
        size_t n = path_length(i, mixed);
        EventType type = i % 2U ? FILE_CREATED : FILE_FOUND;
        if (index[i].offset != cursor || cursor >= path_bytes ||
            index[i].length != n || n >= path_bytes - cursor ||
            index[i].type != type || fixed[i].type != type ||
            fixed[i].path[n] != '\0' || paths[cursor + n] != '\0' ||
            memcmp(fixed[i].path, paths + cursor, n + 1U) != 0)
            goto cleanup;
        for (size_t j = 0; j < n; ++j)
            if ((unsigned char)fixed[i].path[j] != path_byte(i, j))
                goto cleanup;
        fixed_hash = accumulate(fixed_hash, fixed[i].path, fixed[i].type);
        packed_hash = accumulate(packed_hash, paths + cursor, index[i].type);
        cursor += n + 1U;
    }
    if (cursor != path_bytes || fixed_hash != packed_hash)
        goto cleanup;
    printf("cache_layout count=%zu distribution=%s fixed_bytes=%zu packed_bytes=%zu index_bytes=%zu path_bytes=%zu digest=%" PRIx64 " PASS\n",
           count, mixed ? "mixed" : "short", fixed_bytes,
           index_bytes + path_bytes, index_bytes, path_bytes, fixed_hash);
    for (unsigned int round = 1; round <= 3; ++round)
        for (unsigned int position = 0; position < 2; ++position)
        {
            int packed = (int)((round - 1U + position) % 2U);
            if (measure(fixed, count, mixed, packed, round, fixed_hash) != 0)
                goto cleanup;
        }
    result = 0;
cleanup:
    free(paths);
    free(index);
    free(fixed);
    return result;
}

int main(void)
{
    const size_t counts[] = {64, 1024, 4096};
    printf("cache_layout_struct fixed_size=%zu fixed_align=%zu packed_index_size=%zu packed_index_align=%zu\n",
           sizeof(FileEvent), _Alignof(FileEvent), sizeof(struct packed_event),
           _Alignof(struct packed_event));
    for (size_t i = 0; i < sizeof counts / sizeof counts[0]; ++i)
        for (int mixed = 0; mixed <= 1; ++mixed)
            if (sample(counts[i], mixed) != 0)
                return 1;
    puts("cache_layout_reference=PASS samples=6");
    puts("cache_layout_timing=PASS samples=36");
    return 0;
}
