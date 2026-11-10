# FCJ4 Test-Only Append Adapter Design

## Status

Proposed design only. This is a focused implementation gate following the merged FCJ4 schema/replay, parser, serializer, in-memory writer, and writer-state reconciliation milestones.

This document authorizes no implementation by itself. A separately reviewed implementation PR is required. The only proposed adapter is a **test-only adapter for disposable temporary journal files**. It must not open, create, or append to `journal/move.fcj4`, link into the production executable, or enable FCJ4 in the mover or CLI.

## 1. Objective

Exercise the durability boundary between the existing in-memory FCJ4 writer and an operating-system file using a narrow, injectable Linux/POSIX adapter. Prove that short writes, EINTR, partial failures, fsync uncertainty, and cleanup failures cannot silently advance writer state or permit unsafe subsequent appends.

The adapter exists to test protocol semantics, not to deliver production journal adoption.

## 2. Scope and explicit non-goals

### Included in the future implementation proposal

- A private/test-only append adapter operating only on an explicitly supplied descriptor for a disposable temporary file.
- A narrow injected I/O operations table for write, fsync, close, lock, unlock, fstat, and lseek/offset inspection; production code must not gain global fault-injection switches.
- A deterministic append state machine connecting prepared bytes to the existing in-memory writer state.
- Test fixtures using temporary directories/files created by the test harness, with strict cleanup and no user data.
- Fault-injection, restart/replay, and process-crash simulation tests.
- Documentation of the exact kernel, filesystem, container, and CI environment exercised.

### Explicitly excluded

- Opening or creating `journal/move.fcj4` or any application journal path.
- Any production executable, CLI, mover, startup, or recovery integration.
- FCJ2/legacy or FCJ3 changes.
- Production journal directory creation, ownership/mode policy, lock lifecycle, or stable public ABI.
- Filesystem evidence capture, planning, execution, rename, unlink, quarantine, publication, source retirement, cleanup, or recovery mutation.
- Journal rotation, compaction, repair, truncation, migration, automatic tail recovery, or multi-writer/network-filesystem support.
- Claims of power-loss guarantees based only on process termination tests.

## 3. Architectural boundary

Keep the responsibilities separate:

1. **FCJ4 serializer and writer state** validate and prepare one canonical record. They remain usable without filesystem access.
2. **Test append adapter** receives an exact prepared byte sequence and writes it to a caller-supplied disposable journal descriptor.
3. **Durability result** determines whether the in-memory prepared transition may be committed, must remain uncommitted, or has an uncertain outcome that poisons the adapter session.
4. **Pure parser/replay** is used after close/reopen or simulated restart to inspect the complete journal.

The adapter must not construct records, infer operation state, inspect source/stage/destination files, or perform any recovery action. It must not treat parser validity as authenticity or filesystem truth.

## 4. Proposed private interface

The implementation proposal should define an opaque test adapter context and a private operations table. Exact names and ABI are not frozen here.

The interface must support:

- Initialize from a caller-supplied already-open disposable journal descriptor and an explicit expected file identity.
- Append one already-prepared record with an explicit byte pointer and length.
- Return a structured result distinguishing:
  - rejected before any write;
  - complete write plus successful fsync (durable);
  - write outcome uncertain (poisoned);
  - fsync outcome uncertain (poisoned);
  - durable record with close/unlock cleanup failure.
- Destroy the context without silently retrying, truncating, repairing, or mutating journal bytes.

The adapter must never own or modify the in-memory writer's private state. The caller may call `fcj4_writer_commit()` only after the adapter reports complete write plus successful fsync. On uncertain write/fsync, the prepared writer state must not be committed and the adapter must reject future appends until a new context is initialized from a complete replay.

The operations table is private to tests/adapter implementation. It must be immutable for a context lifetime, explicitly passed, and contain no process-global mutable fault controls.

## 5. File and descriptor preconditions

For this test-only milestone:

- The test harness creates a fresh temporary directory and journal file with exclusive creation.
- The adapter receives the descriptor; it does not resolve a pathname or create a file.
- The descriptor must refer to a regular file. Capture device/inode identity at initialization and verify it before append.
- Verify expected file size/offset before each append; unexpected identity, size, or offset fails closed before writing.
- Do not follow symlinks in test setup; use descriptor-based operations after setup.
- Use close-on-exec where available.
- Keep the test file private to the test process; test locking semantics explicitly if a lock is part of the adapter contract.
- The test harness owns cleanup of its temporary directory after all descriptors are closed. Adapter failure must never trigger automatic truncation or repair.

This precondition is intentionally weaker than a future production journal-directory policy because the test adapter is not a production security boundary. The implementation PR must not generalize it into production use.

## 6. Append and durability algorithm

For each prepared record:

1. Reject if context is poisoned, input is null/empty/oversized, or the prepared line is not exactly one canonical LF-terminated record within protocol limits.
2. Verify the descriptor still identifies the expected regular file and that current size/offset equals the context's known committed length.
3. Enter the appending state; reject reentrant or concurrent calls.
4. Write the exact bytes in a loop:
   - retry EINTR without changing byte order;
   - correctly handle short positive writes;
   - treat zero-byte write as failure;
   - treat any non-retryable error as uncertain once append has begun.
5. If all bytes are written, call `fsync()`.
6. Only a successful `fsync()` returns the durable result and advances the adapter's known length.
7. On any write failure after the first write attempt, or any fsync failure, poison the context. Do not retry the whole record, truncate a partial tail, append a compensating record, or accept a valid prefix.
8. If fsync succeeded but close/unlock later fails during teardown, preserve the durable result and report cleanup/ownership failure distinctly. Do not report the durable record as absent.
9. Preserve the primary system error across cleanup.

