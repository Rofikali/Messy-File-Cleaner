# FCJ3 Test-Only Durable Journal Adapter

## Status

Implementation candidate for review. This module is intentionally linked only
into `test_fcj3_journal`; it is not part of the production executable and
must not write to `journal/move.log` or create a production FCJ3 journal.

## Scope

- Opens a single journal component relative to a caller-owned directory fd.
- Requires a directory owned by the expected service UID and not group/world
  writable.
- Uses `O_APPEND | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK`, creates with mode
  0600, validates regular-file type, owner, single link, and exact 0600 mode.
- Holds a nonblocking exclusive `flock()` for the journal lifetime.
- Uses a pthread mutex to serialize complete append + fsync sequences.
- Fsyncs the parent directory after creating the journal file.
- Writes the exact prepared FCJ3 bytes using descriptor I/O with EINTR and
  short-write handling; it does not use stdio.
- Validates the prepared transition against a private context copy before
  writing, then commits the real context only after successful journal fsync.
- Poisons the real operation context after a partial/uncertain write, fsync
  failure, or stale/tampered prepared transition. It never truncates or repairs
  the journal automatically.
- Test-only fault injection covers a partial write followed by EIO and an
  fsync failure.

## Test coverage

The integration test uses disposable directories and checks:
- exact serialized bytes and parser replay;
- same-process and cross-process exclusive journal locking;
- complete non-interleaved records from two threads;
- rejection of a symlink and a file with unsafe mode;
- rejection of malformed prepared input without changing context state;
- partial-write and fsync uncertainty poison the operation context and prevent
  subsequent transitions.

## Important limits

This adapter is not a recovery implementation. A partial record can remain at
the end of a disposable test journal after an injected failure; the adapter
preserves it and refuses further writes for that operation. The future FCJ3
recovery phase must define strict tail validation and recovery behavior before
any production FCJ3 journal is enabled.

The caller must stop and join all append-capable threads before closing the
journal. A single operation context remains single-owner and is not made
thread-safe by the journal mutex. The mutex serializes the journal stream, not
concurrent mutation of one context.

## Acceptance gates

- Build and run this target under C11, C17, and C23.
- Run the existing CTest suite and sanitizer configuration.
- Verify the production target does not link `fcj3_journal.c`,
  `fcj3_writer.c`, or `fcj3_parser.c`.
- Run under the normal non-root Codespace identity.
- Keep the staging lifecycle test-only until this adapter and staging tests
  have passed review; keep production recovery disabled until its own phase.
