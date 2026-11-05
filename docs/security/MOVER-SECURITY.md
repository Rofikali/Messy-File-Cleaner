# Mover security and publication contract

## Current behavior

The mover uses a conservative copy-then-retire algorithm for all moves, including same-filesystem moves. This is intentionally safer but slower than a same-filesystem rename fast path; performance optimization is deferred until the safety model and benchmarks are established.

1. Split the source into a parent directory and basename.
2. Open the source parent and destination directory with directory/no-follow flags.
3. Open the source entry with O_NOFOLLOW and verify with fstat that it is a regular file.
4. Copy from the already-open file descriptor into an O_CREAT | O_EXCL temporary file in the destination directory.
5. Compare source inode, size, modification time, and change time after copying; reject a file that changed while being copied.
6. Apply ordinary permission bits, fsync the temporary file, and close it successfully.
7. Publish with Linux renameat2(RENAME_NOREPLACE), choosing a suffixed name if a destination already exists.
8. fsync the destination directory before attempting source retirement.
9. Rename the source entry to a unique quarantine name, verify that quarantine still refers to the inode originally opened, and only then unlink it. If identity differs, attempt a no-replace restore and report failure.
10. fsync the source directory and record journal completion.

## Safety properties

- Existing destination files are never intentionally overwritten.
- Symlink sources, symlink destination directories, and special-file sources are rejected.
- Failed copies remove only the temporary file created by this operation; the source remains.
- If source identity changed during processing, the mover does not intentionally unlink the replacement.
- If source retirement or final journaling fails after publication, the destination is retained. A duplicate may remain; data loss is not preferred over a duplicate.
- The standalone copy helper uses exclusive creation rather than O_TRUNC.

## Platform and threat-model boundaries

- The no-replace publication primitive uses Linux renameat2(RENAME_NOREPLACE). Unsupported kernels fail closed rather than falling back to an overwriting rename.
- Source-parent and destination paths are traversed one component at a time from an open root/current-directory descriptor. Each component is opened with `openat()` plus `O_DIRECTORY | O_NOFOLLOW`.
- Missing destination components are created with `mkdirat()` relative to the already-open parent descriptor, then reopened with no-follow flags.
- `.` and `..` path components are rejected by the directory traversal helper; callers should provide normalized paths. A plain `.` path and `/` root are valid directory roots.
- A hostile process with write access to the source directory can race directory entries. The quarantine-and-identity check reduces the destructive race window and preserves mismatched entries, but a complete adversarial multi-process filesystem policy needs explicit directory ownership/permission assumptions and dedicated stress/fault-injection tests.
- Journal parent traversal uses directory descriptors and no-follow opens for each traversed component; missing components are created with `mkdirat()` and reopened descriptor-relatively. Symlinked ancestors are rejected.
- Journal paths containing `.` or `..` components are rejected. This is a conservative path policy; callers should provide normalized paths.
- Descriptor-relative traversal prevents ancestor substitution through symlinks, but does not defend against a hostile process that can rename or replace entries inside directories it controls. Directory ownership/permissions remain part of the threat model.
- The journal stores versioned hex-encoded paths with bounded parsing and fail-closed recovery; source/destination recovery checks remain pathname-based and are a separate hardening area.

## Performance decision

Same-filesystem rename is normally cheaper than copying, but this milestone deliberately uses one copy/publish/retire path to simplify correctness across filesystems. Do not add a rename fast path until tests prove that it preserves the same no-overwrite, identity, crash, and recovery guarantees. Benchmark metadata-only rename separately from data copy, fsync, and journal durability.
