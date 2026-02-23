
// 📄 src/platform/atomic_posix.c

#include "cleaner/platform/atomic.h"

void cleaner_atomic_init(cleaner_atomic_i64 *a, int64_t v)
{
    a->value = v;
}

int64_t cleaner_atomic_increment(cleaner_atomic_i64 *a)
{
    return __sync_add_and_fetch(&a->value, 1);
}

int64_t cleaner_atomic_load(const cleaner_atomic_i64 *a)
{
    return __sync_fetch_and_add((int64_t *)&a->value, 0);
}