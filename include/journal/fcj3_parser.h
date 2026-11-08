#ifndef FCJ3_PARSER_H
#define FCJ3_PARSER_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#define FCJ3_OPERATION_ID_HEX_LENGTH 32U
#define FCJ3_MAX_TRANSACTIONS 4096U
/* 261 bytes covers fixed fields, 10 maximum-width uint64 decimal fields,
 * 15 delimiters and LF; encoded paths/components add the remaining bytes. */
#define FCJ3_MAX_LINE_BYTES \
    (261U + 4U * (PATH_MAX - 1U) + 2U * (NAME_MAX - 1U))

typedef enum
{
    FCJ3_STATE_NONE = 0,
    FCJ3_STATE_STAGED = 1,
    FCJ3_STATE_STARTED,
    FCJ3_STATE_PUBLISHED,
    FCJ3_STATE_COMPLETED,
    FCJ3_STATE_FAILED,
    FCJ3_STATE_RECOVERED
} Fcj3State;

typedef struct
{
    char operation_id[FCJ3_OPERATION_ID_HEX_LENGTH + 1U];
    char source[PATH_MAX];
    char destination[PATH_MAX];
    char staging_component[NAME_MAX];
    uint64_t timestamp_ns;
    uint64_t sequence;
    int error_code;
    dev_t destination_dir_dev;
    ino_t destination_dir_ino;
    dev_t staging_dev;
    ino_t staging_ino;
    Fcj3State state;
} Fcj3Transaction;

typedef struct
{
    Fcj3Transaction *transactions;
    size_t count;
    size_t capacity;
} Fcj3Replay;

/* Parser/replay only. These functions never open, write, truncate, or unlink
 * journal or user files. The caller owns the replay object and its lifetime. */
void fcj3_replay_init(Fcj3Replay *replay);
void fcj3_replay_destroy(Fcj3Replay *replay);

/* Add exactly one complete FCJ3 line, including its final LF.
 * Returns 0 on success; on failure returns -1 and sets errno:
 * EBADMSG for malformed/illegal records, E2BIG for configured bounds,
 * ENOMEM for allocation failure.
 */
int fcj3_replay_add_line(Fcj3Replay *replay, const char *line, size_t length);

/* Shared protocol policy used by both replay and the operation-context writer. */
int fcj3_transition_is_valid(Fcj3State previous, Fcj3State next);
int fcj3_error_code_is_valid(Fcj3State state, int error_code);

#endif
