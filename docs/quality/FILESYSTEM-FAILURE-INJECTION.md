# Filesystem failure-injection test plan

## Safety invariant

A failed move must not silently remove or overwrite the user's original file.
Until a complete temporary copy is verified, synchronized, and published without
replacing an existing destination, the source remains untouched.

If failure occurs after publication, the destination is retained and the journal
must preserve enough state for recovery. Never delete the published destination
just to make the operation appear rolled back.

## First slice delivered by this branch

The mover test target is compiled with `FC_TESTING`; the production executable
is not. The test-only seam can deterministically inject:

- A one-shot `EINTR` from the destination write operation. The copy loop must
  retry and complete normally.
- `ENOSPC` or `EIO` after a configured number of bytes have been written. The move must
  fail, preserve the complete source, avoid publishing a partial destination,
  and remove its private temporary file.

A separate test-only hook injects `EACCES` at temporary-file creation. This
makes permission-path tests deterministic even when CI runs as root.

The seam is compiled only into the mover test target. It does not alter
production behavior or expose a runtime CLI/API.

## Second slice: post-copy mover failures

The follow-up mover tests add deterministic, test-only injection for:

- Temporary-file `fsync()` failure before publication: source remains and the incomplete private destination is removed.
- Destination publication failure: source remains and no destination is published.
- Destination-directory `fsync()` failure after publication: retain both complete source and destination copies; never attempt a destructive rollback.
- Source-retirement failure before quarantine rename: retain both complete copies.
- Source-directory `fsync()` failure after the quarantine entry is unlinked: return the durability error but retain the already-published destination. The source pathname has been retired at this point; this is a distinct state from failures before retirement and must not be described as source-preserving.

The fsync hook uses explicit call order (temporary file, destination directory, source directory) and is compiled only into the test target.

## Remaining failure cases

1. Journal append, flush, and fsync failures; recovery after each durable state.
2. Interrupted read and synchronization calls, including retry behavior.
3. Directory creation and permission failures at each path component.
4. Recovery and reconciliation for quarantined source entries left by interruption.

Each test must assert source bytes, destination bytes, private temporary files,
journal state, and return/errno behavior. Tests must not depend on host
permissions, disk exhaustion, or timing races.

## Test design rules

- Keep fault injection behind `FC_TESTING`; production builds must not contain
  mutable fault controls.
- Do not use `assert(function_with_side_effect())` for operations that must run.
  Store the return value first, then assert on the value.
- Do not treat a successful CTest run as proof of crash durability. Durability
  claims require explicit `fsync` ordering and crash/recovery tests.
- Preserve the priority order: data safety, security, correctness, maintainability,
  then performance.


## Journal append and durability failure slice

The journal test target now supports deterministic, test-only failures at three boundaries:

- **Before write:** no record bytes are appended. The record call returns an error.
- **Partial write:** only a prefix is appended and the record call returns an error. Recovery rejects the incomplete tail instead of guessing, silently accepting it, or truncating evidence.
- **After complete write, at fsync:** the record bytes may be visible, but durability is uncertain. The caller receives an error; recovery parses the actual complete record and applies the existing conservative state rules.

All mutable injection controls are compiled only when `FC_TESTING` is defined. Production builds use the normal POSIX `write()` and `fsync()` calls.

The tests verify that source and pre-existing destination files remain present across these journal failures. This slice does not attempt automatic repair of a malformed tail: preserving evidence and failing closed is safer than making an unproven assumption about which bytes reached stable storage.
