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

#ifdef FC_TESTING
enum
{
    FC_TEST_WRITE_NONE = 0,
    FC_TEST_WRITE_EINTR_ONCE = 1,
    FC_TEST_WRITE_ENOSPC_AFTER_PARTIAL = 2,
    FC_TEST_WRITE_EIO_AFTER_PARTIAL = 3
};

enum
{
    FC_TEST_CRASH_NONE = 0,
    FC_TEST_CRASH_AFTER_STARTED = 1,
    FC_TEST_CRASH_AFTER_PUBLISHED = 2,
    FC_TEST_CRASH_AFTER_SOURCE_RETIRED = 3
};
void fc_test_mover_stop_at_crash_stage(int stage);
void fc_test_mover_set_write_fault(int fault, long bytes_before_failure);
void fc_test_mover_set_temp_create_error(int error_code);
/* fsync call order: 1=temp file, 2=destination directory, 3=source directory. */
void fc_test_mover_set_fsync_fault(unsigned int call_number, int error_code);
void fc_test_mover_set_publish_error(int error_code);
void fc_test_mover_set_retire_error(int error_code);
#endif

#endif
