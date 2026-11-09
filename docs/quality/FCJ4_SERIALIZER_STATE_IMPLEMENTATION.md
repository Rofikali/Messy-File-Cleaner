# FCJ4 Serializer and In-Memory State Implementation

## Status

Implementation proposal for the merged FCJ4 writer/serialization design (PR #65). This milestone adds only an isolated pure-C serializer and in-memory transition guard. It does not implement a journal writer or any persistence adapter.

## Included

- `include/journal/fcj4_writer.h`: typed inputs for BEGIN, SEALED, MOVE, ACTION_DEF, and ACTION; serializer and opaque in-memory state API.
- `src/journal/fcj4_writer.c`: canonical ASCII/LF serialization, bounded output, scalar and component validation, and prepare/commit/abort state transitions.
- `tests/unit/test_fcj4_writer.c`: parser round-trip for a complete valid record sequence, prepared-state behavior, stale/duplicate commit rejection, and serializer error cases.
- An isolated CTest target that links the writer module only into its unit test, never into the production executable.

## State and commit semantics

The state object is single-owner and not thread-safe. It retains a bounded table for at most 4096 operation IDs and 16 action IDs per operation, plus one prepared record no larger than the protocol line limit. Only one prepared record may be outstanding. Prepare validates local ordering and legal move/action transitions, serializes the record, retains one bounded copy of its bytes, and returns a token tied to the current revision. Prepare does not advance state. Commit requires the exact byte sequence and length to match the retained preparation before advancing in-memory state once; abort discards the outstanding preparation. Reusing or committing a stale token is rejected.

**Important:** this milestone has no append adapter. Calling `fcj4_writer_commit` is only the caller's explicit assertion that its external boundary accepted the exact prepared bytes. The API does not prove that bytes were written, synchronized, or durable. It must not be connected to production or described as a durable journal commit. A later append-adapter milestone must bind the token to the exact bytes, poison on uncertain writes/fsync, and advance state only after successful fsync.

## Validation boundary

`fcj4_serialize` validates each record's own fields and canonical encoding. Cross-record invariants (for example, action identity binding to BEGIN/SEALED evidence and the full journal-level quarantine-action relationship) remain authoritatively checked by the existing whole-buffer parser. Callers must round-trip complete proposed histories through `fcj4_replay_parse` before treating them as protocol-valid. This isolated API does not authorize an append or filesystem action.

## Explicit non-goals

- No file open/create/append, `write`, `fsync`, `fdatasync`, lock, close, or journal path.
- No FCJ4 production source-list integration or CLI exposure.
- No FCJ2/legacy or FCJ3 changes.
- No planner, executor, filesystem adapter, recovery action, or user-file mutation.
- No claim of journal authenticity, filesystem truth, or durable commitment.

## Acceptance evidence required

The milestone is not complete until CI and owner-run validation demonstrate:
- focused FCJ4 writer and parser tests pass;
- full `make test` and `make validate` pass (C11/C17/C23);
- Release build/test and configured Docker/non-root workflows pass;
- serializer output is accepted by the current parser;
- production executable remains free of the FCJ4 writer module;
- no file-system or journal side effects are introduced.
