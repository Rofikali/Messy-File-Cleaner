# ADR 0001: FCJ3 transaction identity and versioned journal records

- Status: Proposed
- Date: 2026-11-08
- Decision owners: repository owner and maintainers
- Related design: [FCJ3 Journal Schema](../../quality/FCJ3_JOURNAL_SCHEMA.md), [Journal-Owned Temporary Recovery](../../quality/JOURNAL_OWNED_TEMP_RECOVERY_DESIGN.md)

## Context

The current FCJ2 and legacy journal replay model groups records by source and destination path. This is insufficient as a durable transaction identity because the same path pair can participate in multiple operations. A new STAGED state alone would be unsafe: transitions from distinct operations could be conflated, and the current records do not identify the staging inode or its parent directory.

The project needs a versioned protocol that can eventually support crash-safe anonymous staging and narrowly authorized cleanup, while retaining existing journal compatibility.

## Decision proposed

Introduce FCJ3 as a separate record protocol with:
- a 128-bit operation ID from the OS CSPRNG;
- a monotonically increasing per-operation sequence number;
- explicit state and error code;
- source and destination path bytes;
- immutable destination-directory and staging-inode device/inode identities;
- one validated staging filename component;
- all immutable metadata repeated in every transition record.

FCJ3 replay groups only by operation ID. FCJ2/legacy replay retains the existing source/destination grouping and behavior. Records from different versions are never joined by path equality.

The legal transition graph and exact field grammar are specified in docs/quality/FCJ3_JOURNAL_SCHEMA.md.

## Alternatives considered

### Add STAGED to FCJ2 and keep grouping by path pair

Rejected. Repeated operations over the same source/destination pair can be confused. There is no stable per-operation identity or immutable staging ownership proof.

### Use source/destination plus timestamp as identity

Rejected. Timestamps are not unique, may be affected by clock behavior, and are not a security identity.

### Use PID or process-local counter

Rejected. PIDs are reused and counters reset after restart. Neither is collision-resistant across journal lifetime.

### Use a random operation ID without sequence/metadata validation

Rejected. An ID alone does not detect transition gaps, replay conflicts, or metadata substitution.

### Rewrite all old records into FCJ3

Rejected for initial migration. Rewriting persistent journal history creates unnecessary migration and corruption risk. Keep old formats readable under their existing conservative rules.

## Consequences

Positive:
- Repeated operations on the same paths remain distinct.
- Parser can detect illegal transition order and conflicting metadata.
- Recovery can eventually verify exact staging ownership rather than infer from a name prefix.
- Compatibility is explicit and testable.

Costs and constraints:
- Journal records and replay memory grow.
- A secure random source is mandatory; no weak fallback is allowed.
- Parser limits and overflow checks must be explicit.
- Writer and mover APIs need an immutable operation context.
- FCJ3 must not be emitted until the staging lifecycle can supply all metadata in the correct order.

## Security and data-safety impact

This decision does not itself enable cleanup. Parser-only work must be read-only with respect to user files and must not append recovery records on malformed input. No FCJ2 or legacy record can authorize staging cleanup. Exact directory and inode checks plus the private quarantine boundary remain mandatory before any future unlink.

## Implementation plan

1. Review and approve the exact schema and state graph.
2. Implement a pure FCJ3 parser/replay model with tests; do not change the writer.
3. Add a secure operation context and FCJ3 writer with deterministic failure tests.
4. Integrate destination-specific O_TMPFILE staging and capability checks.
5. Implement exact-identity recovery and private quarantine.
6. Enable only after crash, adversarial, non-root, filesystem-matrix, language-standard, sanitizer, static-analysis, and Docker gates pass.

## Rollback

Until FCJ3 writing is enabled, rollback is simply reverting parser-only code and documentation; existing FCJ2/legacy journals remain unchanged. Once FCJ3 records are emitted, older binaries must not open them as FCJ2. Deployment must enforce a minimum compatible binary version or provide an explicitly designed dual-version operational policy. Never downgrade by truncating or rewriting the journal.
