# FCJ4 Recovery Execution Safety Design

## Status

Proposed design only. This document requires owner review and approval before any implementation.

This is the next architecture milestone after the FCJ3 read-only recovery report. It does **not** authorize recovery execution, source retirement, staging cleanup, quarantine, or FCJ3 production writes.

## 1. Executive decision

**Do not implement a destructive recovery executor against the current FCJ3 record format.**

The current FCJ3 transaction records:
- operation ID and ordered state sequence;
- source and destination path strings;
- destination-parent device/inode identity;
- staging device/inode identity and staging component.

It does not durably record enough evidence to prove all of the following after a process restart:
1. the source path still names the exact source inode that was copied;
2. the source parent is the original source parent;
3. the source bytes still match the bytes copied into staging;
4. the published destination bytes still match the copied bytes;
5. a visible staging/destination entry can be safely retired without racing a directory-entry replacement.

The FCJ3 stage module calculates a SHA-256 digest in memory while copying, but that digest is not part of the persistent FCJ3 record. In-memory state is lost on process crash. A matching device/inode is identity evidence, not content-integrity evidence.

The FCJ3 report and classifier are therefore diagnostic tools only. Their classifications such as `PRESERVE_INCOMPLETE` and `PRESERVE_PUBLISHED` are not executable commands.

**Recommendation:** design a new, explicitly versioned FCJ4 durable transaction contract before writing an executor. Do not silently extend FCJ3's 16-field record or reinterpret existing FCJ3 records. FCJ3 remains read-only/test-only until a separate compatibility and production-adoption decision is approved.

## 2. Business and safety objective

The business goal is to recover interrupted file moves without:
- losing the only complete copy;
- overwriting an unrelated destination;
- deleting a replacement file;
- confusing a staging-name collision with transaction ownership;
- marking a transaction recovered when its filesystem effects are uncertain;
- breaking FCJ2/legacy journal compatibility.

The governing priority is:

1. preserve user data;
2. preserve evidence and report uncertainty;
3. maintain journal compatibility;
4. maintain recoverability and idempotency;
5. optimize runtime only after correctness is proven.

A recovery attempt that cannot prove ownership must stop for manual review. A slower manual-review outcome is preferable to an irreversible data-loss event.

## 3. Current FCJ3 evidence gap

| Required proof after restart | Current FCJ3 support | Consequence |
|---|---|---|
| Destination parent identity | Recorded device/inode | Can pin and validate this directory object |
| Staging inode identity | Recorded device/inode | Can recognize the recorded staging inode if found under the verified destination parent |
| Source parent identity | Not recorded | A source path walk cannot prove it is the original parent |
| Source inode identity | Not recorded | Source presence is advisory; it may be a replacement |
| Source content digest | Not persisted | Cannot prove current source bytes are the bytes copied |
| Staged/published content digest | Calculated in memory, not journaled | Cannot verify bytes after restart |
| Quarantine directory identity | Not recorded | Cannot bind a cleanup operation to a previously approved quarantine object |
| Durable action intent for recovery mutations | Not defined | A crash during recovery cannot be reconciled safely |

Do not bridge these gaps with path equality, filename prefixes, file size alone, matching inode alone, or a fresh hash computed only after restart. A hash of a candidate after the crash does not prove it is the same byte stream that was originally copied.

## 4. FCJ4 journal design requirements

FCJ4 must be a new format/version with a strict parser and independent replay path. Its schema must be specified and reviewed before implementation.

### 4.1 Immutable transaction identity and object evidence

The initial durable transaction record must capture, before any recovery-relevant mutation:

**Source**
- absolute source path as diagnostic/re-resolution input;
- source-parent device/inode identity;
- source file device/inode identity;
- file type, owner, mode, size;
- high-resolution mtime and ctime;
- SHA-256 digest of the exact byte stream copied;
- an explicit statement that path strings are not object identity.

**Destination**
- absolute destination path as diagnostic/re-resolution input;
- destination-parent device/inode identity;
- final destination basename;
- expected published file device/inode identity, inherited from the staging inode;
- expected file size and SHA-256 digest.

**Staging**
- unique staging component;
- staging device/inode identity;
- owner and restrictive mode;
- expected size and SHA-256 digest;
- staging-parent identity (must equal the recorded destination-parent identity).

**Quarantine**
- a dedicated, private quarantine directory identity (device/inode), owner, and required mode;
- a transaction-specific quarantine component derived from the operation ID, never ownership inferred from a prefix alone.

The journal must use fixed-width, canonical numeric encodings, strict length bounds, checked conversions, canonical lowercase hex for binary fields, and explicit schema versioning. Digest algorithm and digest length must be encoded as protocol constants/versioned fields, not inferred from a string prefix.

