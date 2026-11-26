#ifndef CPU_OPT_KERNELS_H
#define CPU_OPT_KERNELS_H
#include <stddef.h>
size_t scalar_count(const unsigned char *data, size_t length, unsigned char needle);
size_t libc_count(const unsigned char *data, size_t length, unsigned char needle);
#endif
