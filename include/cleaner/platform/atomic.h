
// 📄 include/cleaner/platform/atomic.h

#ifndef CLEANER_PLATFORM_ATOMIC_H
#define CLEANER_PLATFORM_ATOMIC_H

#include <stdint.h>

typedef struct
{
    volatile int64_t value;
} cleaner_atomic_i64;

void cleaner_atomic_init(cleaner_atomic_i64 *a, int64_t v);
int64_t cleaner_atomic_increment(cleaner_atomic_i64 *a);
int64_t cleaner_atomic_load(const cleaner_atomic_i64 *a);

#endif