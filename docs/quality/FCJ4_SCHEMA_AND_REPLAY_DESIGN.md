# FCJ4 Schema and Replay Design

## Status

Wire contract approved by merged PRs #61 and #62. The isolated pure-C parser/replay implementation is proposed separately in PR #63 and is not yet merged or production-enabled. This document does not authorize filesystem mutation, FCJ3 production writes, source retirement, staging cleanup, quarantine, or production adoption.

Parent architecture decision: `FCJ4_RECOVERY_EXECUTION_SAFETY_DESIGN.md` (merged PR #60).

ACTION_DEF wire clarifications: `FCJ4_ACTION_DEF_WIRE_CLARIFICATIONS.md` (merged PR #62).

## 1. Decision summary

FCJ4 is a new, append-only, line-oriented journal protocol. It uses a distinct file identity and parser. It never shares `journal/move.log` with FCJ2/legacy and never appends to an FCJ3 journal.

The protocol separates three concepts:

1. **Transaction identity/evidence** — immutable facts about source, destination, staging, and the designated quarantine directory.
2. **Move lifecycle** — copy/publication progress.
3. **Recovery action lifecycle** — one separately identified, idempotent action proposal and its durable execution progress.

A move state is never used to imply that a recovery action occurred. A parser validates history; it does not decide that an action is safe to execute.

The first implementation milestone is parser/replay only. No writer, filesystem adapter, planner, executor, or production integration is included.

## 2. File and record framing

- File name reserved for future adoption: `journal/move.fcj4`. This name is not created by this milestone.
- Encoding: ASCII bytes only; one record per line; LF (`0x0a`) terminator required.
- No BOM, CRLF, embedded NUL, whitespace padding, or blank lines.
- Delimiter: ASCII vertical bar (`|`).
- Every record begins with `FCJ4`, a record-kind token, and schema version `1`.
- Field count is exact for each record kind. Empty fields are rejected unless explicitly specified as an optional field in a future schema version.
- Unknown record kinds and unsupported schema versions fail closed. No skip-unknown behavior in version 1.
- A final record without LF is a truncated journal and invalidates the complete replay. A valid prefix must never be returned as a successful replay.

### 2.1 Canonical scalar encodings

- Unsigned integers: base-10 ASCII, no sign, no leading zero except the single value `0`; checked against the destination type before conversion.
- Signed timestamps: base-10 ASCII; no leading plus; no leading zero except `0`; negative values use one leading minus and cannot be `-0`.
- Enumerations: exact uppercase tokens defined below.
- IDs: 128-bit cryptographically random values encoded as exactly 32 lowercase hexadecimal characters. Generation belongs to a future writer and must fail closed if the OS random source fails.
- SHA-256: exactly 64 lowercase hexadecimal characters. Algorithm is fixed by schema version 1; it is an integrity comparison, not authentication.
- Paths and components: byte strings encoded as lowercase hexadecimal. The decoded length is stored in a preceding field and must match exactly. Decoded values cannot contain NUL or control bytes. Paths must be absolute and are diagnostic/re-resolution inputs, not object identity.
- Binary fields never use locale-sensitive text or case-insensitive parsing.

Hex encoding avoids delimiter ambiguity and makes escaping canonical. It increases record size; all limits below account for worst-case encoding.

## 3. Record kinds and exact field order

All fields below are separated by `|`. The first three fields are always `FCJ4|KIND|1`.

### 3.1 BEGIN — immutable pre-copy intent

```text
FCJ4|BEGIN|1|operation_id
|source_path_len|source_path_hex
|source_parent_dev|source_parent_ino
|source_dev|source_ino|source_type|source_uid|source_gid|source_mode|source_size
|source_mtime_sec|source_mtime_nsec|source_ctime_sec|source_ctime_nsec
|destination_path_len|destination_path_hex
|destination_parent_dev|destination_parent_ino
|destination_basename_len|destination_basename_hex
|quarantine_parent_dev|quarantine_parent_ino
```

The line above is a field-layout illustration; the actual record is one physical line.

Rules:
- One BEGIN record per operation ID, and it must be the first record for that operation.
- Source and destination parents must be opened and validated by the future caller before BEGIN is written. The recorded device/inode pairs bind the transaction to those directory objects.
- Source must be a regular file. `source_type` is the canonical token `REGULAR` in version 1; other types are rejected.
- Source size is an unsigned 64-bit byte count.
- mtime/ctime seconds are signed 64-bit values; nanoseconds are unsigned values in [0, 999999999].
- Destination basename is one nonempty component; reject `.`, `..`, slash, NUL, and controls.
- Quarantine parent must be a pre-provisioned private directory on the supported destination filesystem. Its owner/mode policy is fixed by the future executor contract; directory device/inode identity is persisted here.
- BEGIN does not claim a content digest or stage inode exists.

### 3.2 SEALED — immutable post-copy evidence

```text
FCJ4|SEALED|1|operation_id
|stage_component_len|stage_component_hex
|stage_dev|stage_ino|stage_uid|stage_mode|stage_size
|sha256
```

Rules:
- Exactly one SEALED record follows BEGIN and precedes every MOVE or ACTION record for the operation.
- Stage component is a single nonempty component in the reserved journal-owned namespace. The name alone never proves ownership.
- Stage must be a regular inode on the recorded destination-parent device, with the required restrictive mode and expected service owner.
- `stage_size` must equal the copied byte count and the BEGIN source size under the version-1 full-copy contract.
- `sha256` is computed over the exact bytes successfully copied to staging. The source descriptor and source directory entry must be revalidated after copy against BEGIN metadata; a mismatch prevents SEALED.
- Expected destination inode identity is the stage device/inode pair. Published content is checked against the persisted digest.
- SEALED is immutable. A second SEALED record is invalid.

### 3.3 MOVE — move lifecycle transition

```text
FCJ4|MOVE|1|operation_id|sequence|state|error_code
```

Version-1 states and allowed transitions:

| Previous | Next |
|---|---|
| no MOVE state | STAGED |
| STAGED | STARTED, FAILED |
| STARTED | PUBLISHED, FAILED |
| PUBLISHED | COMPLETED |

Rules:
- Sequence begins at 1 and increments by exactly 1 for each operation.
- STAGED is permitted only after BEGIN and SEALED.
- FAILED requires a positive errno value and is terminal. It is valid only where the caller can prove no publication occurred; uncertain side effects must not be represented as FAILED.
- All other states require error_code 0.
- COMPLETED requires the move success postconditions specified by the later writer/executor design. It does not by itself authorize source retirement.
- No RECOVERED move state exists in FCJ4 version 1. Recovery progress is represented only by ACTION records.
- Terminal move states cannot have later MOVE transitions.

### 3.4 ACTION_DEF — immutable recovery-action intent

```text
FCJ4|ACTION_DEF|1|operation_id|action_id|action_kind|target_role
|expected_parent_dev|expected_parent_ino
|expected_entry_dev|expected_entry_ino|expected_entry_type
|expected_uid|expected_mode|expected_size|expected_sha256
|source_component_len|source_component_hex
|target_component_len|target_component_hex
```

Rules:
- ACTION_DEF is permitted only after the operation has a complete BEGIN and SEALED record.
- action_id is a unique 128-bit ID within the operation; duplicate action IDs are invalid.
- action_kind is one of `QUARANTINE_STAGE`, `QUARANTINE_SOURCE`, `RETIRE_QUARANTINED_SOURCE`, or `REMOVE_QUARANTINED_STAGE`.
- target_role is one of `SOURCE`, `STAGE`, `DESTINATION`, or `QUARANTINE`.
- Version 1 action definitions must specify exact expected parent and entry identity, type, owner, mode, size, and digest. A field not meaningful for an action must use the explicitly specified sentinel `0`; empty fields are not used.
- Source/target components are exact single-component names, never paths. The target component is reserved in the already validated quarantine directory for quarantine actions.
- Action definitions are immutable. A conflicting duplicate is invalid, even if the action ID matches.
- This schema record does not authorize execution. A future planner must independently prove that the action is permitted for the current move state and evidence.

### 3.5 ACTION — recovery-action lifecycle transition

```text
FCJ4|ACTION|1|operation_id|action_id|sequence|state|error_code
```

Version-1 states and allowed transitions:

| Previous | Next |
|---|---|
| no ACTION state | ACTION_PREPARED |
| ACTION_PREPARED | ACTION_STARTED, ACTION_MANUAL_REVIEW |
| ACTION_STARTED | ACTION_APPLIED, ACTION_MANUAL_REVIEW |
| ACTION_APPLIED | ACTION_SYNCED, ACTION_MANUAL_REVIEW |
| ACTION_SYNCED | ACTION_COMPLETED, ACTION_MANUAL_REVIEW |

Rules:
- ACTION_DEF must precede the first ACTION transition.
- Sequence begins at 1 and increments by exactly 1 for each action ID.
- ACTION_PREPARED means immutable action parameters were validated and the intent record is durable. It is not permission to mutate.
- ACTION_STARTED is the durable pre-mutation boundary. The future executor must revalidate all relevant descriptors, parent identities, entry identities, content, and action parameters immediately before mutation.
- ACTION_APPLIED means the requested namespace operation returned success and its resulting entry identity was verified. It does not mean the affected directory is durable.
- ACTION_SYNCED means all required affected-directory fsync operations succeeded.
- ACTION_COMPLETED means postconditions were rechecked and the completion record was durably appended.
- ACTION_MANUAL_REVIEW is terminal, requires a positive error_code, and prohibits automatic continuation for that action ID.
- All non-error states require error_code 0.
- A syscall success followed by uncertain verification, fsync, or journal append must never be blindly repeated. Replay and fresh evidence are required; if the exact postcondition cannot be proven, manual review is mandatory.

## 4. Replay and ordering contract

- Records from different operation IDs may be interleaved; each operation's own history must still obey the ordering rules above.
- BEGIN must precede all records for its operation. SEALED must follow BEGIN. MOVE and ACTION_DEF/ACTION records require SEALED.
- ACTION records require a prior ACTION_DEF for the same operation/action ID.
- One operation may contain at most 16 action definitions in version 1. An action ID is unique within its operation.
- The parser preserves first-seen operation order and first-seen action-definition order for deterministic reporting.
- Duplicate BEGIN, duplicate SEALED, conflicting immutable metadata, duplicate/non-increasing sequence, skipped sequence, illegal transition, unknown operation/action ID, and records after a terminal state are invalid.
- Parser output is all-or-nothing: on any invalid record or incomplete tail, no successful replay object is exposed.
- Replay is pure in-memory validation. It performs no file opens, filesystem observations, journal writes, repair, truncation, or recovery actions.
- Parser/replay must not decide that a filesystem action is safe. It only validates the persisted protocol history.

## 5. Resource limits

Version 1 limits:
- Maximum journal bytes: 64 MiB.
- Maximum physical record bytes including LF: 32 KiB.
- Maximum operations: 4096.
- Maximum action definitions per operation: 16.
- Maximum total records: 100,000.
- Maximum decoded path bytes: PATH_MAX - 1 for each path.
- Maximum decoded component bytes: NAME_MAX - 1.
- All counts, buffer growth, multiplication, and integer conversions use checked arithmetic.
- Hitting a limit returns a distinct bounded-resource error (E2BIG or the module's documented equivalent); it must not return a partial replay.
- The replay implementation must have a documented upper bound on allocated memory. A later implementation review must calculate the worst-case memory footprint before choosing in-memory record representations.

## 6. Compatibility and adoption

- FCJ2/legacy parser and replay remain byte-for-byte behaviorally unchanged.
- FCJ3 parser, writer, report, and diagnostic behavior remain unchanged.
- FCJ4 uses a separate parser API and separate file path. No format sniffing in the FCJ2 parser and no mixed-format journal.
- No automatic migration, repair, truncation, compaction, or journal rewrite.
- No production writer, journal creation, mover integration, or recovery executor in the parser milestone.
- A future production-adoption ADR must define journal directory ownership/mode, single-writer exclusion, append durability, backup/restore, rotation, corruption response, and supported filesystems.

## 7. Security and correctness requirements

- Reject noncanonical encodings; never normalize hostile journal input into an accepted record.
- Reject embedded NUL, controls, path traversal components, invalid absolute paths, malformed hex, invalid digest length, invalid enum values, and out-of-range timestamps/nanoseconds.
- Check integer conversion to platform types; persistent numeric fields remain fixed-width on disk.
- No path from a record is opened by the parser.
- No string field is used as a filesystem authorization decision.
- SHA-256 is an integrity comparison only. It is not a MAC, signature, or defense against an attacker able to rewrite both journal and filesystem state.
- A valid replay is not proof that the journal is authentic, that the filesystem matches it, or that recovery is safe.
- Same-UID and privileged-attacker limitations remain explicit in the threat model.

## 8. Parser milestone acceptance criteria

The first implementation milestone after this design is merged may add only:
- `include/journal/fcj4_parser.h`;
- `src/journal/fcj4_parser.c`;
- focused unit tests and fixtures;
- CMake/CTest registration for the isolated parser test;
- this document's implementation-status update, if needed.

It must not add a writer, CLI, filesystem adapter, planner, executor, mutation syscall, production source-list entry, or create `journal/move.fcj4`.

Required tests:
- valid BEGIN/SEALED/MOVE history;
- valid interleaved operations and multiple independent actions;
- every legal transition and every illegal transition;
- missing/reordered/duplicate BEGIN and SEALED;
- duplicate action definitions and unknown action IDs;
- malformed field counts, unknown kinds/versions, noncanonical integers, overflow, malformed/lowercase-hex violations, NUL/control bytes, path/component traversal;
- invalid timestamps/nanoseconds, digest lengths, sentinel misuse, and unsupported enum values;
- truncated final record, oversized line/journal, record/operation/action limits;
- duplicate and skipped sequences, post-terminal transitions, conflicting immutable metadata;
- all-or-nothing replay and deterministic ordering;
- C11/C17/C23 Debug and Release, sanitizers, static analysis, Docker/non-root workflow compatibility;
- existing FCJ2/legacy and FCJ3 tests remain unchanged and passing.

The parser API must document ownership, lifetime, bounds, errno/status behavior, and all-or-nothing semantics. Tests must prove parsing has no filesystem or journal side effects.

## 9. Explicit non-goals

- No FCJ4 writer or production journal file.
- No FCJ3 production writes or executor.
- No filesystem mutation, source retirement, staging cleanup, quarantine, rename, link, unlink, overwrite, chmod, truncate, or journal repair.
- No changes to FCJ2/legacy behavior.
- No C++, Rust core, broker, database, daemon, or microservice.
- No claim of atomic journal snapshots or protection against privileged/same-UID journal tampering.

## 10. Rollback

This design milestone adds one document only. Rollback is removal of `docs/quality/FCJ4_SCHEMA_AND_REPLAY_DESIGN.md`. No executable behavior, journal compatibility, or filesystem state changes.
