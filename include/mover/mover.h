// ./include/mover/mover.h
#ifndef MOVER_H
#define MOVER_H

#include <limits.h>

#include "journal/journal.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct
{
    char destination[PATH_MAX];
    int error_code;
} MoveResult;

int move_file(const char *src, const char *dest_dir);
int move_file_journaled(const char *src,
                        const char *dest_dir,
                        MoveJournal *journal,
                        MoveResult *result);

#endif
