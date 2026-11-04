# Scanner callback, cancellation, and path contract

## Callback contract

`scan_directory()` calls an `event_handler_fn` for each regular file. The callback returns 0 to accept the event or -1 to stop traversal. On failure it sets `errno`; if it does not, the scanner reports `EIO`.

## Cleanup contract

If traversal or the callback fails, the scanner closes every open directory stream and frees every allocated path/frame before returning. The first meaningful error is preserved as `errno`.

## Path handling

- Paths are assembled dynamically with checked size arithmetic; the scanner does not assume a fixed `PATH_MAX` buffer.
- Symlinks are not followed. Directory entries are inspected with `fstatat(..., AT_SYMLINK_NOFOLLOW)` and child directories are opened with `openat(..., O_NOFOLLOW)`.
- The application event contract currently stores paths in a fixed-size `FileEvent.path`; the enqueue callback rejects paths that do not fit with `ENAMETOOLONG`. It never truncates a path and then moves a different file.

## Cancellation

A stop request prevents further enqueue operations. The callback returns `ECANCELED`, the scanner stops traversal, and the application exits nonzero rather than reporting a complete scan. Files not queued remain in place for a later run.

## Limitations

The scanner treats `ENOENT` from an entry disappearing between `readdir` and `fstatat` as a normal concurrent filesystem race. Permission, I/O, allocation, and other traversal failures are reported. A scan is not a filesystem snapshot; files can change while traversal is in progress.
