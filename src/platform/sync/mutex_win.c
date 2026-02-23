// 📄 src/platform/mutex_win.c

#include <windows.h>
#include <stdlib.h>
#include "cleaner/platform/mutex.h"

struct cleaner_mutex
{
    CRITICAL_SECTION cs;
};

cleaner_mutex *cleaner_mutex_create(void)
{
    cleaner_mutex *m = malloc(sizeof(*m));
    InitializeCriticalSection(&m->cs);
    return m;
}

void cleaner_mutex_lock(cleaner_mutex *m)
{
    EnterCriticalSection(&m->cs);
}

void cleaner_mutex_unlock(cleaner_mutex *m)
{
    LeaveCriticalSection(&m->cs);
}

void cleaner_mutex_destroy(cleaner_mutex *m)
{
    DeleteCriticalSection(&m->cs);
    free(m);
}