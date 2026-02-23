// include/cleaner/platform/time.h

#ifndef CLEANER_SYSTEM_TIME_H
#define CLEANER_SYSTEM_TIME_H

#include <stdint.h>

/*
 * Platform-independent time abstraction.
 * Implemented in platform layer.
 */

uint64_t cleaner_time_now_ns(void);

#endif