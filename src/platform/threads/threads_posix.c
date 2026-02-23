// // src/platform/threads_posix.c

// #include <pthread.h>
// #include <stdlib.h>

// #include "cleaner/platform/threads.h"

// struct cleaner_thread
// {
//     pthread_t thread;
// };

// typedef struct
// {
//     cleaner_thread_fn fn;
//     void *arg;
// } thread_pack;

// static void *thread_entry(void *arg)
// {
//     thread_pack *pack = arg;
//     pack->fn(pack->arg);
//     free(pack);
//     return NULL;
// }

// cleaner_thread *cleaner_thread_create(
//     cleaner_thread_fn fn,
//     void *arg)
// {
//     cleaner_thread *t = malloc(sizeof(*t));
//     thread_pack *pack = malloc(sizeof(*pack));

//     pack->fn = fn;
//     pack->arg = arg;

//     pthread_create(&t->thread,
//                    NULL,
//                    thread_entry,
//                    pack);

//     return t;
// }

// void cleaner_thread_join(cleaner_thread *t)
// {
//     pthread_join(t->thread, NULL);
// }

// void cleaner_thread_destroy(cleaner_thread *t)
// {
//     free(t);
// }

#include <pthread.h>
#include <stdlib.h>

#include "cleaner/platform/threads.h"

struct cleaner_thread
{
    pthread_t thread;
};

typedef struct
{
    cleaner_thread_fn fn;
    void *arg;
} thread_pack;

static void *thread_entry(void *arg)
{
    thread_pack *pack = arg;
    pack->fn(pack->arg);
    free(pack);
    return NULL;
}

cleaner_thread *cleaner_thread_create(
    cleaner_thread_fn fn,
    void *arg)
{
    cleaner_thread *t = malloc(sizeof(*t));
    thread_pack *pack = malloc(sizeof(*pack));

    pack->fn = fn;
    pack->arg = arg;

    pthread_create(&t->thread,
                   NULL,
                   thread_entry,
                   pack);

    return t;
}

void cleaner_thread_join(cleaner_thread *t)
{
    pthread_join(t->thread, NULL);
}

void cleaner_thread_destroy(cleaner_thread *t)
{
    free(t);
}