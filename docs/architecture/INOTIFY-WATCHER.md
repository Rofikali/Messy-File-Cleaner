# Inotify watcher behavior and failure policy

## Scope

The current watcher monitors the configured root directory only. It is not recursive. Batch scanning is recursive, but watch mode currently processes create and move-into events in the watched root; nested-directory watches are a separate feature.

## Event handling

- The inotify descriptor is nonblocking and close-on-exec.
- `poll()` waits for readiness with a bounded timeout so shutdown requests are observed without a busy loop.
- `EINTR` is retried; `EAGAIN` means there is currently no event to read.
- Each returned event record is bounds-checked before its name is read.
- Paths are dynamically allocated and checked against the application's fixed event-path limit before enqueue.
- Only regular-file create and move-into notifications are queued; directories are ignored.

## Fail-closed behavior

- Failure to initialize inotify or add the root watch is a watcher failure and produces a nonzero application exit status.
- Unexpected poll/read errors, malformed event records, root watch removal, and path overflow stop watch mode and produce a nonzero exit status.
- `IN_Q_OVERFLOW` means the kernel dropped events. The application stops watch mode with an explicit error instead of claiming all files were processed.
- On watcher failure, workers are stopped and queued work is drained. Files that were not processed remain in the filesystem for a later batch run.

## Limitations

- The watcher is root-only, not recursive.
- Automatic reconciliation after `IN_Q_OVERFLOW` is not implemented. Operator action is to rerun batch mode, inspect the root, then restart watch mode.
- Atomic rename/replace and temporary-file lifecycle handling may require additional event masks and policy in a future milestone.
- Integration tests cover startup failure, root-level file creation, queue delivery, and graceful shutdown. Kernel queue-overflow injection remains untested.

## Separate overflow research (not production)

**Linux-native next research (2026-11-28):** #309 merged at `fabf43680422765e0842eeb5e1f40afd4725e6bc`; 14/14 exact-head checks PASS, C17 Release and Ubuntu C17 92 passed/1 skipped (93 registered), five statx references PASS, optional same-device mount NOT VERIFIED; repeated median fstat 593 vs statx 625 ns/call warm single fd with validation; integrated-main 12/14 checks PASS, 2 pending at inspection; independent review unverified. STATX-MOUNT-EVAL-01 PARTIAL 5/5, retain fstat and reference, no sixth PR. Select WATCH-OVERFLOW-EVAL-01 (1/5; PR #310 open) for distinct actual kernel overflow and read-only reconciliation research; prior lifecycle and synthetic terminal tests did not establish real overflow. First candidate observes loss on one private inotify instance with bounded events, no sysctl change, no production dispatch or recovery activation. No imports, spend, release or FS-01B closure. CI pending. PR #310 same-slot correction: head b92fa09 had six optimized-build failures from GCC -Werror=maybe-uninitialized on limit; eight checks passed, C17 Debug actual overflow reference PASS (0.05 s). Initialize limit to zero, publish parsed queue limit only after successful stream cleanup, and reject a zero/out-of-cap result before generation. Keep -Werror and all CI gates. Corrected exact-head CI pending; no new PR slot.

Existing fail-closed watcher remains unchanged. The bounded private raw inotify reference observes actual overflow and missed event evidence; automatic reconciliation and real app overflow dispatch remain NOT VERIFIED. See [research plan](../upstream/LINUX_NEXT_HYPOTHESES.md).

### Second overflow experiment (reference only)

**Linux-native next research (2026-11-28):** #310 corrected and merged at `b9e2b8f27d2eeefa051668fa7e770bd4f68b9b97`; exact-head and integrated-main 14/14 checks PASS; GCC C17 Release and Ubuntu C17 93 passed/1 skipped (94 registered), real kernel overflow reference PASS (0.15/0.09 s respectively; whole-test durations); independent review unverified. WATCH-OVERFLOW-EVAL-01 (2/5; PR #311 open) adds separate bounded descriptor-relative read-only rescan after actual overflow on a quiescent private fixture, explicit incomplete state until exact snapshot validation. No production recovery, dispatch, recursive watch, source import, spend, release or FS-01B closure. CI pending.

