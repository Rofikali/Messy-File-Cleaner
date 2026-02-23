
// cleaner/include\threadpool.h
#ifndef CLEANER_THREADPOOL_H
#define CLEANER_THREADPOOL_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct ThreadPool ThreadPool;

    typedef void (*thread_task_fn)(void *arg);

    /* Create a thread pool */
    ThreadPool *threadpool_create(int thread_count);

    /* Submit a job */
    void threadpool_submit(ThreadPool *pool,
                           thread_task_fn fn,
                           void *arg);

    /* Destroy pool (waits for completion) */
    void threadpool_destroy(ThreadPool *pool);

#ifdef __cplusplus
}
#endif

#endif /* CLEANER_THREADPOOL_H */