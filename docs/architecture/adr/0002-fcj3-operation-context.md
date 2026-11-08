# ADR 0002: Separate FCJ3 operation context from durable journal append

- Status: Proposed for implementation review
- Date: 2026-11-08
- Related design: [FCJ3 schema](../quality/FCJ3_JOURNAL_SCHEMA.md), [writer/context design](../quality/FCJ3_WRITER_CONTEXT_DESIGN.md), [journal-owned temporary recovery](../quality/JOURNAL_OWNED_TEMP_RECOVERY_DESIGN.md)

## Context

FCJ3 records need a secure operation ID and immutable path, destination-directory, and staging-inode metadata. The existing MoveJournal and mover use FCJ2 and do not yet have the O_TMPFILE staging lifecycle that can supply these values in the required order.

Enabling a writer before the staging lifecycle is ready could create FCJ3 records with guessed or incomplete ownership metadata. Advancing in-memory state before the record is durably appended could also cause sequence divergence after write/fsync failures.

## Decision

Implement an isolated operation-context and record-encoding module:
- obtain 128-bit identity only from Linux getrandom(), with EINTR/short-read handling and no weak fallback;
- capture and own immutable metadata from caller-provided stat snapshots;
- validate destination-directory and staging-file types, require the pre-link O_TMPFILE inode to have st_nlink == 0 and the same st_dev as its destination directory, and validate the reserved .fc-tmp- component namespace;
- separate transition preparation from commit so state advances only after a future journal adapter reports a successful durable append;
- poison the context after an uncertain append/fsync result;
- compile the module only into dedicated tests until O_TMPFILE staging and journal integration are separately reviewed.

## Alternatives

### Enable FCJ3 writes directly in the current mover

Rejected. The current mover does not yet create anonymous staging and cannot supply all required metadata in the specified order.

### Advance state when a record is formatted

Rejected. Formatting is not durability. A later write/fsync failure would leave in-memory sequence state ahead of the journal.

### Fall back to weaker random sources

Rejected. Operation identity is a durable transaction key. Weak or predictable fallback would undermine the protocol.

## Consequences

- The CSPRNG, metadata capture, state validation, and serialization can be tested independently.
- No production journal bytes or runtime behavior change in this phase.
- The future append adapter must hold the journal's serialization lock, append the exact prepared bytes, fsync, and only then commit.
- A partial write or fsync failure leaves durability uncertain; the context must not be reused until conservative recovery resolves the journal.
- O_TMPFILE and exact-identity recovery remain later milestones.

## Rollback

Revert the isolated module and tests. Existing FCJ2 writer and recovery remain unchanged.
