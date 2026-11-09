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

typedef enum {
    FCJ4_ACTION_KIND_QUARANTINE_STAGE = 1,
    FCJ4_ACTION_KIND_QUARANTINE_SOURCE = 2,
    FCJ4_ACTION_KIND_RETIRE_QUARANTINED_SOURCE = 3,
    FCJ4_ACTION_KIND_REMOVE_QUARANTINED_STAGE = 4
} Fcj4ActionKind;

typedef enum {
    FCJ4_TARGET_QUARANTINE = 1,
    FCJ4_TARGET_NONE = 2
} Fcj4TargetRole;

typedef struct {
    char operation_id[FCJ4_OPERATION_ID_HEX_LENGTH + 1U];
    const char *source_path;
    const char *destination_path;
    const char *destination_basename;
    const char *stage_component; /* NULL until SEALED */
    const char *stage_sha256;    /* NULL until SEALED */
    uint64_t source_parent_dev, source_parent_ino;
    uint64_t source_dev, source_ino, source_uid, source_gid, source_mode, source_size;
    int64_t source_mtime_sec, source_ctime_sec;
    uint64_t source_mtime_nsec, source_ctime_nsec;
    uint64_t destination_parent_dev, destination_parent_ino;
    uint64_t quarantine_parent_dev, quarantine_parent_ino;
    uint64_t stage_dev, stage_ino, stage_uid, stage_mode, stage_size;
    size_t action_count;
    Fcj4MoveState move_state;
} Fcj4OperationView;

typedef struct {
    char action_id[FCJ4_ACTION_ID_HEX_LENGTH + 1U];
    Fcj4ActionKind kind;
    Fcj4TargetRole target_role;
    const char *source_component;
    const char *target_component; /* NULL for FCJ4_TARGET_NONE */
    uint64_t expected_parent_dev, expected_parent_ino;
    uint64_t expected_entry_dev, expected_entry_ino;
    uint64_t expected_uid, expected_mode, expected_size;
    const char *expected_sha256;
    Fcj4ActionState state;
    uint64_t sequence;
} Fcj4ActionView;

/* Opaque, caller-owned replay result. Initialize before parse; destroy once. */
typedef struct {
    void *operations;
    size_t count;
    size_t capacity;
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

/*
 * Read-only accessors. String pointers in views remain valid until replay is
 * destroyed. Both return -1 for invalid arguments or out-of-range indexes.
 */
int fcj4_replay_operation(const Fcj4Replay *replay, size_t index,
                          Fcj4OperationView *out);
int fcj4_replay_action(const Fcj4Replay *replay, size_t operation_index,
                       size_t action_index, Fcj4ActionView *out);

#endif
