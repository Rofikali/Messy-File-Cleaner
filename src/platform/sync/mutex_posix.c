// 📄 src/platform/mutex_posix.c

#include <pthread.h>
#include <stdlib.h>

#include "cleaner/platform/mutex.h"

struct cleaner_mutex {
    pthread_mutex_t mutex;
};

cleaner_mutex *cleaner_mutex_create(void)
{
    cleaner_mutex *m = malloc(sizeof(*m));
    pthread_mutex_init(&m->mutex, NULL);
    return m;
}

void cleaner_mutex_lock(cleaner_mutex *m)
{
    pthread_mutex_lock(&m->mutex);
}

void cleaner_mutex_unlock(cleaner_mutex *m)
{
    pthread_mutex_unlock(&m->mutex);
}

void cleaner_mutex_destroy(cleaner_mutex *m)
{
    pthread_mutex_destroy(&m->mutex);
    free(m);
}