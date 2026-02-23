
// // src/cleaner/platform/threads_win.c

#include <windows.h>
#include <stdlib.h>

#include "cleaner/platform/threads.h"

/* ============================================================
 * Internal Types
 * ============================================================ */

struct cleaner_thread
{
    HANDLE handle;
};

typedef struct
{
    cleaner_thread_fn fn;
    void *arg;
} thread_start_pack;

/* ============================================================
 * Thread Entry
 * ============================================================ */

static DWORD WINAPI thread_entry(LPVOID arg)
{
    thread_start_pack *pack = (thread_start_pack *)arg;

    cleaner_thread_fn fn = pack->fn;
    void *real_arg = pack->arg;

    free(pack);

    fn(real_arg);
    return 0;
}

/* ============================================================
 * Public API
 * ============================================================ */

cleaner_thread *cleaner_thread_create(
    cleaner_thread_fn fn,
    void *arg)
{
    cleaner_thread *t = malloc(sizeof(*t));
    if (!t)
        return NULL;

    thread_start_pack *pack = malloc(sizeof(*pack));
    if (!pack)
    {
        free(t);
        return NULL;
    }

    pack->fn = fn;
    pack->arg = arg;

    t->handle = CreateThread(
        NULL,
        0,
        thread_entry,
        pack,
        0,
        NULL);

    if (!t->handle)
    {
        free(pack);
        free(t);
        return NULL;
    }

    return t;
}

void cleaner_thread_join(cleaner_thread *t)
{
    if (!t)
        return;

    WaitForSingleObject(t->handle, INFINITE);
}

void cleaner_thread_destroy(cleaner_thread *t)
{
    if (!t)
        return;

    CloseHandle(t->handle);
    free(t);
}