Quiescent private fixture rescan distinguishes regular files from symlinks/directories and rejects incomplete or unexpected membership. This does not change the production fail-closed policy or establish concurrent snapshot consistency. See [research plan](../upstream/LINUX_NEXT_HYPOTHESES.md).

### Third overflow experiment (reference only)

**Linux-native next research (2026-11-28):** #311 merged at `e9c1d56b9d09d161e6ca6ff43ff7392cf3f64fb3`; exact-head 14/14 checks PASS, GCC C17 Release and Ubuntu C17 94 passed/1 skipped (95 registered), real overflow and quiescent rescan PASS (not skipped); integrated-main 9/14 checks PASS, 5 pending at inspection; independent review unverified. WATCH-OVERFLOW-EVAL-01 (3/5; PR #312 open) adds deterministic name replacement between directory enumeration and metadata validation after actual overflow, requiring explicit stale identity rejection with positive and preservation controls. No production recovery, dispatch, concurrent atomicity claim, source import, spend, release or FS-01B closure. CI pending.

A deliberately inserted private-fixture rename/replacement at the enumeration-to-metadata boundary must return explicit stale identity, preserving the original and replacement. This is deterministic interposition, not general concurrent recovery. Production fail-closed policy remains unchanged. See [research plan](../upstream/LINUX_NEXT_HYPOTHESES.md).

### Fourth overflow experiment (reference only)

**Linux-native next research (2026-11-28):** #312 merged at `23edab1ac888cae370d2f43c38c03a5aef7e0551`; exact-head 14/14 checks PASS, GCC C17 Release and Ubuntu C17 95 passed/1 skipped (96 registered), all three Inotify references PASS (not skipped); integrated-main 10/14 checks PASS, 4 pending at inspection; independent review unverified. WATCH-OVERFLOW-EVAL-01 (4/5; PR #313 open) adds bounded fresh inotify instance restart after real overflow, discards pending old-instance events, validates quiescent snapshot and accepts only new-instance notifications. No production recovery, event replay, arbitrary concurrent consistency, imports, spend, release or FS-01B closure. CI pending.

Old instance with pending events is closed, a fresh instance starts empty, the quiescent snapshot is revalidated, and exactly one fresh notification is accepted. Numeric descriptor reuse is allowed; no proof of concurrent gap coverage or application work-queue generation tracking. Production fail-closed policy remains unchanged. See [research plan](../upstream/LINUX_NEXT_HYPOTHESES.md).

### Final overflow experiment and decision (reference only)

**Linux-native next research (2026-11-28):** #313 merged at `c3d0d9513a3dff49ecb03d26e6c14e44f3d81d8e`; exact-head 14/14 checks PASS, GCC C17 Release and Ubuntu C17 96 passed/1 skipped (97 registered), all four Inotify references PASS (not skipped); integrated-main 12/14 checks PASS, 2 pending at inspection; independent review unverified. WATCH-OVERFLOW-EVAL-01 (5/5; PR #314 open) adds deterministic close/register gap creation after actual overflow and requires rejection of old snapshot despite an empty new queue. Adoption decision: retain production fail-closed watcher; defer automatic recovery. No sixth PR, production recovery, dispatch, source import, spend, release or FS-01B closure. CI pending.

Deterministically create an owned file between closing the old instance and registering the new one. Require an empty new queue but reject the old snapshot with explicit unexpected membership; verify file identities and preserve incomplete state. Retain production fail-closed behavior and manual batch/restart guidance. Concurrent recovery, application work-queue generation and production adoption remain deferred. See [research plan](../upstream/LINUX_NEXT_HYPOTHESES.md).
