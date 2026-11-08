#ifndef FCJ3_WRITER_H
#define FCJ3_WRITER_H

#include "journal/fcj3_parser.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>

typedef struct
{
    char operation_id[FCJ3_OPERATION_ID_HEX_LENGTH + 1U];
    char source[PATH_MAX];
    char destination[PATH_MAX];
    char staging_component[NAME_MAX];
    dev_t destination_dir_dev;
    ino_t destination_dir_ino;
    dev_t staging_dev;
    ino_t staging_ino;
    uint64_t sequence;
    Fcj3State state;
    int initialized;
    int poisoned;
} Fcj3OperationContext;

typedef struct
{
    char operation_id[FCJ3_OPERATION_ID_HEX_LENGTH + 1U];
    uint64_t previous_sequence;
    Fcj3State previous_state;
    uint64_t sequence;
    Fcj3State state;
    int error_code;
    uint64_t timestamp_ns;
    size_t length;
    char line[FCJ3_MAX_LINE_BYTES + 1U];
    int valid;
} Fcj3PreparedRecord;

/* Captures immutable metadata from already-validated stat snapshots.
 * The directory must be a directory; the staging inode must be a regular,
 * anonymous (st_nlink == 0) file on the same device.
 * Generates the operation ID only with Linux getrandom(); no fallback exists.
 * After initialization, treat context fields as private and serialize access
 * from one owner; the context is not internally thread-safe.
 */
int fcj3_operation_init(Fcj3OperationContext *context,
                        const char *source, const char *destination,
                        const struct stat *destination_directory,
                        const char *staging_component,
                        const struct stat *staging_inode);

void fcj3_operation_reset(Fcj3OperationContext *context);

/* Prepare a canonical FCJ3 line without advancing the context. The caller
 * must append the complete line and fsync successfully before committing it.
 */
int fcj3_operation_prepare_transition(const Fcj3OperationContext *context,
                                     Fcj3State next_state, int error_code,
                                     uint64_t timestamp_ns,
                                     Fcj3PreparedRecord *prepared);

/* Commit only after the prepared line is known to be durably appended.
 * A stale prepared record is rejected.
 */
int fcj3_operation_commit_transition(Fcj3OperationContext *context,
                                     const Fcj3PreparedRecord *prepared);

/* Call after a partial-write or fsync uncertainty. A poisoned context cannot
 * prepare or commit further transitions.
 */
void fcj3_operation_mark_uncertain(Fcj3OperationContext *context);

#ifdef FC_TESTING
void fcj3_test_set_random_error(int error_code);
void fcj3_test_set_random_chunk_limit(size_t limit);
#endif

#endif
