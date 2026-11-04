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
