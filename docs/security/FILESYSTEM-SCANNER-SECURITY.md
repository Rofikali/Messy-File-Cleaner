# Filesystem security: scanner boundary

## Implemented scanner guarantees

The scanner now uses descriptor-relative traversal rather than building a path and calling stat() on it:

- Opens the requested root as a directory and rejects a symlink at the root path.
- Uses fstatat(..., AT_SYMLINK_NOFOLLOW) to inspect entries without following symlinks.
- Uses openat(..., O_DIRECTORY | O_NOFOLLOW) when descending into child directories.
- Visits regular files only. Symlinks, sockets, FIFOs, devices, and other special files are skipped.
- Builds event paths with checked dynamic allocation rather than a fixed 1024-byte buffer.
- Uses an explicit directory stack instead of recursive C calls.
- Returns a failure status when traversal cannot safely complete; batch mode reports that failure.

## Test coverage

The scanner integration test creates disposable temporary directories and verifies:

- A regular file is reported.
- A deeply nested file whose path exceeds the old 1024-byte limit is reported.
- A symlink to an outside file is not reported.
- A symlink to an outside directory is not traversed.
- A symlink used as the scan root is rejected.

The test fixtures are created only under /tmp in the CI runner.

## Important remaining limitation

A scanner event is still a pathname string. The file can be replaced after scanning but before a worker opens it. Therefore scanner no-follow checks alone do not eliminate the scanner-to-mover TOCTOU race.

The mover must independently open the source with no-follow semantics, verify the opened file with fstat, and use descriptor-relative destination operations. That work is a separate filesystem-mover milestone. Do not describe the whole application as fully symlink-race-safe until that milestone is complete.

The root's final path component is protected against symlink following. Canonical root policy and ancestor-component validation are also separate work; the current exact-string dangerous-root check is not a complete root authorization boundary.
