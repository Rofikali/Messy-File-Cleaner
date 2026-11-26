#include "cpu_opt_kernels.h"
#include <string.h>

size_t scalar_count(const unsigned char *data, size_t length,
                           unsigned char needle)
{
    size_t count = 0;
    for (size_t i = 0; i < length; ++i)
        count += (size_t)(data[i] == needle);
    return count;
}

/* Mature libc reference; length-bounded, including embedded NUL bytes. */
size_t libc_count(const unsigned char *data, size_t length,
                         unsigned char needle)
{
    size_t count = 0;
    while (length != 0)
    {
        const unsigned char *hit = memchr(data, needle, length);
        if (!hit)
            break;
        size_t consumed = (size_t)(hit - data) + 1U;
        ++count;
        data += consumed;
        length -= consumed;
    }
    return count;
}


/* Test-only candidates. Guarded wrappers never require optional ISA support. */
#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
#include <immintrin.h>

int cpu_has_sse2(void)
{
    __builtin_cpu_init();
    return __builtin_cpu_supports("sse2") != 0;
}

int cpu_has_avx2(void)
{
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx2") != 0;
}

__attribute__((target("sse2,no-avx,no-popcnt"), noinline))
static size_t sse2_kernel(const unsigned char *data, size_t length,
                          unsigned char needle)
{
    size_t count = 0, i = 0;
    const __m128i key = _mm_set1_epi8((char)needle);
    for (; length - i >= 16U; i += 16U)
    {
        __m128i v;
        memcpy(&v, data + i, sizeof v);
        const unsigned int mask = (unsigned int)_mm_movemask_epi8(_mm_cmpeq_epi8(v, key));
        count += (size_t)__builtin_popcount(mask);
    }
    for (; i < length; ++i)
        count += (size_t)(data[i] == needle);
    return count;
}

__attribute__((target("avx2,no-popcnt"), noinline))
static size_t avx2_kernel(const unsigned char *data, size_t length,
                          unsigned char needle)
{
    size_t count = 0, i = 0;
    const __m256i key = _mm256_set1_epi8((char)needle);
    for (; length - i >= 32U; i += 32U)
    {
        __m256i v;
        memcpy(&v, data + i, sizeof v);
        const unsigned int mask = (unsigned int)_mm256_movemask_epi8(_mm256_cmpeq_epi8(v, key));
        count += (size_t)__builtin_popcount(mask);
    }
    for (; i < length; ++i)
        count += (size_t)(data[i] == needle);
    return count;
}

size_t sse2_count(const unsigned char *data, size_t length, unsigned char needle)
{
    return cpu_has_sse2() ? sse2_kernel(data, length, needle)
                          : scalar_count(data, length, needle);
}

size_t avx2_count(const unsigned char *data, size_t length, unsigned char needle)
{
    return cpu_has_avx2() ? avx2_kernel(data, length, needle)
                          : scalar_count(data, length, needle);
}
#else
int cpu_has_sse2(void) { return 0; }
int cpu_has_avx2(void) { return 0; }
size_t sse2_count(const unsigned char *data, size_t length, unsigned char needle)
{
    return scalar_count(data, length, needle);
}
size_t avx2_count(const unsigned char *data, size_t length, unsigned char needle)
{
    return scalar_count(data, length, needle);
}
#endif

/* Pure policy: only the intersection of known detected and allowed bits. */
enum cpu_backend cpu_choose_backend(unsigned int detected, unsigned int allowed)
{
    const unsigned int usable = detected & allowed & CPU_ALLOW_ALL;
    if (usable & CPU_ALLOW_AVX2)
        return CPU_BACKEND_AVX2;
    if (usable & CPU_ALLOW_SSE2)
        return CPU_BACKEND_SSE2;
    return CPU_BACKEND_PORTABLE;
}

/* Select once, before publishing a function pointer to consumers.
 * The real detector is never replaced with caller-provided capabilities. */
cpu_count_fn cpu_select_count(unsigned int allowed, enum cpu_backend *selected)
{
    unsigned int detected = 0;
    if (cpu_has_sse2())
        detected |= CPU_ALLOW_SSE2;
    if (cpu_has_avx2())
        detected |= CPU_ALLOW_AVX2;
    const enum cpu_backend backend = cpu_choose_backend(detected, allowed);
    if (selected)
        *selected = backend;
#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
    if (backend == CPU_BACKEND_AVX2)
        return avx2_kernel;
    if (backend == CPU_BACKEND_SSE2)
        return sse2_kernel;
#endif
    return scalar_count;
}
