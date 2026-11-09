# FCJ3 Test-Only Staging Lifecycle

## Status

Implementation candidate. This module is linked only into a disposable integration
test. It is not part of the production executable and does not modify the
existing FCJ2/legacy mover or journal.

## Implemented boundary

- Open an anonymous O_TMPFILE inode relative to an already-open destination
  directory, without O_EXCL.
- Verify the directory identity, procfs mount, inode type, owner, link count,
  and same-device requirement.
- Let the test caller populate the anonymous descriptor, then fsync and capture
  its exact inode metadata.
- Append and fsync STAGED before linking.
- Link only through the verified /proc/self/fd/<fd> + AT_SYMLINK_FOLLOW method.
- Verify the exact staging component by device/inode and fsync the destination
  directory before STARTED.
- Append and fsync STARTED before publication.
- Publish with renameat2(RENAME_NOREPLACE); never overwrite a collision.
- Verify the final component is the exact staging inode and fsync the directory
  before PUBLISHED.

## Deliberate limits

This first lifecycle slice stops at PUBLISHED. It does not retire or unlink the
source, append COMPLETED, recover a transaction, quarantine an entry, or clean
up an orphan. The source remains untouched in every test. If publication or a
durability boundary is uncertain, the test preserves visible entries and
reports failure. Collision is surfaced as EEXIST so the caller can append
terminal FAILED to the test journal; a retry must use a new operation context.

FCJ3 bytes exist only in per-test disposable journals. No writes are made to
journal/move.log or any production FCJ3 journal. Production FCJ3 remains
disabled until the recovery dispatcher and adversarial recovery tests pass.

## Security and ownership

- The destination directory fd is borrowed and must remain open.
- The stage object owns its O_TMPFILE and procfs descriptors.
- The stage object must not be copied.
- Closing the object never unlinks a visible staging or final entry.
- No AT_EMPTY_PATH, capability grant, named-temp fallback, prefix sweep, or
  implicit cleanup is allowed.
- Unsupported O_TMPFILE/procfs/renameat2 environments fail closed.