The digest is an integrity comparison, not a MAC, signature, or defense against a privileged attacker who can rewrite both journal and files.

### 4.2 Capture and copy protocol

1. Open source parent and source file descriptor-relatively, without following symlinks.
2. Verify regular-file type and capture source parent and inode metadata.
3. Copy from the already-open source descriptor into anonymous staging on the validated destination filesystem.
4. Compute SHA-256 over the exact bytes successfully copied.
5. Recheck source descriptor metadata and source directory-entry identity after copying. Any mismatch poisons the operation.
6. Verify staged size and digest from the staged descriptor.
7. Persist the complete immutable FCJ4 identity/evidence record and required state transition with the documented durability ordering before proceeding.
8. At every later gate, revalidate the relevant open descriptor, directory identity, named-entry identity, size, metadata, and digest.

The exact ordering of the initial record relative to stage creation must be resolved in the FCJ4 state-machine ADR. No recovery-relevant mutation may occur before enough durable intent exists to explain it after a crash.

## 5. State machine and durable recovery protocol

FCJ4 must define two related but distinct state machines:

1. **Move transaction state** — describes copy and publication progress.
2. **Recovery action state** — describes one explicit, idempotent recovery action and its durable intent/completion.

Do not overload the move state to mean that a recovery cleanup action happened.

### 5.1 Move states

The existing concepts remain useful but require an FCJ4 transition table:
- `STAGED`: immutable source/stage/destination evidence is durable; stage exists but is not published.
- `STARTED`: copy verification passed and the operation is permitted to proceed toward publication.
- `PUBLISHED`: exact expected inode is visible at the destination and destination directory durability step succeeded.
- `COMPLETED`: the move's defined success postconditions are durable. Source retirement semantics must be explicit and must not be inferred from this state name.
- `FAILED`: operation stopped without claiming success; it is terminal only if all remaining objects and uncertain effects are represented.
- `RECOVERED`: reserved until its exact meaning and proof obligations are defined by the recovery-action state machine. Existing FCJ3 semantics must not be silently reused.

### 5.2 Recovery action record

Every recovery mutation must have a separate durable action identity and an explicit intent/completion protocol. At minimum:
- recovery action ID;
- parent move operation ID;
- action kind and exact target role (source, stage, destination, quarantine);
- expected parent device/inode;
- expected entry device/inode/type/owner/mode/size/digest;
- intended source and target component names;
- action sequence and state;
- error/uncertainty result.

A recovery action must have a legal transition table such as:
- `ACTION_PREPARED`: immutable action parameters validated and intent durably appended;
- `ACTION_STARTED`: immediately before the filesystem mutation, after last-moment identity checks;
- `ACTION_APPLIED`: mutation performed and resulting entry identity verified;
- `ACTION_SYNCED`: affected directory metadata successfully synchronized;
- `ACTION_COMPLETED`: postconditions and terminal result durably recorded;
- `ACTION_MANUAL_REVIEW`: uncertainty or conflict prevents safe continuation.

This is a proposed shape, not an approved wire schema. Exact transitions, error semantics, and crash windows require a separate state-machine review.

A state must never be appended as completed before its filesystem postconditions and required directory sync have succeeded. If a syscall succeeds but a later verification, fsync, or journal append is uncertain, do not repeat a non-idempotent action blindly. Reopen, replay, inspect exact evidence, and classify the result; otherwise stop for manual review.

## 6. Recovery action policy

The first executor must be deliberately narrow. Each action requires a separate acceptance gate.

### 6.1 Preserve-only outcomes

- `MANUAL_REVIEW`: no mutation, no automatic retry, no cleanup.
- `PRESERVE_INCOMPLETE`: preserve source and stage; do not discard partial staging automatically.
- `PRESERVE_PUBLISHED`: preserve destination and all remaining evidence; do not remove source based on current FCJ3 evidence.
- `NO_ACTION_TERMINAL`: no filesystem mutation. A separate reporting/acknowledgement operation may be considered later, but must not alter transaction semantics.

The existing FCJ3 classifier remains a read-only policy function. It is not an executor action table.

### 6.2 Source retirement

Source retirement is **prohibited** for FCJ3 and is not enabled merely by FCJ4 metadata capture.

