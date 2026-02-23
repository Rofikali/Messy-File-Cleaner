// include/cleaner/platform/threads.h

#ifndef CLEANER_PLATFORM_THREADS_H
#define CLEANER_PLATFORM_THREADS_H

typedef struct cleaner_thread cleaner_thread;

typedef void (*cleaner_thread_fn)(void *arg);

cleaner_thread *cleaner_thread_create(
    cleaner_thread_fn fn,
    void *arg);

void cleaner_thread_join(cleaner_thread *t);
void cleaner_thread_destroy(cleaner_thread *t);

#endif