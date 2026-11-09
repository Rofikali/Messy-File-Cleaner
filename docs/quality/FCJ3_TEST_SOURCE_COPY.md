# FCJ3 test-only source-copy consistency

## Scope

This milestone adds a test-only source-to-anonymous-stage copy primitive to
`Fcj3Stage`. It does not add a production mover, source retirement, recovery,
quarantine, cleanup, or production FCJ3 journal writes.

The caller supplies an already-open source descriptor. The intended caller
contract is to open the source with `O_RDONLY | O_CLOEXEC | O_NOFOLLOW` using
the project's existing descriptor-relative path validation. The staging module
does not resolve a source pathname and never unlinks or modifies the source.

## Copy algorithm

1. Validate the source descriptor refers to a regular file and is readable.
2. Capture source device, inode, mode, owner, group, size, mtime, and ctime.
3. Verify the anonymous staging inode is still empty, unlinked, and has the
   expected device/inode identity.
4. Copy the original size using bounded 64 KiB `pread()` / `pwrite()`
   loops. Both operations handle `EINTR` and short transfers.
5. Treat premature EOF, read/write errors, or any partial-copy uncertainty as
   terminal for this staging object: mark it poisoned and reject sync, link,
   and publication.
6. Re-stat the source and staging descriptors. The source snapshot must remain
   identical across the copy, and the staging inode must contain exactly the
   original byte count with the expected owner, mode, device, inode, and zero
   link count.
7. Return the captured source snapshot and byte count only after all checks
   succeed. The caller can then fsync the staging inode and continue through
   the existing test-only link/publication lifecycle.

The copy uses explicit offsets, so it does not depend on or change the source
descriptor's shared file offset. The helper rejects using the staging inode as
the source.

## Failure policy

- No retry from a partially written stage.
- No truncate-and-reuse repair.
- A file or directory `fsync()` failure poisons the operation stage. It cannot be retried into publication.
- No linking or publication after a poisoned copy.
- The source descriptor and source pathname are never changed by the copy
  implementation.
- Closing a still-anonymous poisoned stage releases its anonymous inode; there
  is no named staging entry to sweep.
- A stage that has already become visible is never automatically unlinked on
  identity or durability uncertainty.

The `FC_TESTING` hooks provide deterministic partial-`pwrite` failure, a
source-size mutation before the post-copy metadata check, retained staging-descriptor mutation before sync and after link, and
file/directory `fsync()` failures. They are test-only and are not part of a
production interface. The tests assert that an fsync failure before linking
leaves no named entry, a directory-fsync failure after linking preserves the
visible staging entry, and a publication-fsync failure preserves the visible
destination without attempting cleanup.

## Important consistency limit

Matching metadata before and after copying detects ordinary concurrent
mutation during the copy, but it is not an atomic filesystem snapshot. The
source can change after the final metadata check. A future source-retirement
milestone must therefore revalidate the still-open source descriptor and its
directory entry against the saved identity and metadata immediately before
unlinking the source. If that revalidation fails, preserve both source and
published destination and record an uncertain/failed outcome for recovery.

This helper copies file bytes only. It does not preserve ownership, timestamps,
ACLs, extended attributes, sparse extents, hard-link relationships, or other
filesystem metadata. Those requirements need an explicit product policy and
separate tests before a production mover can use this path.

## Production gate

All FCJ3 code remains isolated to test targets. Do not write FCJ3 records to
`journal/move.log`, do not enable FCJ3 in the production executable, and do
not add source retirement until the recovery dispatcher and crash-recovery
acceptance matrix are implemented and reviewed.