An FCJ4 source-retirement design must prove:
- source parent descriptor identity matches the recorded source parent;
- source entry is a regular file and matches the recorded source device/inode;
- source content digest and required metadata still match the copied source snapshot;
- destination parent identity matches the recorded destination parent;
- destination entry matches the expected published inode and digest;
- the destination file and destination parent have crossed their required durability boundaries;
- the source entry is moved into a private, identity-validated quarantine using a no-replace operation;
- the quarantined object is reopened/verified against the expected source identity and digest;
- only then may a separately journaled retirement action unlink it;
- any mismatch, race, permission problem, unsupported filesystem operation, or uncertain fsync/journal result stops without unlinking.

A check-then-`unlinkat` sequence is not an acceptable proof. A same-UID attacker remains outside the guaranteed isolation boundary unless the product introduces a stronger process/permission isolation model. The threat model must state this clearly.

### 6.3 Staging cleanup

Staging cleanup is also prohibited for FCJ3. A matching inode alone is not enough to prove the file contents or that removing it cannot destroy the only good copy.

An FCJ4 cleanup action must be journal-owned, tied to exact parent and inode identity, and use the approved quarantine protocol. Never sweep by `.fc-tmp-*` or `.fc-retire-*` prefix. If identity or content checks fail, preserve the candidate and require manual review.

### 6.4 Destination handling

- Never overwrite a destination.
- Never remove a destination merely because the journal says the operation was incomplete.
- A destination with mismatching inode, content, type, or parent identity is preserved and reported.
- Publication recovery may only be considered when exact expected inode and digest are proven, and the state/action journal protocol establishes what remains to do.
- No automatic rename or link fallback may weaken the publication guarantee.

## 7. Crash consistency and idempotency requirements

Before implementation, create a crash matrix covering process termination and simulated I/O failure:
- before initial intent append;
- partial initial record write;
- after complete write but before journal fsync;
- after journal fsync;
- after stage sync;
- before and after stage link;
- before and after no-replace publication;
- after publication but before destination directory fsync;
- after destination directory fsync but before journal state append;
- partial recovery-action intent;
- recovery-action intent fsynced but before mutation;
- mutation succeeded but verification failed;
- mutation verified but directory fsync failed;
- directory fsync succeeded but completion record is partial;
- completion record written but journal fsync failed;
- process restart and repeated recovery after each boundary.

For every row, specify:
- durable journal state;
- possible filesystem states;
- exact safe next action;
- whether action is repeatable;
- what evidence must be revalidated;
- when manual review is mandatory.

Recovery must be idempotent by evidence and action identity, not by assuming a syscall can safely be repeated. A repeated action must either observe its already-completed exact postcondition or stop without altering unrelated data.

## 8. Journal integrity, ordering, and compatibility

- FCJ4 must use a distinct file/format identity and parser. Do not append FCJ4 records to FCJ2 `journal/move.log` or FCJ3 records.
- Keep existing FCJ2/legacy replay unchanged.
- Keep FCJ3 read-only diagnostics unchanged until an explicit deprecation/migration decision.
- Never auto-migrate a journal by rewriting it in place.
- Journal append must handle short writes, EINTR, partial-write uncertainty, fsync failures, close failures, and concurrent-writer exclusion.
- A poisoned writer must stop further transitions and require replay/reconciliation.
- Journal ownership, permissions, parent-directory identity, locking/single-writer policy, and rotation/size limits must be specified.
- A malformed/truncated/conflicting journal fails closed. Do not accept a valid prefix or truncate a bad tail automatically.
- Define whether one journal file contains multiple operations or one operation, and enforce operation ordering and duplicate-ID rules.
- Define how the journal is backed up and how operators recover from journal loss/corruption without guessing.

## 9. Filesystem and attacker model

Assume:
- untrusted names and paths;
- symlink, hard-link, FIFO, socket, and special-file entries;
- concurrent renames and replacement attempts;
- destination collisions;
- separate unprivileged users with access to shared directories;
- same-UID interference with process-owned writable state;
- ENOSPC, EIO, EACCES, EINTR, short I/O, fsync/close failure, process crash, and power-loss-like uncertainty;
- unsupported `O_TMPFILE`, procfs, `renameat2`, or filesystem semantics.

Controls:
- descriptor-relative operations and no-follow resolution;
- exact device/inode/type/owner/mode/size/content validation;
- no-replace operations;
- private, prevalidated quarantine;
- strict single-writer journal lifecycle;
- bounded records, transactions, descriptors, memory, and work;
- non-root runtime with least privilege;
- fail closed if a required capability is unavailable; no weaker fallback.

Do not claim protection against a fully privileged kernel/root attacker. Do not claim that a private directory alone defeats a fully capable same-UID attacker.

## 10. Architecture and module boundaries

Keep pure C and the existing modular-monolith architecture.

