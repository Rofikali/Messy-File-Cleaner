# FCJ4 Writer, Serialization, and Append-Durability Design

## Status

Proposed design only. This document is a follow-on to the merged FCJ4 schema/replay contract (PRs #61/#62), pure parser/replay implementation (PR #63), and interleaved-operation replay test (PR #64).

This design does not implement a writer, create `journal/move.fcj4`, enable FCJ4 in production, or authorize filesystem recovery actions. It must be reviewed and merged before a separate implementation milestone begins.

## 1. Objective and decision

Define the contract for producing canonical FCJ4 records and appending them to a dedicated future FCJ4 journal without weakening the parser's strict wire contract or claiming durability that the operating system has not confirmed.

The design separates four responsibilities:

1. **Record construction/serialization** — convert validated typed values into one canonical FCJ4 byte line.
2. **Writer state validation** — ensure a proposed record is a legal next transition for the writer's in-memory operation/action state.
3. **Journal append and durability** — append the complete record, synchronize it, and report uncertainty without silently advancing state.
4. **Replay reconciliation** — after any uncertain result or restart, parse the complete journal and reconcile observed history before another transition is attempted.

The writer is not a recovery planner or executor. It does not inspect, rename, unlink, quarantine, publish, or otherwise mutate user files. It never infers filesystem truth from journal state.

## 2. Non-negotiable invariants

- FCJ4 is a separate protocol and future path: `journal/move.fcj4`. Never append FCJ4 to FCJ2/legacy `journal/move.log` or to an FCJ3 journal.
- Existing FCJ2/legacy and FCJ3 readers/writers remain behaviorally unchanged.
- The existing FCJ4 parser remains pure, read-only, all-or-nothing, and authoritative for wire-history validation.
- A writer-generated record must be accepted by the current FCJ4 parser under the same schema version.
- One logical journal writer at a time. No concurrent appenders and no multi-process interleaving.
- A record is not considered committed in writer memory until its complete bytes have been appended and the required journal durability operation has succeeded.
- Any partial write, append error after bytes may have reached the file, or fsync failure poisons the writer. No later record may be appended by that writer instance. A close/unlock failure after a successful fsync does not undo the committed record; it makes descriptor/lock ownership unresolved and prevents reuse until safely resolved.
- Never truncate a malformed or partial tail automatically. Never accept a valid prefix as the complete journal.
- No fallback to weaker durability, no silent retry after uncertain append, and no “best effort” success.
- Journal validity is not journal authenticity; SHA-256 is integrity evidence, not a MAC or signature.
- A successful append does not prove any filesystem action occurred. A durable ACTION record is protocol history only.
- No writer API may call a filesystem mutation affecting source, stage, destination, or quarantine entries.

## 3. Scope boundaries

### Included in this design

- Typed record input contract for BEGIN, SEALED, MOVE, ACTION_DEF, and ACTION.
- Exact canonical serialization rules and field-count/length calculations.
- Validation responsibilities before serialization.
- State transition and append sequencing.
- Single-writer ownership, journal-file identity, locking, append, fsync, close, and failure/poison semantics.
- Replay-based recovery after uncertain writes.
- Unit, integration, fault-injection, crash, compatibility, and resource-bound tests.
- Future implementation boundaries and explicit non-goals.

### Explicitly excluded

- Production FCJ4 writer implementation in this PR.
- Creation or opening of `journal/move.fcj4` by the application.
- Adding FCJ4 to production source lists, mover paths, or startup/recovery paths.
- FCJ3 changes or migration.
- Filesystem evidence capture, planner, executor, quarantine, source retirement, stage cleanup, destination publication, or any user-file mutation.
- Journal rotation, compaction, repair, truncation, or automatic migration.
- Multi-writer support, distributed locking, databases, brokers, daemons, or network storage guarantees.
- Authentication against privileged or same-UID journal tampering.

## 4. Canonical serialization contract

The schema in `FCJ4_SCHEMA_AND_REPLAY_DESIGN.md` remains the wire-format authority. This document adds writer-side construction rules; it must not redefine field meaning or loosen parser acceptance.

### 4.1 Common framing

Every record is exactly one ASCII line:

`FCJ4|KIND|1|...fields... LF`

- ASCII bytes only; LF is the final byte.
- No BOM, CR, CRLF, NUL, blank line, leading/trailing whitespace, or extra delimiter.
- Exact field count for the record kind.
- No optional/empty fields in schema version 1.
- The writer serializes into a bounded caller-owned or writer-owned buffer before any append attempt.
- The serialized length includes the final LF and must be no greater than `FCJ4_MAX_LINE_BYTES`.
- A record must never be split across multiple independently committed append calls.
- No locale-dependent formatting. Decimal output uses explicit unsigned/signed integer conversion helpers; do not use locale-sensitive grouping or floating-point formatting.

### 4.2 Exact field counts

Field counts include the three-field prefix `FCJ4|KIND|1` and every subsequent field, but exclude LF.

| Record | Exact fields | Maximum line bytes |
|---|---:|---:|
| BEGIN | 27 | Derived from validated PATH_MAX/NAME_MAX inputs and fixed scalar widths; hard-capped at 32 KiB |
| SEALED | 12 | Derived from component bound and fixed scalar widths; hard-capped at 32 KiB |
| MOVE | 7 | Fixed small bound |
| ACTION_DEF | 20 | Derived from component bounds and fixed scalar widths; hard-capped at 32 KiB |
| ACTION | 8 | Fixed small bound |

The serializer must use the exact order in the schema document. It must not calculate field count from a loosely populated generic key/value map.

### 4.3 Scalar encodings

- Unsigned integers: canonical base-10; zero is exactly `0`; no plus sign or leading zero.
- Signed seconds: canonical base-10; no plus sign, no leading zero except `0`, and no `-0`.
- Enums: exact uppercase schema tokens; unknown enum values fail before serialization.
- Operation/action IDs: exactly 32 lowercase hexadecimal characters. The serializer validates the supplied identifier; secure random generation is a separate input-acquisition responsibility.
- SHA-256: exactly 64 lowercase hexadecimal characters.
- Device/inode values: validate conversion from platform `dev_t`/`ino_t` to the schema's unsigned 64-bit representation without truncation. Do not assume casts preserve values.
- Nanoseconds: integer range 0 through 999,999,999.
- Error codes: validate the schema-specific state/error relationship; do not serialize negative errno values or platform-specific error strings.
- Paths/components: serialize the exact byte string as lowercase hex with a preceding decoded-byte length. Validate length before multiplication and ensure encoded length arithmetic cannot overflow.
- The serializer does not normalize paths, Unicode, separators, case, or components. Validation rejects forbidden values; accepted values are serialized byte-for-byte.

### 4.4 String and path validation

Before encoding:
- Require non-null pointer and nonzero length for required fields.
- Require the declared length to equal the actual bounded byte-string length.
- Reject embedded NUL, control bytes, DEL, and malformed inputs.
- BEGIN source and destination paths must be absolute diagnostic paths and within the parser's decoded path bound. They are not authorization tokens and must never be opened by the serializer.
- Destination basename, stage component, source component, and target component must each be exactly one component; reject empty, `.`, `..`, slash, NUL, and controls.
- Component role and sentinel rules must match the schema exactly. A sentinel is an explicit numeric zero only where the schema defines it; empty fields are never sentinels.
- Do not rely on `PATH_MAX` or `NAME_MAX` alone as proof that a path exists or is safe to resolve.

### 4.5 Buffer sizing and checked arithmetic

- Define a maximum encoded record size at compile time using the schema constants and platform-independent maximum scalar widths, then assert it does not exceed `FCJ4_MAX_LINE_BYTES`.
- Use checked addition and multiplication for hex expansion, decimal widths, separators, and LF.
- No VLA, unbounded `sprintf`, or allocation based solely on untrusted length fields.
- Serialization failure leaves the output buffer and writer state in a documented non-committed state; no bytes are appended.
- A too-small output buffer returns a distinct size/capacity error and the required size where the API supports it.
- All output buffers have explicit capacity and output length. Do not require NUL termination for the on-disk record; a convenience NUL must be outside the appended length.

## 5. Typed API and ownership contract

The implementation should expose a small, versioned C interface in a dedicated FCJ4 writer header. Internal parser structs must not be repurposed as mutable writer state.

Proposed conceptual operations (names are illustrative and may be refined in implementation review):

- initialize an in-memory writer/operation context from validated immutable evidence;
- prepare a canonical record into a bounded output buffer;
- append one prepared record through an explicitly supplied journal adapter;
- commit the in-memory transition only after the append durability contract succeeds;
- mark the writer poisoned after any uncertain append/durability outcome;
- destroy/reset caller-owned state without filesystem side effects.

Contract requirements:
- Pure C; C17 production baseline; C11 compatibility; C23 validation.
- Opaque context preferred where it materially protects invariants; fixed-width integer fields at stable boundaries.
- No exposed `FILE *`, pthread type, internal queue, or platform-specific file descriptor in a future stable public ABI. A private Linux/POSIX implementation may use descriptors internally.
- Caller ownership, context lifetime, output-buffer lifetime, thread safety, and error semantics must be documented.
- One context is single-owner and not internally thread-safe unless a future design explicitly proves otherwise.
- A prepared record is bound to the exact prior operation/action sequence and immutable identity. It cannot be committed twice, replayed into a different context, or modified between prepare and append.
- Do not expose mutable internal buffers or string pointers whose lifetime is unclear.
- No global mutable writer state.

This design does not freeze a public ABI. Header shape and exact function names require implementation review after this design is approved.

## 6. Writer state and legal record ordering

The writer must enforce the same state machine as the parser, before serialization and again when committing a prepared record.

### 6.1 Operation records

- BEGIN: exactly once; first record for the operation ID.
- SEALED: exactly once; only after BEGIN; before MOVE, ACTION_DEF, or ACTION.
- MOVE: only the legal transitions in the schema; sequence begins at 1 and increments by one.
- ACTION_DEF: only after BEGIN and SEALED; immutable; action ID unique within that operation; no more than 16 definitions per operation.
- ACTION: only after its ACTION_DEF; sequence begins at 1 and increments by one; legal state transitions only.
- Interleaving across different operation IDs is allowed at journal level, but each operation's own history must preserve its order.
- A single writer serializes appends. It may switch between independent operations only after each record has passed validation and the previous record's durability operation succeeded.
- Records after terminal move/action states are rejected.
- No writer transition is inferred from a successful filesystem syscall. The caller must present the explicit next typed state only after its own higher-level preconditions have been met.

### 6.2 Prepared versus committed state

The writer must have distinct states:
- **Ready**: prior journal state is known and no append is outstanding.
- **Prepared**: one immutable next record is serialized and bound to a prior sequence/state.
- **Appending**: append is in progress; no second append is permitted.
- **Committed**: complete record append and required durability operation succeeded; in-memory replay state may advance.
- **Poisoned**: append or durability outcome is uncertain; no further records are permitted until a fresh file reopen and complete replay/reconciliation creates a new writer context.

The in-memory state must never advance before the durability boundary. A successful serializer call is not a journal commit.

## 7. Journal file and single-writer contract

This is a future Linux/POSIX adapter contract, not permission to create the production journal path in this milestone.

### 7.1 File identity and open policy

When a later implementation milestone is approved, it must:
- Use only the distinct FCJ4 path under the already validated journal directory.
- Open descriptor-relatively from a validated journal-directory descriptor.
- Reject symlink journal entries; use no-follow and close-on-exec flags where supported.
- Require a regular file with the expected owner and restrictive mode; reject hard-link surprises and unexpected link counts according to the final threat model.
- Verify the journal directory's device/inode identity and ownership/mode policy before use.
- Never follow a path from a journal record.
- Never create, truncate, replace, or repair an existing journal as a fallback.
- Treat missing file creation as a separately gated future operation with explicit exclusive-create semantics; this design does not authorize it.
- Fail closed if the required descriptor-relative or locking capability is unavailable.

The exact owner/mode values and whether the file is pre-created or exclusively created must be resolved in the implementation ADR against the repository threat model. Do not invent a weaker fallback.

### 7.2 Locking and concurrency

- Enforce one process and one writer context per FCJ4 journal.
- Acquire an exclusive lock before replay/open-for-append and hold it for the writer lifetime; use a documented Linux/POSIX locking primitive whose semantics are tested for the supported filesystems.
- A second writer must fail safely rather than append concurrently.
- Thread-level access must be serialized by the owning module; do not rely on `O_APPEND` alone to establish record-level transaction ordering or durability.
- Lock acquisition, lock loss, descriptor replacement, or unsupported lock semantics are fatal for the writer session.
- Network filesystems and filesystems with unverified locking/fsync semantics are unsupported unless separately tested and explicitly approved.

### 7.3 Append algorithm

The later writer adapter must follow a single explicit protocol:

1. Hold the exclusive journal lock.
2. Verify the journal descriptor still refers to the validated regular file and expected directory entry.
3. Confirm the current end-of-file offset matches the writer's known journal length from the successful initial replay plus committed appends.
4. Serialize and validate the entire next record into memory before writing any byte.
5. Append the record bytes in a loop that handles EINTR and short writes without changing byte order.
6. If a write returns zero or a non-retryable error, mark the writer poisoned. Do not append another record.
7. After all bytes are written, call `fsync(journal_fd)`; the required contract is data and metadata durability for the journal file. Do not substitute `fdatasync` unless a future platform-specific ADR proves it preserves all required metadata and crash semantics.
8. Only after fsync succeeds may the in-memory operation/action sequence advance and the record be reported committed.
9. Preserve the primary errno across cleanup. A close or unlock failure after a successful fsync must be reported distinctly; it must not be misreported as a failed record append if the record is already durable.
10. Never automatically truncate a partial tail, retry a whole record after uncertain partial progress, or append a compensating record to hide uncertainty.

Because a partial write may leave a prefix of a record, any write failure after append begins poisons the journal session. A subsequent process must replay the complete file; if the tail is malformed or truncated, stop for manual/operator recovery. No automatic valid-prefix acceptance or tail repair.

### 7.4 Durability boundary and parent directory

- The journal file must already exist before append. This design's append operation does not create or rename it.
- For initial creation in a future separately approved milestone, the creation protocol must define exclusive creation, file mode, file fsync, and journal-directory fsync before the file is considered established.
- For append to an existing file, fsync of the journal file is the record durability boundary. Directory fsync is not required for each append because the directory entry is unchanged.
- If journal rotation/replacement is ever introduced, it requires a separate protocol and directory-fsync crash matrix.
- No record is reported durable merely because write returned success or because data appears in the page cache.

## 8. Error handling and poisoned-writer semantics

Errors must distinguish at least:
- invalid caller input / illegal transition (no bytes written; writer remains usable if its known state is unchanged);
- capacity/record-size/resource limit (no bytes written);
- allocation failure before append (no bytes written);
- lock/open/file-identity failure before append (no bytes written; session not usable);
- write failure after append began (outcome uncertain; poison);
- fsync failure after complete write (outcome uncertain; poison);
- descriptor/close/unlock failure after fsync success (record may be durable; report committed state plus cleanup/ownership error without pretending the record is absent);
- replay failure on reopen (journal invalid; no further append; preserve evidence and require manual review).

Rules:
- Preserve errno from the primary failing operation before cleanup.
- Never hide a primary append/fsync failure behind a later close error.
- Do not use errno alone as a protocol state. Return explicit status plus saved system error where appropriate.
- Do not retry a non-idempotent whole-record append after uncertain progress.
- A poisoned context rejects all prepare/append/commit operations.
- A new context may be created only after closing the old descriptor, reacquiring the exclusive lock, reopening the same validated journal object, and parsing the entire journal successfully. If identity changes or the journal is invalid, stop.
- The writer cannot decide to truncate a tail, skip malformed records, or infer that an incomplete line was harmless.

## 9. Restart and reconciliation after uncertainty

A restart/reopen flow is a separate operation, not an implicit retry:

1. Stop all writes and preserve the current journal bytes.
2. Close the old writer context; retain the original failure details for diagnostics.
3. Reacquire the exclusive journal lock.
4. Revalidate journal directory and file identity.
5. Read the complete bounded journal (maximum 64 MiB) without following symlinks or accepting a prefix.
6. Parse with the existing pure FCJ4 parser.
7. If parsing fails, do not modify the journal; report manual review.
8. If parsing succeeds, reconstruct operation/action sequence state from replay.
9. Compare replayed committed history with any known in-memory prepared record only for diagnostics. Never assume a failed fsync means bytes are absent or present.
10. Resume only from the exact parsed terminal history if all writer invariants and file identity checks pass. Any mismatch, unexpected tail, or ambiguous append result stops for manual review.

A valid replay confirms syntactic and transition consistency only. It does not authorize filesystem actions or prove their postconditions.

## 10. Test and verification strategy

The implementation milestone must not be considered complete on compile success alone.

### 10.1 Pure serializer unit tests

- Golden byte fixtures for all five record kinds.
- Exact field counts, field order, separators, LF termination, and exact byte lengths.
- Round-trip: serialize each valid typed record, parse it with the existing FCJ4 parser, and compare every exposed field/state.
- Canonical decimal output at zero, one, maximum supported values, signed minimum/maximum, and boundary timestamps.
- Lowercase ID/digest validation; reject uppercase, wrong length, non-hex, and invalid IDs.
- Reject null/empty required strings, mismatched declared lengths, embedded NUL, controls, DEL, invalid absolute paths, component traversal, slash in components, and sentinel misuse.
- Reject unknown enums, illegal error-code/state combinations, duplicate/invalid sequence, and over-limit action counts.
- Boundary tests for hex expansion, integer-width conversions, addition/multiplication overflow, insufficient output capacity, and maximum legal record length.
- Ensure serialization failure writes no journal bytes and does not advance operation/action state.
- Run tests with assertions enabled and disabled; do not put side effects inside assert expressions.

### 10.2 Writer state tests

- BEGIN must be first and unique.
- SEALED requires BEGIN and is unique.
- MOVE transitions exactly match the parser contract.
- ACTION_DEF requires BEGIN + SEALED, is immutable, and is unique per action ID.
- ACTION requires ACTION_DEF and follows its own independent sequence/state machine.
- Interleaved independent operations preserve per-operation order.
- Reused action IDs across different operations are accepted; duplicate within one operation is rejected.
- Prepared records cannot be committed twice, modified, or committed against stale prior state.
- Writer state advances only after durable append success.
- Poisoned writer rejects all further operations.

### 10.3 Append fault injection

Use an injectable, narrow journal I/O adapter in tests. Do not add production global fault state.

Inject:
- EINTR before any bytes and between short writes;
- short writes of one byte and varying chunks;
- zero-byte write;
- ENOSPC, EIO, EACCES, EBADF, and other non-retryable write errors;
- failure after a configurable prefix has been appended;
- fsync failure after full write;
- close and unlock failures after fsync success;
- lock contention and lock-acquisition failure;
- file identity replacement, unexpected EOF/offset, and unexpected file size;
- allocation failure before append.

Assertions:
- no second writer interleaves;
- each successful record is exactly one canonical line;
- a partial/uncertain append poisons the current context;
- no whole-record retry or automatic truncation occurs;
- a fsync failure does not advance in-memory state;
- a close/unlock failure after successful fsync preserves the committed-record result while reporting cleanup failure;
- original errno is preserved;
- source, stage, destination, quarantine, and unrelated files are untouched.

### 10.4 Crash and restart tests

For a disposable journal file only, terminate or simulate failure:
- before append;
- after a prefix write;
- after complete line write but before fsync;
- after successful fsync;
- during reopen/replay;
- with a truncated tail;
- with a malformed tail;
- with a valid complete record present after a simulated fsync error.

After each case:
- reopen under the exclusive lock;
- replay the complete journal;
- assert no valid-prefix acceptance or automatic repair;
- assert the next legal record is determined only by complete replay;
- assert repeated restart/replay is stable;
- preserve malformed/uncertain bytes exactly.

Power-loss behavior cannot be fully proven by process-kill tests. Document the tested filesystem/kernel/container environment and do not claim power-loss guarantees beyond the platform's fsync contract and actual validation.

### 10.5 Compatibility and build matrix

- Existing FCJ2/legacy tests remain unchanged and pass.
- Existing FCJ3 parser/writer/report/recovery tests remain unchanged and pass.
- Existing FCJ4 parser fixtures remain accepted.
- Serializer output round-trips through the current FCJ4 parser.
- C11/C17/C23 Debug and Release builds.
- ASan/UBSan; static analysis according to repository workflow.
- `make test`, `make validate`, Docker build/runtime smoke, Ubuntu 24.04 development/production image checks, non-root runtime checks.
- No production executable links or invokes the FCJ4 writer during this isolated milestone.

## 11. Resource and operational limits

- Reuse the parser's current limits: 64 MiB journal, 32 KiB physical record, 4096 operations, 16 action definitions per operation, and 100,000 records.
- Writer must refuse to append when the resulting journal would exceed the maximum journal size.
- Check `current_size + record_length` for overflow before append.
- The writer must not rotate, compact, truncate, or silently start a new journal when the limit is reached. It fails closed and requires a separately designed operator workflow.
- Bound memory to one prepared record plus bounded replay state; avoid retaining duplicate unbounded journal copies.
- No recursive processing or unbounded retries; EINTR retry loops are bounded by OS return behavior and must still terminate on non-retryable failure.

## 12. Observability and audit output

- Report operation ID, action ID where relevant, record kind, sequence, byte offset, and explicit status.
- Never log raw file contents, secrets, or full journal lines by default.
- Paths may contain sensitive user data; diagnostics should prefer IDs and escaped/truncated path context, with explicit opt-in for detailed path logging.
- Distinguish “serialized”, “append attempted”, “append uncertain”, “fsync succeeded”, and “record committed”. Do not collapse them into a generic “saved” message.
- Diagnostics must not mutate journal bytes or change writer state.

## 13. Implementation milestones after this design

This design PR authorizes no implementation. After it is reviewed and merged, the next implementation proposal should be a narrowly scoped, isolated FCJ4 serializer and in-memory writer-state milestone with golden fixtures and parser round-trip tests. It should not open or append to a production journal file.

A later, separately reviewed milestone may implement a test-only append adapter with fault injection and crash tests against disposable temporary journals. Production journal creation/opening, writer integration, and FCJ4 adoption require additional explicit design/review gates.

No writer implementation, journal filesystem adapter, production integration, planner, executor, or recovery mutation may begin merely because this document is merged.

## 14. Rollback

This design milestone adds one document only. Rollback is removal of `docs/quality/FCJ4_WRITER_SERIALIZATION_AND_APPEND_DESIGN.md`. No executable code, journal bytes, production behavior, or user files change.
