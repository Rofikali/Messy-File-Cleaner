# Process crash and restart recovery tests

## Purpose

The existing fault-injection tests simulate failed system calls while the process
continues running. They do not prove behavior when the operating system kills
the process and closes its descriptors without running cleanup code.

This suite adds real child-process termination at durable move boundaries. The
parent waits until the child reaches a test-only checkpoint, sends SIGKILL,
waits for process death (and therefore kernel descriptor/lock release), then
runs journal recovery from a fresh process context.

## Crash matrix

| Checkpoint | Durable state at interruption | Expected files after recovery | Recovery behavior |
| --- | --- | --- | --- |
| After STARTED | STARTED record synced; destination not published | Source only | Record RECOVERED; preserve source |
| After PUBLISHED | Destination published and destination directory synced; PUBLISHED record synced | Source and destination | Record RECOVERED with EALREADY; preserve both copies |
| After source retirement | Source quarantine/unlink and source-directory sync completed; PUBLISHED remains latest journal state | Complete destination only | Record RECOVERED; preserve destination |

Each scenario runs recovery twice. The second pass must not append another
RECOVERED record or change the journal size. The tests also verify payload
contents, not merely path existence.

## Known limitation exposed by the STARTED crash

The mover creates its private `.fc-tmp-<pid>-<sequence>` file before appending
STARTED. If the process is killed after STARTED but before publication, that
private temporary file remains in the destination directory. The current
journal does not persist a temporary-file identity, so recovery deliberately
does not guess which unreferenced temporary entries it may delete.

The integration test verifies this exact orphan and removes it only as part of
cleaning its disposable test fixture. A future cleanup design must establish
ownership safely (for example, by journaling a verifiable temporary identity)
before production recovery is allowed to remove such files. Do not add a broad
prefix-based cleanup sweep.

## Test mechanics and safety

- Checkpoints and SIGSTOP behavior exist only in `FC_TESTING` builds.
- The parent sends SIGKILL only to its own child test process.
- Every test uses a private `/tmp/fc-process-crash-XXXXXX` fixture.
- The parent verifies SIGKILL termination, journal lock reacquisition, source
  and destination bytes, and repeat-recovery idempotency.
- The test has a hard timeout to prevent a broken checkpoint from hanging CI.
- No production CLI option, signal handler, or crash-control API is added.

## Scope and limitations

SIGKILL tests process termination and kernel cleanup of descriptors/locks. It
is not equivalent to sudden power loss, kernel panic, storage-controller cache
loss, or a filesystem/hardware durability certification. The results apply to
the CI filesystem and runtime used for the test. Continue to describe fsync
guarantees with filesystem and platform assumptions.

## Validation

Use the repository developer interface:

```sh
make test
make validate
```

The CTest suite includes `process_crash_recovery`; `make validate` repeats it
under C11, C17, and C23 Debug builds.
