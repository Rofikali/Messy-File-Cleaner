/* Independent, bounded byte-count reference; not production classification. */
#define _POSIX_C_SOURCE 200809L
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include "cpu_opt_kernels.h"

static unsigned char fixture(size_t i, unsigned int pattern)
{
    if (pattern == 0U)
        return (unsigned char)'/';
    if (pattern == 1U)
        return (unsigned char)'.';
    if (pattern == 2U)
        return (unsigned char)(i % 256U);
    return (unsigned char)((i * 73U + 19U) % 256U);
}

typedef size_t (*count_fn)(const unsigned char *, size_t, unsigned char);

static int timed_count(count_fn fn, const unsigned char *data, size_t length,
                       size_t expected, unsigned int density, unsigned int round,
                       const char *name)
{
    enum { ITERATIONS = 128 };
    struct timespec begin, end;
    size_t sum = 0;
    /* Separate translation unit and volatile function pointer discourage
     * loop elimination without requiring a nonportable compiler barrier. */
    count_fn volatile call = fn;
    if (clock_gettime(CLOCK_MONOTONIC, &begin) != 0)
        return 1;
    for (unsigned int i = 0; i < ITERATIONS; ++i)
    {
        size_t actual = call(data, length, '/');
        if (actual != expected)
            return 1;
        sum += actual;
    }
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0 ||
        sum != expected * ITERATIONS)
        return 1;
    int64_t ns = (int64_t)(end.tv_sec - begin.tv_sec) * INT64_C(1000000000) +
                 (int64_t)end.tv_nsec - (int64_t)begin.tv_nsec;
    if (ns < 0)
        return 1;
    printf("cpu_opt_timing mode=%s bytes=%zu density=%u round=%u iterations=%u matches=%zu total_ns=%" PRId64 "\n",
           name, length, density, round, (unsigned int)ITERATIONS, expected, ns);
    return 0;
}

static int timing_tests(void)
{
    const size_t lengths[] = {64, 1024, 65536};
    unsigned int rows = 0;
    for (size_t n = 0; n < sizeof lengths / sizeof lengths[0]; ++n)
    {
        size_t length = lengths[n];
        unsigned char *data = malloc(length);
        if (!data)
            return 1;
        for (unsigned int density = 0; density < 3U; ++density)
        {
            size_t expected = 0;
            for (size_t i = 0; i < length; ++i)
            {
                int match = density == 2U || (density == 1U && i % 64U == 0);
                data[i] = (unsigned char)(match ? '/' : 'x');
                expected += (size_t)match;
            }
            for (unsigned int round = 0; round < 3U; ++round)
                for (unsigned int pos = 0; pos < 2U; ++pos)
                {
                    unsigned int mode = (round + pos) % 2U;
                    if (timed_count(mode ? libc_count : scalar_count, data,
                                    length, expected, density, round,
                                    mode ? "libc" : "portable") != 0)
                    {
                        free(data);
                        return 1;
                    }
                    ++rows;
                }
        }
        free(data);
    }
    printf("cpu_opt_timing=PASS rows=%u\n", rows);
    return rows == 54U ? 0 : 1;
}

int main(void)
{
    const unsigned char needles[] = {0, '/', '.', 0x80, 0xff};
    size_t cases = 0;
    if (scalar_count(NULL, 0, 0) != 0 || libc_count(NULL, 0, 0) != 0)
        return 1;
    for (size_t offset = 0; offset < 32U; ++offset)
        for (size_t length = 0; length <= 257U; ++length)
        {
            size_t bytes = offset + length;
            unsigned char *storage = malloc(bytes ? bytes : 1U);
            if (!storage)
                return 1;
            memset(storage, 0xa5, bytes ? bytes : 1U);
            unsigned char *data = storage + offset;
            for (unsigned int pattern = 0; pattern < 4U; ++pattern)
            {
                for (size_t i = 0; i < length; ++i)
                    data[i] = fixture(i, pattern);
                for (size_t n = 0; n < sizeof needles / sizeof needles[0]; ++n)
                {
                    size_t expected = libc_count(data, length, needles[n]);
                    size_t actual = scalar_count(data, length, needles[n]);
                    if (actual != expected ||
                        (pattern == 0U && expected != (needles[n] == '/' ? length : 0U)) ||
                        (pattern == 1U && expected != (needles[n] == '.' ? length : 0U)))
                    {
                        free(storage);
                        return 1;
                    }
                    ++cases;
                }
            }
            free(storage);
        }
    printf("cpu_opt_reference=PASS cases=%zu offsets=32 lengths=258 patterns=4 needles=5\n", cases);
    return cases == 32U * 258U * 4U * 5U ? timing_tests() : 1;
}
