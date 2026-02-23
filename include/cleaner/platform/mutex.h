// 📄 include/cleaner/platform/mutex.h

#ifndef CLEANER_PLATFORM_MUTEX_H
#define CLEANER_PLATFORM_MUTEX_H

typedef struct cleaner_mutex cleaner_mutex;

cleaner_mutex *cleaner_mutex_create(void);
void cleaner_mutex_lock(cleaner_mutex *m);
void cleaner_mutex_unlock(cleaner_mutex *m);
void cleaner_mutex_destroy(cleaner_mutex *m);

#endif