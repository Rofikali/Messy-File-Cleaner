#ifndef CPU_OPT_KERNELS_H
#define CPU_OPT_KERNELS_H
#include <stddef.h>
size_t scalar_count(const unsigned char *data, size_t length, unsigned char needle);
size_t libc_count(const unsigned char *data, size_t length, unsigned char needle);
int cpu_has_sse2(void);
int cpu_has_avx2(void);
size_t sse2_count(const unsigned char *data, size_t length, unsigned char needle);
size_t avx2_count(const unsigned char *data, size_t length, unsigned char needle);
/* Allowed masks can restrict, never elevate, detected capabilities. */
enum cpu_backend {
    CPU_BACKEND_PORTABLE = 0,
    CPU_BACKEND_SSE2 = 1,
    CPU_BACKEND_AVX2 = 2
};
enum { CPU_ALLOW_SSE2 = 1U, CPU_ALLOW_AVX2 = 2U, CPU_ALLOW_ALL = 3U };
typedef size_t (*cpu_count_fn)(const unsigned char *, size_t, unsigned char);
enum cpu_backend cpu_choose_backend(unsigned int detected, unsigned int allowed);
cpu_count_fn cpu_select_count(unsigned int allowed, enum cpu_backend *selected);
#endif
