# FCJ3 Operation Context and Record Encoding

## Status

Implementation milestone design. This phase adds secure operation identity, immutable in-memory metadata, transition preparation/commit, and canonical record serialization. It does not append records to the production journal and does not enable mover or recovery integration.

## Objective

Build the smallest C API that can create an FCJ3 transaction context from already-validated filesystem identities and prepare exact FCJ3 transition bytes. Keep persistent writes disabled until the O_TMPFILE staging lifecycle can provide the complete metadata in the required order.

## Inputs and trust boundary

The initializer receives:
- nonempty source and destination path strings, each shorter than PATH_MAX;
- a destination-directory stat snapshot that must identify a directory;
- a staging-component name in the reserved `.fc-tmp-` namespace, with a nonempty suffix and fewer than NAME_MAX bytes;
- a staging-inode stat snapshot that must identify a regular file, have `st_nlink == 0` (still anonymous), and have the same `st_dev` as the destination directory.

The caller is responsible for obtaining these stat snapshots from the actual validated destination directory and anonymous O_TMPFILE descriptor, before the staging inode is linked by name. The initializer copies all values into the context. It does not open paths, follow symlinks, or perform filesystem operations.

## Operation identity

- Generate exactly 16 bytes using Linux `getrandom()`.
- Retry interrupted and short reads until all 16 bytes are obtained.
- Any non-EINTR failure or zero-byte return fails initialization.
- No timestamp, PID, counter, address, PRNG, or weak fallback is permitted.
- Encode the bytes as exactly 32 lowercase hexadecimal characters.

## Immutable context

The context owns:
- operation ID;
- source and destination path bytes;
- destination directory device/inode;
- staging component bytes;
- staging inode device/inode;
- current committed state and sequence;
- an uncertainty/poison flag.

The immutable fields cannot be changed by transition preparation. A context becomes unusable for further transitions after an uncertain append/fsync result is reported by a future journal adapter.

## Transition API

The API uses a two-step protocol:
1. `prepare_transition` validates the next state/error code and serializes a complete canonical line without changing the context.
2. The future journal adapter writes the complete line and fsyncs the journal.
3. Only after successful durable append does the caller invoke `commit_transition`, which advances the in-memory state/sequence.

A prepared transition records the prior state and sequence. A stale or duplicate prepared object cannot advance the context after another transition has committed. If a future append has a partial-write or fsync uncertainty, the caller must poison the context and stop appending; it must not guess whether the record became durable.

This phase does not implement the journal adapter or the durable append step. The prepare/commit API exists to make that ordering explicit and testable.

## Serialization

- Exact 16-field FCJ3 grammar from FCJ3_JOURNAL_SCHEMA.md.
- Canonical unsigned decimal fields, lowercase hex, byte lengths, and final LF.
- Device/inode values serialized through uintmax_t decimal representation.
- Bounded output using FCJ3_MAX_LINE_BYTES.
- No locale-sensitive formatting and no path interpretation after the caller has validated the input strings.

## Explicit non-goals

- No modification to MoveJournal or existing FCJ2 APIs.
- No FCJ3 append to the production journal.
- No O_TMPFILE creation/linking, destination capability probing, source retirement, or mover changes.
- No FCJ3 replay integration into journal_recover().
- No temporary-file cleanup, quarantine, unlink, or prefix sweep.
- No change to FCJ2/legacy behavior.

## Acceptance criteria

- CSPRNG errors fail closed and never fall back.
- Context captures immutable metadata and validates file types and component policy.
- All six states serialize only when their transition is legal and error semantics are correct.
- Prepare does not mutate state; commit advances only the exact prepared transition.
- A stale prepared transition, terminal transition, or poisoned context is rejected.
- Parser round-trips every prepared record and validates the resulting replay.
- C11/C17/C23 Debug and Release, sanitizer, Docker, Makefile, and Ubuntu validation pass.
