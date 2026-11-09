# FCJ3 Read-Only Recovery Classifier Design

## Status

Design proposal for review. This document does not implement recovery, perform filesystem operations, append journal records, or authorize cleanup. FCJ3 remains test-only and production FCJ2/legacy recovery remains unchanged.

## Objective

Separate three responsibilities:

1. Journal replay validates bytes, operation IDs, immutable metadata, sequence numbers, and legal state transitions.
2. Evidence collection opens the recorded destination directory and candidate entries safely, then reports observed facts without changing them.
3. Classification is a pure decision function. It maps one valid FCJ3 transaction plus explicit observations to a conservative outcome. It has no file descriptors, syscalls, journal writer, or cleanup API.

A later recovery executor may act only on an explicitly approved classification and only after a separate design, implementation, and adversarial review. The first implementation of this milestone must be read-only.

## Non-negotiable invariants

- Parse the complete journal successfully before classifying any operation. A malformed, truncated, oversized, unknown-version, duplicate, or illegal record stops FCJ3 recovery for the journal. Do not process a valid prefix and ignore a bad suffix.
- FCJ2/legacy replay remains independent. Never correlate formats by matching paths.
- The operation ID is the transaction key. Paths are immutable metadata, not transaction identity.
- The recorded destination directory must be the exact directory inode (st_dev, st_ino) before inspecting or classifying its children.
- A staging/destination name is only a lookup key. Ownership requires a regular file with exact recorded staging st_dev and st_ino.
- ENOENT, permission errors, I/O errors, symlinks, special files, and identity mismatches are different observations. Never collapse them into “missing”.
- Source path presence is not proof that the current inode is the original source. FCJ3 does not persist source device/inode identity. No source unlink, rename, overwrite, or retirement can be authorized from the current schema.
- The classifier cannot write, rename, unlink, quarantine, chmod, truncate, or append RECOVERED.
- Any uncertainty returns a preserve/manual-review outcome. No best-effort cleanup and no prefix sweep.
- Pure C only. No C++ or new runtime dependency.

## Evidence model

The evidence collector must return one of these values for each path observation:

- ABSENT: lookup returned ENOENT from the already-verified parent directory.
- MATCHING_REGULAR: fstatat with AT_SYMLINK_NOFOLLOW found a regular file whose device and inode equal the journal's recorded staging identity.
- MISMATCHING_REGULAR: regular file exists but device or inode differs.
- SYMLINK: the directory entry is a symlink.
- SPECIAL: entry exists but is not a regular file (directory, FIFO, socket, device, etc.).
- UNKNOWN_ERROR: lookup failed for any other reason, including EACCES, EIO, resource exhaustion, or unsupported behavior.

Do not follow symlinks. Do not use stat() on concatenated untrusted paths. Resolve the recorded destination directory through a separately reviewed path-resolution boundary, open it as a directory without following a final symlink, and compare its fstat() identity before child lookup.

The source observation is deliberately weaker:

- SOURCE_ABSENT
- SOURCE_PRESENT
- SOURCE_UNKNOWN_ERROR

For SOURCE_PRESENT, the collector may record file type for diagnostics, but the classifier must not infer original-source identity. A future schema version would be required to persist a trustworthy source identity before source retirement could even be considered.

The destination-directory observation is:

- DIRECTORY_IDENTITY_MATCH
- DIRECTORY_IDENTITY_MISMATCH
- DIRECTORY_UNKNOWN_ERROR

No child observation is trusted unless the directory identity matches.

## Pure classification output

The classifier returns exactly one category:

- NO_ACTION_TERMINAL: the journal state is terminal and no state transition or filesystem mutation is requested. This does not certify that every filesystem entry is healthy; any conflicting evidence still overrides this category.
- PRESERVE_INCOMPLETE: transaction is nonterminal and current evidence is consistent with an incomplete operation, but no automatic action is permitted in this phase.
- PRESERVE_PUBLISHED: the destination entry is the exact recorded staging inode and publication is evidenced; preserve all entries.
- MANUAL_REVIEW: observations conflict, cannot be trusted, or do not match a known safe case.
- INVALID_INPUT: invalid enum values, absent transaction identity, or inconsistent evidence object. This is a programming/input error, not a recoverable filesystem state.

A separate reason enum should explain the result (e.g. directory mismatch, stage inode mismatch, destination inode mismatch, unexpected stage+destination coexistence, unknown lookup error, source missing, state/evidence disagreement). Avoid string matching in tests; reason codes are stable API values.

## State/evidence decision table

The table assumes the destination directory identity has already matched. Any directory mismatch or unknown error is MANUAL_REVIEW before this table is considered.

