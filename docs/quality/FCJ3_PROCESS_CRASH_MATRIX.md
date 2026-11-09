# FCJ3 Process-Crash Matrix (Test-Only)

## Purpose

The FCJ3 parser, operation context, durable append adapter, and staging lifecycle
now have independent tests. This milestone composes those modules in a child
process and terminates that process with SIGKILL at durable transaction
boundaries. The parent then reopens and replays the disposable journal and
classifies the exact filesystem state by recorded device/inode identity.

This is a crash-observation and recovery-readiness test, not the production
recovery dispatcher. It deliberately does not append RECOVERED, unlink a
source, remove a staging entry, quarantine anything, or repair a journal.

## Crash matrix

| Child termination point | Last durable journal state | Expected source | Expected stage name | Expected destination |
| --- | --- | --- | --- | --- |
| After STAGED, before link | STAGED | Present | Absent (anonymous inode dies with process) | Absent |
| After link and destination-directory sync | STAGED | Present | Present, exact recorded inode | Absent |
| After durable STARTED | STARTED | Present | Present, exact recorded inode | Absent |
| After publish and destination-directory sync, before PUBLISHED | STARTED | Present | Absent | Present, exact recorded inode |
| After durable PUBLISHED | PUBLISHED | Present | Absent | Present, exact recorded inode |

The test child uses only Fcj3Stage and Fcj3Journal against a unique
/tmp/fcj3-crash-matrix-* fixture. It writes fixture.fcj3; it never opens or
writes journal/move.log or a production move.fcj3.

## Restart-side assertions

After the parent confirms that its own child died from SIGKILL, it:

1. Reopens the disposable FCJ3 journal from a fresh process context.
2. Replays all complete records with the read-only Fcj3Replay parser.
3. Checks the expected operation state and sequence.
4. Checks source payload bytes remain unchanged.
5. Checks any visible stage/destination entry is a regular file with the exact
   st_dev and st_ino recorded in the journal.
6. Replays a second time and confirms the journal size and parsed state do not
   change.

The parent only removes exact fixture names during final test teardown. This is
not a recovery action and must not be generalized into production cleanup.

## Safety and interpretation

- All FCJ3 components remain linked only into dedicated test executables.
- No FCJ3 recovery dispatcher or production mover integration is introduced.
- No source retirement, quarantine, RECOVERED transition, prefix sweep, or
  automatic cleanup is performed.
- If an entry is present but does not match the recorded inode, the test fails;
  it never treats a name alone as proof of ownership.
- SIGKILL tests process death and descriptor release. It does not simulate
  power loss, kernel panic, storage-controller cache loss, or certify hardware
  durability.
- A crash between a filesystem operation and its following fsync is not
  directly forced by these checkpoints. Existing deterministic fsync failure
  tests cover error returns; later work must add narrowly scoped crash/fault
  boundaries if required.
- The matrix stops at PUBLISHED. Source retirement is intentionally excluded
  until a separate recovery design proves exact source-entry revalidation and
  durable state transitions.

## Validation

Run:

    make test
    make validate

The CMake target is fcj3_process_crash. It is built with FC_TESTING and
uses the C11/C17/C23 compatibility matrix through the repository validation
workflow.