The adapter must not call `fdatasync()` as a fallback. No success is reported merely because write returned a complete byte count.

## 7. Failure-state contract

| Failure point | Record result | Context |
|---|---|---|
| Input/state/file-identity rejection before write | Not appended | Not poisoned if identity/state remains known; otherwise unusable |
| EINTR | Retry same remaining bytes | Appending |
| Short positive write | Continue from exact unwritten suffix | Appending |
| Zero write or non-retryable write error | Uncertain; do not commit writer state | Poisoned |
| fsync error after full write | Uncertain; do not commit writer state | Poisoned |
| fsync success | Durable; caller may commit writer state | Ready for next record |
| close/unlock failure after fsync success | Durable record plus cleanup error | Unusable until ownership safely resolved |

The implementation must use explicit result codes and retain the primary errno. Errno alone must not encode the adapter state.

## 8. Replay reconciliation

After a poisoned outcome or simulated restart:

1. Stop appending and preserve the journal bytes exactly.
2. Close the old descriptor/context without truncation or repair.
3. Reopen the same disposable file under the test harness's exclusive test lock.
4. Verify file identity and read the complete bounded file.
5. Parse the entire file with `fcj4_replay_parse()`; do not accept a valid prefix.
6. If replay fails, preserve the bytes and require manual/test-harness diagnosis. Do not append.
7. If replay succeeds, create a fresh in-memory writer state from that replay.
8. Resume only from the parsed history and a newly initialized adapter context after identity/offset checks.

A failed fsync may leave the complete record visible or absent after a crash. The test must not assume either outcome. The complete replay is the only source of protocol history after restart.

## 9. Fault-injection matrix

The injected operations table must allow deterministic failure at each relevant call and byte boundary.

Required cases:

- EINTR before any bytes are written.
- EINTR after one or more short writes.
- One-byte writes and varying short-write chunk sizes.
- Zero-byte write.
- ENOSPC, EIO, EACCES, EBADF, and a generic non-retryable error before any bytes and after a nonzero prefix.
- Failure after every possible byte offset for representative records, including immediately before LF.
- fsync failure after a complete line was written.
- close failure after successful fsync.
- lock acquisition/contention and unlock failure if locking is included.
- fstat identity mismatch, non-regular descriptor, changed size, and unexpected current offset.
- Allocation failure before the first write.
- Reentrant/concurrent append attempt.

Assertions for every injected case:

- No second record is appended after an uncertain outcome.
- The writer's in-memory revision advances only after successful fsync and explicit caller commit.
- The exact prepared bytes are used; no regenerated or duplicated whole record is written.
- No automatic truncate, tail repair, valid-prefix acceptance, or compensating record occurs.
- The primary errno/result is retained across cleanup.
- A successful fsync remains a durable commit even if later close/unlock reports failure.
- No source, stage, destination, quarantine, or unrelated file is touched.

## 10. Crash and restart matrix

Use disposable temporary files only. Simulate termination:

- before the first write;
- after a partial record prefix;
- after all bytes including LF but before fsync;
- after successful fsync;
- after fsync error;
- during close/reopen/replay;
- with a truncated tail and with a malformed tail.

For each case, reopen and parse the complete file. Assert that malformed/truncated tails are not modified, accepted, or silently discarded. Repeat replay to prove deterministic outcomes.

Process-kill tests do not simulate power loss. The report must state this limitation and must not claim hardware power-loss guarantees.

## 11. Test organization and build isolation

The implementation PR should:

- Keep adapter sources in a test-only target or private test support library, not in the production executable source list.
- Register focused CTest coverage with deterministic tests and bounded runtime.
- Keep temporary-file fixtures isolated per test and clean up only test-owned paths.
- Run under C11, C17, and C23 validation; production language baseline remains C17.
- Run ASan/UBSan, `make test`, `make validate`, `make release`, Docker build/runtime smoke, Ubuntu 24.04 checks, and non-root runtime checks.
- Add a test proving the production executable does not link or invoke the test adapter.
- Preserve all existing FCJ2/legacy, FCJ3, FCJ4 parser, serializer, and writer-state tests.

No benchmark is a substitute for correctness or crash/fault-injection coverage.

## 12. Acceptance gates

Do not mark the implementation PR complete unless:

- Every fault-injection row has an explicit test and asserted state/result.
- Writer state never advances before fsync success.
- Uncertain outcomes poison the adapter and block further appends.
- Restart uses full parser replay and never repairs/truncates a tail.
- Close/unlock failures after fsync are reported without undoing the durable result.
- Temporary-file scope and test-only build isolation are proven.
- Existing language, sanitizer, release, Docker, and non-root checks pass.
- Documentation and threat model still clearly state that FCJ4 is not enabled in production and does not authorize filesystem recovery.

## 13. Next milestone and prohibited expansion

After this design is reviewed and merged, the next proposal may implement only the test-only append adapter and its fault/crash harness against disposable temporary journals.

It must not create/open `journal/move.fcj4`, integrate with production mover/CLI/startup, or implement recovery planning/execution. A production journal lifecycle, directory ownership policy, production lock ownership, and FCJ4 adoption require separate threat-model review and explicit approval.

## 14. Rollback

This design milestone adds one document. Rollback is removal of this document; no executable code, journal bytes, production behavior, or user files change.
