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
