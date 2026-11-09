# FCJ4 ACTION_DEF Wire Clarifications — Review Required

## Status

Proposed specification closure. This document does not implement FCJ4 and does not authorize a writer, filesystem adapter, planner, executor, journal creation, production integration, or filesystem mutation.

Parent design: `docs/quality/FCJ4_SCHEMA_AND_REPLAY_DESIGN.md` (merged PR #61).

## Why this follow-up exists

A parser must accept exactly one canonical interpretation of every record. A review of the merged FCJ4 contract found several ACTION_DEF cases that currently require an implementation to invent protocol semantics:

- The four action kinds are named, but the source-parent and target-parent mapping for each action is not fully specified.
- The schema requires source and target components, but two action kinds are deletion/retirement operations and have no target entry.
- The text refers to explicit zero sentinels for fields that are not meaningful, but it does not enumerate which fields use those sentinels or distinguish a sentinel from a valid value.
- `expected_entry_type` is required, but its version-1 token set and relationship to the four action kinds are not defined.
- The parser API needs a whole-buffer contract to make all-or-nothing replay enforceable. A public per-line mutator alone can expose a valid prefix before a later malformed line is seen.

The parser must not guess at any of these.

## Proposed version-1 clarifications

These are proposals for owner review, not facts silently added to the merged protocol.

### 1. Action subject and parent semantics

For every ACTION_DEF:

- `expected_parent_dev` and `expected_parent_ino` identify the **current source parent** containing `source_component` before the action.
- `expected_entry_dev`, `expected_entry_ino`, `expected_entry_type`, `expected_uid`, `expected_mode`, `expected_size`, and `expected_sha256` describe that source entry.
- `target_role` identifies the destination parent role, not the source entry role.
- A destination parent identity is resolved only from the immutable BEGIN fields. It is never inferred from a path string.
- The parser validates these relationships as journal semantics only. It does not open paths or verify the live filesystem.

| action_kind | Required source-parent identity | target_role | target_component |
|---|---|---|---|
| QUARANTINE_STAGE | BEGIN destination_parent_dev/ino | QUARANTINE | Required single component |
| QUARANTINE_SOURCE | BEGIN source_parent_dev/ino | QUARANTINE | Required single component |
| RETIRE_QUARANTINED_SOURCE | BEGIN quarantine_parent_dev/ino | NONE | Absent sentinel |
| REMOVE_QUARANTINED_STAGE | BEGIN quarantine_parent_dev/ino | NONE | Absent sentinel |

The source component is always required and nonempty. For QUARANTINE_STAGE it must equal the SEALED stage component. For the other actions it is the exact basename of the entry being acted on; it is not a path.

### 2. Canonical action tokens and component absence

Proposed version-1 tokens:

- `expected_entry_type`: `REGULAR` only. Every action subject in version 1 is a regular file. Directories, symlinks, devices, sockets, and FIFOs are not valid action subjects.
- `target_role`: `QUARANTINE` or `NONE`. The existing SOURCE/STAGE/DESTINATION roles describe entry provenance and are not destination-parent roles in ACTION_DEF.
- For the two quarantine actions, target component length is positive and target component hex is the canonical lowercase hex encoding of that component.
- For the two deletion actions, encode no target component as the exact pair `0|-`. The hyphen is a reserved schema token, not hex. Empty fields remain invalid.
- Source components are always nonempty, lowercase-hex encoded, and must decode to one safe component.

This makes the ACTION_DEF field count fixed for every action kind and avoids empty fields or ambiguous zero-length hex.

### 3. Metadata fields and sentinels

Proposed version-1 rule: all expected-entry metadata fields are meaningful for every action kind because every action subject is a regular file.

- UID, mode, size, device, inode, and SHA-256 are mandatory and must match the actual expected subject in a future executor.
- No zero sentinel is permitted for these fields. Zero is parsed as a real numeric value and then checked against the field's semantic constraints.
- SHA-256 is always exactly 64 lowercase hex characters. It is an integrity comparison, not authentication.
- The only absent-value sentinel introduced by this clarification is the target-component pair `0|-` for deletion actions.

If the owner wants optional metadata in a later schema, that requires a schema-version change or a separately specified presence encoding; the parser must not treat plausible values as “not applicable.”

### 4. Immutable BEGIN-to-action bindings

The parser should enforce these record-level bindings:

- QUARANTINE_STAGE source-parent identity equals BEGIN destination-parent identity.
- QUARANTINE_SOURCE source-parent identity equals BEGIN source-parent identity.
- RETIRE_QUARANTINED_SOURCE and REMOVE_QUARANTINED_STAGE source-parent identity equal BEGIN quarantine-parent identity.
- QUARANTINE_STAGE source component equals SEALED stage component.
- The destination parent for both quarantine actions is BEGIN quarantine-parent identity.
- Deletion actions have target role NONE and the absent target-component sentinel.
- Quarantine actions have target role QUARANTINE and a nonempty target component.
- No action ID may be defined twice, and each action's immutable definition remains fixed for its entire replay.

These checks validate protocol consistency only; they do not authorize the operation.

### 5. Whole-buffer parser/replay API

The parser implementation should expose a whole-buffer entry point conceptually equivalent to:

```c
int fcj4_replay_parse(const void *data, size_t length, Fcj4Replay *out);
void fcj4_replay_destroy(Fcj4Replay *replay);
```

Contract:

- `length` must not exceed 64 MiB.
- Input is a complete journal byte sequence, not a NUL-terminated string.
- Every physical record must end in LF; no trailing partial record is accepted.
- The parser builds a temporary replay and publishes it to `out` only after every record and every cross-record invariant succeeds.
- On any error, `out` remains empty/unchanged according to the documented initialization contract; no valid prefix is exposed as success.
- No filesystem, journal, network, clock, or process APIs are called.
- The API does not provide a streaming/per-line public mutator in version 1.

The implementation must document a conservative upper bound for replay allocations before choosing its data structures. Counts and allocation arithmetic remain checked.

## Acceptance tests added to the parser milestone

In addition to the PR #61 acceptance matrix, tests must cover:

1. Every action kind with its exact valid parent-role and target-component encoding.
2. Each action kind with a mismatched source-parent identity.
3. Quarantine actions with target role NONE, missing target component, or zero-length target with a non-sentinel field.
4. Deletion actions with a target role other than NONE or any target component other than the exact `0|-` sentinel.
5. Invalid `expected_entry_type` tokens and non-regular subjects.
6. All numeric metadata fields parsed canonically, including legitimate zero values where the field permits them.
7. Digest length, case, and non-hex rejection.
8. BEGIN/SEALED binding failures for action definitions.
9. Valid prefix followed by malformed final record: parse fails and publishes no replay.
10. Empty input, missing final LF, CRLF, embedded NUL, blank line, unknown kind/version, oversized journal/line, operation/action/record limits, and allocation-failure behavior.
11. Deterministic operation and action ordering.
12. C11, C17, and C23 Debug/Release builds as required by the repository's validation policy, with sanitizers on the Debug path.

## Scope boundary

This follow-up is documentation-only. It must not add parser code, tests, a writer, CLI, journal file, filesystem adapter, planner, executor, or production source-list entry.

After owner review and merge of this clarification, the next implementation milestone may add only the FCJ4 pure-C whole-buffer parser/replay module, focused tests/fixtures, and isolated CMake/CTest registration. No FCJ4 writer, filesystem access, planner, executor, mutation syscall, production integration, or source retirement is authorized.
