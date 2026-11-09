# FCJ3 Read-Only Recovery Report

## Purpose

`fcj3-recovery-report` is an operator-invoked diagnostic program. It reads an FCJ3 journal, validates the complete journal history, inspects filesystem evidence without following symlinks, and prints a JSON report.

It does not recover operations. It does not modify the journal or any user file.

## Build

The target is available in a build configured with `BUILD_TESTING=ON`:

```sh
cmake -S . -B build/report -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build/report --target fcj3_recovery_report --parallel 2
```

The binary is:

```text
build/report/fcj3-recovery-report
```

## Run

Supply exactly one absolute path to a disposable or operator-selected FCJ3 journal:

```sh
build/report/fcj3-recovery-report /absolute/path/to/move.fcj3
```

- JSON is written to stdout.
- Usage and failure diagnostics are written to stderr.
- A nonzero exit status means no report was produced.
- The tool refuses a journal final-component symlink and refuses symlinked directory components in journal, source, and destination paths.
- The journal must be a regular file and no larger than 64 MiB.
- Every record must be newline-terminated and valid. One malformed record rejects the complete report; no valid-prefix report is emitted.
- The transaction replay is bounded by `FCJ3_MAX_TRANSACTIONS` and parser line-size limits.

## Reading the result

Each transaction includes:
- Latest validated FCJ3 state and sequence.
- Source, destination, and staging component.
- Destination directory identity, stage and destination entry evidence, and advisory source presence.
- Errno diagnostics for source/directory/stage/destination lookup and path resolution.
- Existing classifier result and reason.

Classifications are diagnostic labels:
- `NO_ACTION_TERMINAL`: the classifier found a terminal state with non-conflicting evidence.
- `PRESERVE_INCOMPLETE`: preserve the incomplete operation and its evidence.
- `PRESERVE_PUBLISHED`: preserve the published destination and evidence.
- `MANUAL_REVIEW`: evidence is ambiguous, hostile, or unavailable.
- `INVALID_INPUT`: the classifier rejected its input.

None of these labels triggers an action.

## Snapshot limitations

The program captures journal device, inode, size, modification time, and change time before and after replay. It also verifies the number of bytes read. A detected change rejects the report.

This is a race detector, not an atomic filesystem snapshot, a lock, a cryptographic authenticity check, or proof against a same-privilege attacker able to manipulate the journal and metadata. The report states `snapshot_atomic: false`.

FCJ3 does not record source inode identity or source-parent directory identity. Source presence is therefore advisory only. It must never authorize source deletion or retirement.

Destination parent descriptors are resolved component-by-component from `/` without following symlinks and are checked against the journal's recorded device/inode identity by the evidence collector.

## Safety boundary

The report binary is separate from the production `file-cleaner` executable and is not installed. It is not used by FCJ2/legacy recovery.

It contains no recovery executor and performs no journal append, `RECOVERED` transition, rename, link, unlink, overwrite, chmod, truncate, quarantine, or cleanup. It writes JSON only to stdout.
