
// // cleaner/src/threadpool.c

#include <stdlib.h>
#include <stdbool.h>

#include "cleaner/platform/threads.h"
#include "cleaner/platform/mutex.h"
#include "cleaner/platform/atomic.h"

typedef struct task
{
    void (*fn)(void *);
    void *arg;
    struct task *next;
} task;

typedef struct
{
    cleaner_thread **threads;
    int thread_count;

    task *head;
    task *tail;

    cleaner_mutex *mutex;
    cleaner_atomic_i64 stop;
} threadpool;

static void worker(void *arg)
{
    threadpool *pool = arg;

    while (1)
    {
        cleaner_mutex_lock(pool->mutex);

        task *t = pool->head;

        if (t)
        {
            pool->head = t->next;
            if (!pool->head)
                pool->tail = NULL;
        }
        else if (cleaner_atomic_load(&pool->stop))
        {
            cleaner_mutex_unlock(pool->mutex);
            break; // exit only when stop AND queue empty
        }

        cleaner_mutex_unlock(pool->mutex);

        if (t)
        {
            t->fn(t->arg);
            free(t);
        }
    }
}

// static void worker(void *arg)
// {
//     threadpool *pool = arg;

//     while (!cleaner_atomic_load(&pool->stop))
//     {
//         cleaner_mutex_lock(pool->mutex);

//         task *t = pool->head;
//         if (t)
//         {
//             pool->head = t->next;
//             if (!pool->head)
//                 pool->tail = NULL;
//         }

//         cleaner_mutex_unlock(pool->mutex);

//         if (t)
//         {
//             t->fn(t->arg);
//             free(t);
//         }
//     }
// }

threadpool *threadpool_create(int n)
{
    threadpool *p = calloc(1, sizeof(*p));

    p->thread_count = n;
    // p->threads = calloc(n, sizeof(*p->threads));
    p->threads = calloc((size_t)n, sizeof(*p->threads));
    p->mutex = cleaner_mutex_create();
    cleaner_atomic_init(&p->stop, 0);

    for (int i = 0; i < n; ++i)
        p->threads[i] = cleaner_thread_create(worker, p);

    return p;
}

void threadpool_submit(threadpool *p,
                       void (*fn)(void *),
                       void *arg)
{
    task *t = malloc(sizeof(*t));
    t->fn = fn;
    t->arg = arg;
    t->next = NULL;

    cleaner_mutex_lock(p->mutex);

    if (p->tail)
        p->tail->next = t;
    else
        p->head = t;

    p->tail = t;

    cleaner_mutex_unlock(p->mutex);
}

void threadpool_destroy(threadpool *p)
{
    cleaner_atomic_init(&p->stop, 1);

    for (int i = 0; i < p->thread_count; ++i)
    {
        cleaner_thread_join(p->threads[i]);
        cleaner_thread_destroy(p->threads[i]);
    }

    cleaner_mutex_destroy(p->mutex);
    free(p->threads);
    free(p);
}

// void threadpool_destroy(threadpool *p)
// {
//     cleaner_atomic_increment(&p->stop);

//     for (int i = 0; i < p->thread_count; ++i)
//     {
//         cleaner_thread_join(p->threads[i]);
//         cleaner_thread_destroy(p->threads[i]);
//     }

//     cleaner_mutex_destroy(p->mutex);

//     free(p->threads);
//     free(p);
// }