| Latest journal state | Stage entry | Destination entry | Source observation | Classification |
| --- | --- | --- | --- | --- |
| STAGED | ABSENT | ABSENT | any known source state | PRESERVE_INCOMPLETE |
| STAGED | MATCHING_REGULAR | ABSENT | any known source state | PRESERVE_INCOMPLETE |
| STAGED | ABSENT | MATCHING_REGULAR | any | MANUAL_REVIEW (publication without STARTED is unexpected) |
| STARTED | MATCHING_REGULAR | ABSENT | SOURCE_PRESENT | PRESERVE_INCOMPLETE |
| STARTED | ABSENT | MATCHING_REGULAR | any known source state | PRESERVE_PUBLISHED |
| STARTED | ABSENT | ABSENT | SOURCE_PRESENT | PRESERVE_INCOMPLETE (operation may not have linked/published) |
| STARTED | ABSENT | ABSENT | SOURCE_ABSENT | MANUAL_REVIEW |
| PUBLISHED | ABSENT | MATCHING_REGULAR | any known source state | PRESERVE_PUBLISHED |
| PUBLISHED | ABSENT | ABSENT | any | MANUAL_REVIEW (durable publication record but destination missing) |
| COMPLETED | ABSENT | MATCHING_REGULAR | any known source state | NO_ACTION_TERMINAL |
| FAILED | ABSENT | ABSENT | any known source state | NO_ACTION_TERMINAL |
| RECOVERED | any non-conflicting observation | any non-conflicting observation | any known source state | NO_ACTION_TERMINAL |

The following evidence always overrides the table and returns MANUAL_REVIEW:

- UNKNOWN_ERROR for source, stage, or destination.
- MISMATCHING_REGULAR, SYMLINK, or SPECIAL for stage or destination.
- Stage and destination both present, even if both names appear to refer to the recorded inode. This is an unexpected topology requiring investigation.
- A destination present under STAGED state.
- A stage present under PUBLISHED or COMPLETED state.
- Any state/evidence combination not explicitly listed above.
- Missing source in STAGED or STARTED unless the exact row above explicitly allows it (none currently does).
- For FAILED, visible stage or destination entries are not assumed to be owned/cleanable; classify as manual review rather than silently accepting them.
- For RECOVERED, conflicting/mismatching/symlink/special/unknown evidence still yields manual review; terminal means no replay action, not that evidence checks can be skipped.

SOURCE_UNKNOWN_ERROR always yields MANUAL_REVIEW. The source observation does not authorize deletion even when present.

## Important schema limitations

1. Source identity is absent. The schema stores source path but not source st_dev/st_ino. A replacement file can occupy that path after a crash. Do not unlink it or claim it is the original.
2. Destination parent resolution needs its own contract. The journal stores destination path and destination-directory identity, but recovery must safely establish which directory descriptor corresponds to that path. Do not trust a path string alone or traverse attacker-controlled symlinks.
3. No content hash is stored. Exact inode identity proves the destination is the staged inode, not that arbitrary external writers have not modified its contents after publication. The current staging checks cover the controlled transaction lifecycle; recovery must still treat unexpected metadata/ownership/mode/size as evidence requiring review.
4. Durability assumptions are filesystem-specific. Process SIGKILL tests do not model power loss or hardware cache loss.

These limitations block source retirement and broad production enablement until addressed by a reviewed schema/architecture decision.

## Proposed module boundary

Read-only first cut:

- include/journal/fcj3_recovery_classifier.h
- src/journal/fcj3_recovery_classifier.c
- tests/unit/test_fcj3_recovery_classifier.c

The classifier module may include the parser transaction type and a small evidence/result structure. It must not include unistd.h, fcntl.h, sys/stat.h, or journal-writer/staging headers; it must perform no syscalls and own no resources. Evidence collection belongs in a later adapter module with explicit directory descriptors.

The unit test target links only the classifier and parser types. It must not link fcj3_journal.c, fcj3_writer.c, or fcj3_stage.c unless a separate integration test specifically needs them.

## Required tests before any executor exists

- Every explicit row in the decision table.
- Every state with each mismatch/symlink/special/unknown evidence override.
- Both stage and destination present.
- Directory identity mismatch and unknown.
- Source absent, present, and unknown.
- Invalid enum values and null pointers.
- Deterministic repeated classification (same inputs, same result).
- Table-driven tests proving the classifier never emits a mutation/action command.
- Fuzzing/property tests for enum combinations and invariant preservation.

## Future gates (not part of this PR)

1. Approve this evidence model and table.
2. Implement the pure classifier and unit tests.
3. Implement a read-only evidence collector with descriptor-relative lookup and adversarial path-race tests.
4. Add read-only recovery report output; no journal writes or filesystem mutations.
5. Design a durable recovery executor separately, including RECOVERED semantics and idempotency.
6. Only after source identity and destination path resolution are solved, consider source retirement. Never enable FCJ3 production writes before the complete recovery path and crash matrix are reviewed and validated.

## Acceptance criteria for this design

- Parser and FCJ2/legacy behavior remain untouched.
- No production target includes FCJ3 recovery code.
- No journal file is created or modified.
- No filesystem entry is modified.
- No cleanup or source retirement is authorized.
- All ambiguity maps to MANUAL_REVIEW.
- The implementation plan is split into pure classification, evidence collection, reporting, and later execution.
