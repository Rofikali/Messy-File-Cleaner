# FCJ4 Malformed and Truncated Restart Matrix Design

## Status

Proposed design only. This document is a follow-on to the merged FCJ4 test-only append adapter and cross-thread exclusion milestones. It authorizes no implementation until this design is reviewed and merged.

The scope is limited to parser/replay behavior after reopening disposable test journals. FCJ4 remains unintegrated with production journal paths, mover, CLI, startup, and recovery.

## 1. Objective

Prove that restart/replay handles incomplete, malformed, and semantically invalid journal tails deterministically and conservatively.

The central safety rule is:

**Replay must either accept the entire byte sequence as a complete valid FCJ4 journal or reject the entire sequence. It must never accept a valid prefix while ignoring an invalid suffix, repair bytes, truncate a tail, or mutate the journal.**

The tests must distinguish:
- a complete journal ending exactly at a record boundary;
- an incomplete physical record;
- a complete physical line containing invalid protocol data;
- a semantically invalid record sequence;
- a valid journal followed by an invalid or incomplete suffix.

A parser error is not authorization to discard bytes or perform filesystem recovery.

## 2. Existing coverage and identified gaps

The current test-only append integration suite already covers:
- partial-write failure and parser rejection of the resulting 3-byte prefix;
- fsync failure followed by replay of a complete record;
- writer-commit failure and replay reconciliation;
- post-write descriptor/size mismatch followed by replay;
- durable-but-unacknowledged append followed by replay;
- a successful child-process exit and parent-process replay;
- injected errno failures at every byte boundary of one canonical record.

The existing partial-write test only checks parser rejection of one 3-byte prefix. The successful process-exit test only covers a complete one-record journal. The current tests do not yet define a systematic restart matrix over every truncation offset, malformed framing classes, valid-prefix-plus-invalid-tail all-or-nothing behavior, repeat replay determinism, or explicit proof that replay leaves the original bytes unchanged.

This milestone fills those gaps without changing the parser contract or append adapter behavior.

## 3. Scope

### In scope

- Test-only fixtures using private temporary directories and files created by the integration harness.
- Canonical FCJ4 record bytes produced by the existing serializer; malformed variants derived from those bytes in test memory.
- Close/reopen/replay tests, including child-process termination where it adds coverage.
- Byte-for-byte before/after comparison proving parser/replay does not modify the journal.
- Repeated replay of the same unchanged bytes to prove deterministic success or failure.
- Valid-prefix-plus-invalid-suffix cases proving all-or-nothing replay.
- Resource-limit and framing-boundary cases relevant to restart input.

### Explicitly out of scope

- Any production journal path, including `journal/move.fcj4` and `journal/move.log`.
- Production executable, CLI, mover, startup, watcher, planner, executor, or recovery integration.
- FCJ2/legacy or FCJ3 changes.
- Filesystem recovery, rename, unlink, quarantine, source retirement, cleanup, or other mutation.
- Tail repair, truncation, valid-prefix acceptance, salvage mode, auto-recovery, or migration.
- Changing FCJ4 grammar, record limits, parser API, serializer behavior, or writer state semantics unless a separate design is approved.
- Claims of power-loss durability based on process termination or ordinary CI.

## 4. Normative replay invariants

Every test must enforce the following invariants:

1. **All-or-nothing:** on any parse failure, no partial replay state is exposed as a successful replay.
2. **No byte mutation:** replay receives bytes and does not open or modify the journal file. The test harness compares the file before and after every replay attempt.
3. **No valid-prefix acceptance:** a valid first record followed by an incomplete, malformed, or semantically invalid suffix must fail as a whole.
4. **Determinism:** repeated replay of identical bytes produces the same success/failure result and, on success, equivalent record counts and replay views.
5. **Exact-boundary acceptance:** a complete valid journal ending immediately after LF is accepted.
6. **Strict framing:** no partial line, missing LF, CRLF substitution, embedded NUL, control byte, extra line break, or non-canonical encoding is silently normalized.
7. **Semantic validation:** physically complete records that violate schema, sequence, uniqueness, operation/action lifecycle, or cross-record identity constraints are rejected.
8. **No repair:** tests must not call truncate, rewrite, append a repair marker, or otherwise modify the source journal to make replay pass.
9. **Ownership:** each replay object is destroyed on both success and failure; the input buffer remains caller-owned and unchanged.
10. **No execution authority:** successful replay is evidence of protocol consistency only; it is not proof of authenticity, filesystem truth, or permission to execute recovery.

