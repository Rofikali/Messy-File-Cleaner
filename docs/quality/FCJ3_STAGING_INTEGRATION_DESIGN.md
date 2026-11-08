# FCJ3 Staging Integration and Durable Append Design

## Status

Accepted implementation contract (PR #45, merged 2026-11-08). This document authorizes only the isolated test implementation described below; it does not authorize production mover, journal, or recovery changes. No FCJ3 records are written by the application in this phase.

## Objective

Integrate the already-tested FCJ3 operation context with Linux anonymous staging and a durable journal adapter while preserving the project's non-root, least-privilege policy and all existing FCJ2/legacy behavior.

**This milestone is test-only.** The staging adapter and FCJ3 append adapter must be linked only into dedicated integration tests. They must not be added to the production executable, invoked by the production mover, or create a production `move.fcj3` file. This is deliberate: current startup recovery does not understand FCJ3. Writing production FCJ3 records before the recovery dispatcher exists would leave durable operations ignored after restart. Production enablement is deferred until the recovery milestone passes.

The milestone has two separable boundaries:

1. A destination-specific O_TMPFILE lifecycle that creates, links, publishes, and syncs one exact staging inode.
2. A dedicated FCJ3 journal adapter that appends the exact prepared line, fsyncs it, and only then commits the in-memory context.

Recovery/cleanup remains a later milestone. This phase must not delete temporary files or enable FCJ3 recovery.

## Critical correction: unprivileged linking

Do **not** make `linkat(fd, "", dirfd, name, AT_EMPTY_PATH)` the production linking method. Linux documents that `AT_EMPTY_PATH` requires `CAP_DAC_READ_SEARCH`. Granting that capability to the cleaner conflicts with the non-root least-privilege threat model.

Use one explicit unprivileged mechanism instead:

```c
snprintf(proc_path, sizeof(proc_path), "/proc/self/fd/%d", staging_fd);
linkat(AT_FDCWD, proc_path, destination_dirfd,
       staging_component, AT_SYMLINK_FOLLOW);
```

Linux documents this `/proc/self/fd` method as the alternative when the process does not have `CAP_DAC_READ_SEARCH`. This is not an automatic fallback: it is the single selected linking mechanism for the supported runtime. If a real procfs `/proc/self/fd` is unavailable or the operation fails, fail closed. Do not retry with `AT_EMPTY_PATH`, named temporary files, or another weaker method.

Before relying on this mechanism:
- Open `/proc/self/fd` and use `fstatfs()` to verify it is procfs (`PROC_SUPER_MAGIC`); do not trust a path string alone.
- Keep exclusive ownership of the staging descriptor from pre-link `fstat()` through `linkat()`; do not close or transfer it while the proc-fd path is in use.
- Build the proc-fd path with checked formatting and a valid live descriptor number.
- The descriptor must refer to the exact O_TMPFILE inode created in the already-open destination directory, without `O_EXCL`.
- After linking, verify the exact component with `fstatat(dirfd, name, ..., AT_SYMLINK_NOFOLLOW)`; require regular-file type and exact `st_dev`/`st_ino` equality with the still-open descriptor.
- Treat `EEXIST` as a name collision, not permission to replace an entry.
- Probe this exact method as the production non-root UID in the actual destination directory.

Reference: Linux man-pages `linkat(2)` and `open(2)`; see the links in the ADR.

## Supported runtime policy

The initial supported runtime is Linux with:
- destination filesystem support for `O_TMPFILE`;
- durable `fsync()` on the anonymous file and destination directory;
- procfs mounted and verified at `/proc/self/fd`;
- `linkat(..., AT_SYMLINK_FOLLOW)` from that proc-fd entry into the same destination mount;
- `renameat2(RENAME_NOREPLACE)` for publication;
- the application running as an unprivileged service/user identity.

No capability is granted to make `AT_EMPTY_PATH` work. Unsupported destinations fail before the source is retired. Do not globally cache a probe result; probe the actual open destination directory for each distinct directory/mount context. A successful probe on `/tmp` is not evidence about the user's destination.

## Destination-specific capability probe

The probe must run against the already-open, validated destination directory and must use the same effective UID, mount namespace, procfs view, and security policy as the real move.

1. `fstat(dirfd)`; require a directory and retain its device/inode.
2. Verify `/proc/self/fd` is procfs using an opened descriptor plus `fstatfs()`.
3. Create an anonymous file with `openat(dirfd, ".", O_TMPFILE | O_RDWR | O_CLOEXEC, 0600)`. Do not use `O_EXCL`.
4. Write a short fixed test payload with EINTR/short-write handling; fsync the file; fstat it and require regular-file type, effective-UID ownership, `st_nlink == 0`, and same device as `dirfd`.
5. Generate an unpredictable disposable probe component using `getrandom()`; use exclusive name semantics by relying on `linkat()` returning `EEXIST`, never overwrite.
6. Link through `/proc/self/fd/<fd>` with `AT_SYMLINK_FOLLOW`.
7. Verify the exact linked name with `fstatat(..., AT_SYMLINK_NOFOLLOW)` and compare device/inode/type with the open descriptor.
8. Remove only that exact probe entry after re-verifying its identity, then fsync `dirfd`.
9. Close descriptors and report capability only if every required step succeeded.

If cleanup of a probe entry cannot prove the exact inode, preserve it and report failure; never perform a prefix sweep. Probe errors are not silently converted into success. The implementation must distinguish unsupported capability from an unexpected I/O/security failure for operator diagnostics.

## Staging and transaction ordering

For one selected final destination path:

1. Open and validate source and destination directory descriptors using the existing no-symlink, descriptor-relative policy. Open the source with `O_NOFOLLOW`; fstat and require a regular file.
2. Run the destination-specific capability probe before any source retirement.
3. Create the real anonymous staging inode in the destination directory with `O_TMPFILE | O_RDWR | O_CLOEXEC`, mode 0600, and without `O_EXCL`.
4. Copy source bytes into the anonymous descriptor with EINTR/short-I/O handling. Revalidate the source inode/size/timestamps using the existing mover contract, apply the allowed source mode, fsync the staging descriptor, then fstat it.
5. Generate a unique single-component `.fc-tmp-...` name using CSPRNG bytes. It is an identifier, not proof of ownership. Keep the descriptor open.
6. Initialize `Fcj3OperationContext` from the validated source/final-destination paths, destination-directory stat, staging component, and anonymous staging inode stat. The initializer creates the operation ID using `getrandom()`.
7. Prepare `STAGED`; append its exact bytes to the dedicated FCJ3 journal and fsync that journal. Only after the durable append succeeds, commit the context.
8. Link the anonymous descriptor to the exact staging component through the verified proc-fd mechanism. Verify linked inode identity and fsync the destination directory.
9. Prepare, durably append, and commit `STARTED`.
10. Publish with `renameat2(RENAME_NOREPLACE)` from staging component to the selected final destination name. On `EEXIST`, do not overwrite. Record terminal `FAILED` for this operation, retain/resolve the stage conservatively, and retry the name only as a new operation with a new ID and fresh staging context. Do not reuse a terminal operation context.
11. On successful publication, fsync the destination directory; then durably append and commit `PUBLISHED`.
12. Retire the source using the existing quarantine-and-identity-verification mechanism; fsync the source directory.
13. Durably append and commit `COMPLETED`.
14. Close descriptors and release the operation context.

The staging inode becomes the published destination inode through no-replace rename. Its recorded device/inode therefore also identifies the final destination after publication. The staging component should be absent after a successful rename. Recovery must use the operation state and exact identity; it must never infer that a missing staging component authorizes deleting the destination.

## Durable FCJ3 journal adapter

### Format isolation

Do not append FCJ3 records to the existing `journal/move.log`. The current `journal_recover()` parser understands FCJ2 and legacy records, not FCJ3. Mixing formats before a unified recovery dispatcher exists could make a valid journal unreadable or cause the old parser to reject the file.

Use a disposable FCJ3 journal file inside each integration-test fixture. Do not create or append a production FCJ3 journal yet. When recovery support is implemented, it will introduce a distinct `journal/move.fcj3` file under the protected journal directory; existing `journal/move.log` remains FCJ2/legacy only. The recovery milestone will parse the two logs independently and coordinate operation recovery; it will not reinterpret FCJ2 records as FCJ3.

### Adapter contract

Introduce an opaque or narrowly scoped `Fcj3Journal` C type with:
- an append-only file descriptor opened with `O_APPEND | O_CLOEXEC | O_NOFOLLOW`;
- regular-file, owner, and restrictive-mode validation;
- an exclusive `flock(LOCK_EX | LOCK_NB)` held for the journal lifetime;
- a process mutex serializing append calls;
- explicit open/close lifecycle, with all append-capable threads stopped before close.

Append must:
1. Validate the prepared record length and final LF; do not reformat or mutate the bytes.
2. Hold the mutex for the complete write+fsync sequence.
3. Write all bytes, handling EINTR and short writes.
4. fsync the journal descriptor.
5. Only then allow `fcj3_operation_commit_transition()`.
6. On any append/fsync failure, mark the operation context uncertain/poisoned and stop further transitions for that operation. Do not truncate, repair, or retry a possibly partial record in place. Recovery will inspect the exact journal bytes in a later milestone.

Use descriptor I/O only; do not mix stdio and raw descriptor writes. When creating the FCJ3 journal file, fsync its parent directory after successful creation so the directory entry is durable. Keep FCJ2 APIs unchanged.

## Destination collision and operation identity

The FCJ3 state graph makes `FAILED` terminal. Therefore one operation ID represents one selected source-to-final-destination attempt. If the final destination collides after `STARTED`, append `FAILED` (if the journal is still healthy), retain the source, and do not continue with a different destination under the same ID. Any retry gets a new operation ID and new staging context. The collision loop must never emit a second `STARTED` from a terminal transaction.

A failure after `PUBLISHED` is not a pre-publication `FAILED`; it is a recovery/durability case. Preserve the destination and source state, and leave recovery to the later recovery milestone. Do not add a new state transition to the schema in this implementation without a separately reviewed protocol change.

## Crash and error semantics for this milestone

- Before durable `STAGED`: anonymous inode has no name; process death closes the descriptor and the inode disappears.
- After durable `STAGED`, before link: no named staging entry; recovery can safely conclude no linked stage exists.
- After link, before directory fsync: visibility/durability is uncertain; preserve the exact name and journal record; do not clean it in this milestone.
- After directory fsync, before `STARTED`: durable `STAGED` plus exact named inode; no automatic cleanup until recovery implementation is reviewed.
- After durable `STARTED`, before publication: preserve source and exact stage; no cleanup yet.
- After rename publication, before destination-dir fsync: destination visibility/durability is uncertain; preserve source and destination.
- After destination-dir fsync, before durable `PUBLISHED`: both source and destination may exist; use the recorded inode identity during later recovery, but do not delete either now.
- After durable `PUBLISHED`, before source retirement: preserve both complete copies.
- After source retirement, before durable `COMPLETED`: preserve the published destination; recovery must reconcile source absence.
- Any partial journal write or fsync error: poison context; do not append subsequent states.
- Any unexpected inode/type/owner/directory identity mismatch: stop and preserve all entries.

## Failure injection and acceptance tests

All tests use disposable temporary directories and unique payloads, including a non-root run.

### Capability
- Actual destination supports O_TMPFILE + proc-fd link + inode verification + cleanup + directory fsync.
- A different destination mount with unsupported O_TMPFILE is rejected.
- Procfs absent, masked, or not PROC_SUPER_MAGIC is rejected.
- `linkat` EPERM/EACCES/ENOENT/EOPNOTSUPP and EEXIST are handled distinctly; no AT_EMPTY_PATH or named-temp fallback.
- Probe cleanup refuses to unlink a replaced inode, symlink, or special entry.

### Durable adapter
- Empty/invalid/non-LF/oversized prepared records rejected before write.
- EINTR and short writes complete correctly.
- Failure before first byte, partial write then EIO/ENOSPC, and fsync failure poison context.
- No second record is appended after uncertain outcome.
- Two threads cannot interleave records; second process cannot acquire the journal lock.
- Journal file is regular, no-follow, owner/mode validated; creation syncs parent directory.
- FCJ2 journal bytes remain unchanged; FCJ3 bytes exist only in disposable test fixtures during this milestone.

### Staging and publication
- Anonymous inode is regular, same-device, service-owned, link count zero before STAGED.
- STAGED append+fsync occurs before proc-fd link.
- Link creates exact inode at exact component; destination directory fsync follows.
- STARTED is durable only after link and directory fsync.
- No-replace rename preserves existing destination bytes on collision.
- A collision terminates the operation; retry uses a fresh operation ID.
- Crash/fault injection at every ordering boundary preserves source and destination payloads.
- No recovery cleanup is performed in this milestone.

### Regression gates
- C17 production build; C11 and C23 compatibility builds.
- Debug ASan/UBSan and Release tests.
- Existing mover, journal, and process-crash suites.
- Static analysis, Docker, and production non-root execution.
- No C++ source/dependency/target; pure C policy remains binding.

## Explicit non-goals

- No production FCJ3 journal file or production mover integration.
- No FCJ3 recovery parser integration into `journal_recover()`.
- No quarantine or unlink of real staging entries.
- No prefix sweep or cleanup of abandoned names.
- No migration or rewrite of existing FCJ2/legacy logs.
- No change to source-retirement policy.
- No privilege/capability increase.
- No named-temp fallback.
