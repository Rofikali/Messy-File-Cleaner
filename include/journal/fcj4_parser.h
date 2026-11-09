#ifndef FCJ4_PARSER_H
#define FCJ4_PARSER_H

#include <stddef.h>
#include <stdint.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#define FCJ4_MAX_JOURNAL_BYTES (64U * 1024U * 1024U)
#define FCJ4_MAX_LINE_BYTES (32U * 1024U)
#define FCJ4_MAX_OPERATIONS 4096U
#define FCJ4_MAX_ACTIONS_PER_OPERATION 16U
#define FCJ4_MAX_RECORDS 100000U
#define FCJ4_OPERATION_ID_HEX_LENGTH 32U
#define FCJ4_ACTION_ID_HEX_LENGTH 32U

typedef enum {
    FCJ4_MOVE_NONE = 0,
    FCJ4_MOVE_STAGED,
    FCJ4_MOVE_STARTED,
    FCJ4_MOVE_PUBLISHED,
    FCJ4_MOVE_COMPLETED,
    FCJ4_MOVE_FAILED
} Fcj4MoveState;

typedef enum {
    FCJ4_ACTION_NONE = 0,
    FCJ4_ACTION_PREPARED,
    FCJ4_ACTION_STARTED,
    FCJ4_ACTION_APPLIED,
    FCJ4_ACTION_SYNCED,
    FCJ4_ACTION_COMPLETED,
    FCJ4_ACTION_MANUAL_REVIEW
} Fcj4ActionState;

typedef struct {
    char operation_id[FCJ4_OPERATION_ID_HEX_LENGTH + 1U];
    size_t action_count;
    Fcj4MoveState move_state;
} Fcj4OperationView;

/* Opaque, caller-owned replay result. Initialize before parse; destroy once. */
typedef struct {
    void *operations;
    size_t count;
    size_t record_count;
} Fcj4Replay;

void fcj4_replay_init(Fcj4Replay *replay);
void fcj4_replay_destroy(Fcj4Replay *replay);

/*
 * Parse a complete FCJ4 journal buffer. Input is not NUL-terminated.
 * Success: returns 0 and atomically replaces an initialized, empty output.
 * Failure: returns -1, sets errno (EBADMSG malformed/inconsistent,
 * E2BIG configured bound, ENOMEM allocation failure), and leaves output empty.
 *
 * The parser is pure in-memory code: it performs no filesystem, journal,
 * network, clock, or process operations. It never exposes a valid prefix.
 */
int fcj4_replay_parse(const void *data, size_t length, Fcj4Replay *out);

/* Read-only deterministic operation-order accessor. Returns -1 out of range. */
int fcj4_replay_operation(const Fcj4Replay *replay, size_t index,
                          Fcj4OperationView *out);

#endif