## 5. Test matrix

### 5.1 Truncation at every byte offset

Start with one serializer-produced canonical BEGIN record and a separate multi-record valid journal. For a canonical record of length N, generate every strict prefix with lengths 0 through N-1.

For every prefix:
- write that exact prefix to a new private fixture;
- close the writer descriptor and reopen the file read-only;
- read the exact file length into a caller-owned buffer;
- parse the complete byte sequence;
- require failure and no successful replay state;
- compare the file bytes and size before/after parsing;
- repeat parsing against the same unchanged bytes and require the same result.

The full N-byte record, including its terminal LF, must parse successfully when it is a valid standalone journal. Do not treat zero-byte input as a valid empty journal unless the parser's already-approved contract explicitly says so; the implementation must follow the existing contract and document the observed behavior.

### 5.2 Physical framing and encoding corruption

Derive independent malformed variants from a canonical record. Include at minimum:
- missing terminal LF;
- CRLF in place of LF;
- embedded NUL at the beginning, middle, and near the end;
- an additional LF after a complete record;
- a blank physical line;
- control byte and non-ASCII byte in a field;
- missing delimiter, extra delimiter, and incorrect field count;
- empty required field;
- non-canonical integer (leading zero, sign, overflow, or invalid digit);
- uppercase hex where lowercase is required;
- malformed/incorrect-length identifier or digest;
- physical line at the configured maximum boundary and one byte beyond it.

For each case, state whether failure is expected at framing, field parsing, scalar validation, or resource-limit validation. Assert full replay rejection, unchanged bytes, and deterministic repeated result.

### 5.3 Semantic and transition corruption

Use serializer-produced valid records as a base, then construct complete physical lines that are syntactically framed but violate existing parser invariants. Cover representative cases:
- duplicate operation or action identifiers within their defined uniqueness scope;
- non-monotonic, duplicate, skipped, or out-of-order sequence values where prohibited;
- record before BEGIN or after a terminal lifecycle state;
- SEALED without matching BEGIN identity;
- MOVE or ACTION before required prior state;
- invalid move/action state transition;
- ACTION_DEF whose immutable identity/metadata conflicts with BEGIN/SEALED evidence;
- quarantine follow-up whose referenced target component does not match the prior definition;
- interleaved operations where one operation's own order is invalid while another operation remains valid.

Do not invent new schema constraints in tests. Every negative fixture must point to an invariant already defined in the FCJ4 schema/replay contract or parser implementation.

### 5.4 Valid prefix plus invalid suffix

Construct a complete valid prefix containing at least two records, then append each category of invalid suffix:
- one-byte and multi-byte truncated record;
- complete malformed physical line;
- complete but semantically invalid record;
- extra blank line or extra LF;
- oversized physical line;
- invalid record after an otherwise terminal operation.

Replay must reject the entire concatenated byte sequence. It must not expose the valid prefix as a successful replay, report a usable partial record count, or import partial state into a writer.

Include a positive control: the same valid prefix without a suffix must replay successfully and expose the expected complete record count.

### 5.5 Restart and process-boundary cases

