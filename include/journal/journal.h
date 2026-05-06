#ifndef JOURNAL_H
#define JOURNAL_H

#include <limits.h>
#include <stdio.h>
#include <pthread.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct
{
    FILE *fp;
    pthread_mutex_t lock;
    char path[PATH_MAX];
    int enabled;
} MoveJournal;

int journal_recover(const char *path);
int journal_open(MoveJournal *journal, const char *path);
void journal_close(MoveJournal *journal);
int journal_record_started(MoveJournal *journal, const char *src, const char *dest);
int journal_record_completed(MoveJournal *journal, const char *src, const char *dest);
int journal_record_failed(MoveJournal *journal, const char *src, const char *dest, int error_code);

#endif
