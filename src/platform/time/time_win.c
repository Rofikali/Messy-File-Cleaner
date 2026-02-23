
// 📄 src/platform/time/time_win.c

#include <windows.h>
#include "cleaner/platform/time.h"

uint64_t cleaner_time_now_ns(void)
{
    LARGE_INTEGER freq;
    LARGE_INTEGER counter;

    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);

    // return (uint64_t)((counter.QuadPart * 1000000000ULL) / freq.QuadPart);
    return ((uint64_t)counter.QuadPart * 1000000000ULL) /
           (uint64_t)freq.QuadPart;
}