#ifndef FCJ4_WRITER_H
#define FCJ4_WRITER_H

#include "journal/fcj4_parser.h"

#include <stddef.h>
#include <stdint.h>

/*
 * Isolated FCJ4 record construction and in-memory transition validation.
 * This module has no file descriptor, filesystem, journal, or syscall API.
 * All strings are NUL-terminated byte strings; embedded NUL is therefore
 * impossible to represent. The serializer validates every string before use.
 */

typedef struct {
    const char *operation_id;
    const char *source_path;
    uint64_t source_parent_dev, source_parent_ino;
    uint64_t source_dev, source_ino, source_uid, source_gid, source_mode, source_size;
    int64_t source_mtime_sec, source_ctime_sec;
    uint64_t source_mtime_nsec, source_ctime_nsec;
    const char *destination_path;
    uint64_t destination_parent_dev, destination_parent_ino;
    const char *destination_basename;
    uint64_t quarantine_parent_dev, quarantine_parent_ino;
} Fcj4BeginRecord;

typedef struct {
    const char *operation_id;
    const char *stage_component;
    uint64_t stage_dev, stage_ino, stage_uid, stage_mode, stage_size;
    const char *sha256;
} Fcj4SealedRecord;

typedef struct {
    const char *operation_id;
    uint64_t sequence;
    Fcj4MoveState state;
    uint32_t error_code;
} Fcj4MoveRecord;

typedef struct {
    const char *operation_id;
    const char *action_id;
    Fcj4ActionKind kind;
    Fcj4TargetRole target_role;
    uint64_t expected_parent_dev, expected_parent_ino;
    uint64_t expected_entry_dev, expected_entry_ino;
    uint64_t expected_uid, expected_mode, expected_size;
    const char *expected_sha256;
    const char *source_component;
    const char *target_component; /* NULL only for FCJ4_TARGET_NONE */
} Fcj4ActionDefRecord;

typedef struct {
    const char *operation_id;
    const char *action_id;
    uint64_t sequence;
    Fcj4ActionState state;
    uint32_t error_code;
} Fcj4ActionRecord;

typedef enum {
    FCJ4_RECORD_BEGIN = 1,
    FCJ4_RECORD_SEALED,
    FCJ4_RECORD_MOVE,
    FCJ4_RECORD_ACTION_DEF,
    FCJ4_RECORD_ACTION
} Fcj4RecordKind;

typedef struct {
    Fcj4RecordKind kind;
    union {
        Fcj4BeginRecord begin;
        Fcj4SealedRecord sealed;
        Fcj4MoveRecord move;
        Fcj4ActionDefRecord action_def;
        Fcj4ActionRecord action;
    } value;
} Fcj4Record;

/*
 * Serialize exactly one canonical LF-terminated record. No NUL terminator is
 * included in *written. On failure, *written is zero and no output is
 * promised. EINVAL means invalid values; E2BIG means the canonical line
 * exceeds the protocol limit; ENOSPC means caller capacity is insufficient.
 */
int fcj4_serialize(const Fcj4Record *record, char *output,
                   size_t capacity, size_t *written);

/* Opaque, single-owner, in-memory transition state. Not a journal writer. */
typedef struct Fcj4WriterState Fcj4WriterState;
typedef struct {
    uint64_t revision;
    Fcj4RecordKind kind;
    char operation_id[FCJ4_OPERATION_ID_HEX_LENGTH + 1U];
    char action_id[FCJ4_ACTION_ID_HEX_LENGTH + 1U];
    uint64_t sequence;
    uint64_t token;
} Fcj4PreparedRecord;

int fcj4_writer_state_create(Fcj4WriterState **out);
void fcj4_writer_state_destroy(Fcj4WriterState *state);

/*
 * Import a complete, successfully parsed replay into a fresh writer state.
 * Import is all-or-nothing and deep-copies all retained evidence; the replay
 * may be destroyed after success. The destination must be newly created and
 * have revision zero. Invalid destination/replay descriptors return EINVAL;
 * an accessor or evidence-copy failure returns EBADMSG. On every failure the
 * destination remains empty and reusable. No journal or filesystem I/O is
 * performed. The caller must pass only a replay successfully produced by
 * fcj4_replay_parse; the replay object must remain alive for the duration of
 * this call.
 */
int fcj4_writer_state_import_replay(Fcj4WriterState *state,
                                    const Fcj4Replay *replay);

/*
 * Prepare validates ordering/transitions and serializes into caller storage.
 * Exactly one prepare may be outstanding. The state does not advance here.
 * Commit requires the exact prepared bytes and length, checked against the
 * writer's one-record pending copy. It advances only in-memory state after
 * caller confirmation. This module itself has no file descriptor, filesystem,
 * journal, or syscall API. A separate Linux-only test adapter exercises the
 * durability handshake only with disposable caller-supplied test descriptors;
 * neither module enables production journal append or durable commitment.
 */
int fcj4_writer_prepare(Fcj4WriterState *state, const Fcj4Record *record,
                        char *output, size_t capacity, size_t *written,
                        Fcj4PreparedRecord *prepared);
int fcj4_writer_commit(Fcj4WriterState *state,
                       const Fcj4PreparedRecord *prepared,
                       const char *committed_bytes, size_t committed_length);
int fcj4_writer_abort_prepare(Fcj4WriterState *state,
                              const Fcj4PreparedRecord *prepared);
uint64_t fcj4_writer_revision(const Fcj4WriterState *state);

#endif