For each important on-disk state, close all descriptors and construct a fresh replay object after reopening the fixture. At minimum include:
- no bytes written;
- partial record prefix left by uncertain write;
- complete record bytes after injected fsync failure (process-level tests cannot infer whether the data is power-loss durable);
- complete record after successful fsync and writer acknowledgement;
- complete record after successful fsync but before writer acknowledgement;
- valid journal plus malformed suffix;
- valid journal plus truncated suffix.

Where fork/child-process tests are used:
- child must use only its private fixture;
- child exit status must be checked;
- parent must reopen the path independently;
- child termination must not be described as a power-loss simulation;
- no test should depend on inherited adapter context or stale descriptor ownership.

The adapter's current fsync-error test can show that the complete bytes are visible to a later process in the tested environment. It cannot establish that the bytes survived a sudden power failure.

## 6. Test implementation architecture

- Keep all new cases in a focused Linux-only integration test target or a clearly isolated test source registered with the existing FCJ4 append integration executable.
- Reuse the existing canonical serializer and parser APIs. Do not duplicate the FCJ4 grammar in test code.
- Prefer small deterministic fixture builders that produce one malformed condition at a time.
- Each fixture builder must state the expected rejection layer and the source invariant it violates.
- Use bounded buffers sized from existing public FCJ4 limits. Check all size arithmetic before allocation/copy.
- Avoid unbounded loops, sleeps, timing assumptions, or external tools.
- Preserve the existing C11/C17/C23 validation and ASan/UBSan workflows.
- Keep TSan as a separate sanitizer configuration; do not combine it with ASan/UBSan.
- Do not add production source-list entries or runtime dependencies.
- Preserve the separate concurrency-only target and its test-only pthread dependency.

## 7. Required assertions and evidence

For every negative case:
- parser returns failure;
- replay object is safe to destroy;
- no partial replay is imported into a writer;
- file size and bytes are unchanged;
- second replay attempt gives the same result;
- errno/error classification is asserted only where the parser API promises it; do not over-specify unspecified errno values.

For positive controls:
- parser succeeds;
- expected record count and relevant immutable views match;
- repeated replay produces equivalent results;
- input bytes remain unchanged.

For valid-prefix-plus-invalid-suffix:
- parser fails as a whole;
- no partial writer import is attempted or accepted.

The PR description must distinguish:
- user-provided Codespaces test results;
- exact-head GitHub Actions results;
- process-restart visibility checks;
- guarantees that are not established, especially hardware power-loss behavior.

## 8. Acceptance gates

Do not mark the implementation milestone complete until:
- every byte-prefix truncation case is covered for the selected canonical record;
- all framing/encoding corruption categories in §5.2 have explicit tests;
- semantic corruption cases in §5.3 map to existing documented invariants;
- valid-prefix-plus-invalid-suffix all-or-nothing tests pass;
- successful and unsuccessful replay both preserve exact file bytes;
- repeated replay is deterministic;
- all descriptors, replay objects, and temporary fixtures are cleaned up on test success;
- C11/C17/C23 Debug validation with ASan/UBSan passes;
- C17 Release build passes;
- Docker build/runtime and non-root checks pass;
- dedicated concurrency/TSan coverage remains green;
- production target/source-list isolation remains unchanged;
- docs and threat model still state that FCJ4 is not production-integrated and replay never authorizes filesystem mutation.

A passing process-kill test is not a power-loss guarantee. A passing parser test is not proof of journal authenticity or filesystem state.

## 9. Explicit stop conditions

Stop and request a new design review if implementation appears to require:
- changing the parser's accepted language or schema;
- adding salvage, prefix acceptance, tail repair, or truncation;
- changing journal record limits;
- production file/path opening or mutation;
- production mover/CLI/startup/recovery integration;
- modifying FCJ2/legacy or FCJ3 behavior;
- using replay output to trigger filesystem actions.

## 10. Rollback

Rollback removes only the new test fixtures, test cases, CTest registration, and this milestone's implementation-specific documentation. It must not alter production code, production source lists, existing journal bytes, user files, or FCJ2/legacy/FCJ3 behavior.
