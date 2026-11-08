// ./include/mover/mover.h
#ifndef MOVER_H
#define MOVER_H

#include <limits.h>

#include "journal/journal.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#ifdef FC_TESTING
enum
{
    FC_TEST_WRITE_NONE = 0,
    FC_TEST_WRITE_EINTR_ONCE = 1,
    FC_TEST_WRITE_ENOSPC_AFTER_PARTIAL = 2
};
void fc_test_mover_set_write_fault(int fault, long bytes_before_failure);
#endif

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
