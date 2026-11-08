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

## Next failure cases

Add deterministic injection and assertions for:

1. `EACCES` while opening/creating destination components.
2. `EIO` during copy and `ENOSPC` during metadata synchronization.
3. Interrupted read, write, and synchronization calls.
4. Failure while publishing the destination.
5. Failure while retiring the source and while syncing source/destination
   directory entries.
6. Journal append/flush failures and recovery after each durable state.

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