Proposed boundaries:
- **FCJ4 parser/replay**: pure parsing and transaction-history validation; no syscalls.
- **Recovery policy**: pure state/evidence decision function; no syscalls or mutation.
- **Filesystem evidence adapter**: descriptor-relative observations; read-only.
- **Recovery action planner**: pure mapping from validated transaction + evidence to a typed action proposal; no syscalls.
- **Recovery executor**: narrow mutation adapter, only after action intent is durable; no policy decisions hidden in syscall wrappers.
- **Journal writer**: canonical append, durability, poisoning, and replay reconciliation.
- **Quarantine adapter**: descriptor-relative, no-replace, identity-verified movement and controlled retirement.

Use dependency inversion at these real boundaries for deterministic fault injection. Do not create a generic workflow framework, event broker, database, daemon, or microservice. FCJ4 journal records are durable transaction protocol records, not a distributed event bus.

## 11. Implementation milestones after design approval

The design approval PR does not authorize implementation. After it is merged, split work into dependent, independently reviewable milestones:

1. **FCJ4 schema and parser/replay (read-only):** versioned schema, strict parser, transition tests, bounds, compatibility tests; no filesystem mutations.
2. **FCJ4 immutable evidence capture:** capture source/stage/destination metadata and digest, prove copy consistency, persist intent; still no source retirement or cleanup.
3. **FCJ4 recovery planner (pure):** produce typed action proposals from complete replay and fresh evidence; no mutation.
4. **Quarantine protocol prototype:** isolated test-only implementation with exact identity checks, no-replace operations, directory permissions, fault injection, and crash tests; no production cleanup.
5. **Recovery executor for preserve-only / non-destructive reconciliation:** only actions whose postconditions do not remove or overwrite user data; exact semantics and journal ordering required.
6. **Staging cleanup executor:** only after quarantine and crash matrix gates pass; no prefix sweep.
7. **Source retirement executor:** last, separately reviewed; only after source identity/digest and quarantine protocol prove all required postconditions.
8. **Production FCJ4 adoption:** only after full compatibility, migration, release, backup, operator runbook, non-root, actual-filesystem, crash, and adversarial gates pass.

Each step starts from the latest merged `main`. No step is stacked on an unmerged branch.

## 12. Required tests and measurable acceptance criteria

### Parser and protocol
- Canonical FCJ4 records, malformed fields, noncanonical numbers, invalid hex, overflow, truncation, oversized records, duplicate IDs, invalid sequence, illegal transitions.
- C11/C17/C23 Debug and Release.
- Existing FCJ2/legacy and FCJ3 fixtures replay unchanged.

### Evidence capture
- Source replaced before open, during copy, and after copy.
- Same-size content rewrite; size change; metadata change; source parent replacement.
- Stage mutation after copy; digest mismatch; descriptor/name identity mismatch.
- Destination parent replacement; cross-device paths; symlink and special-file entries.

### Executor and crash behavior
- Fault injection for every read/write/link/rename/unlink/fsync/close/journal append boundary.
- Crash/restart at every point in the matrix.
- Repeat recovery at least twice after every injected crash; assert stable terminal outcome.
- Exact source/destination/quarantine bytes, inode identities, owner, mode, and directory entries.
- Existing unrelated files with colliding names remain byte-for-byte and identity unchanged.
- ENOSPC, EIO, EACCES, EINTR, short I/O, unsupported syscalls/filesystems.
- Run under the intended non-root service UID and on each supported destination filesystem.
- Sanitizers, static analysis, Docker, Ubuntu 24.04, and all C-standard workflows.

### Release and operator acceptance
- No destructive action without explicit CLI opt-in and clear dry-run/report preview.
- Default behavior remains non-destructive.
- Every recovery action is auditable and tied to a unique action ID.
- Operator documentation explains manual-review outcomes, journal backup, restore, and failure handling.
- Rollback preserves source, destination, stage, quarantine, and journal evidence; never performs broad cleanup.

## 13. Non-goals

- No implementation in this PR.
- No changes to FCJ2/legacy parser or recovery.
- No FCJ3 production writer, append, or executor.
- No automatic source deletion, staging cleanup, quarantine, rename, link, overwrite, chmod, truncate, or journal rewrite.
- No C++ or Rust core, microservices, broker, database, or daemon.
- No claim that SHA-256 authenticates the journal or defeats a privileged/same-UID attacker.
- No claim that POSIX/Linux descriptor-relative APIs eliminate every race.
- No automatic recovery from ambiguous evidence.

## 14. Rollback

This PR adds one design document only. Rollback is removal of that document. No production code, journal format, filesystem behavior, CLI behavior, or recovery path changes.
