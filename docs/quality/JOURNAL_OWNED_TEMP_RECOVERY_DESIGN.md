# Journal-owned temporary file recovery design

## Status

Design proposal only. This document does not authorize production deletion of
temporary files. Implementation must follow in a separate, tested change after
this design is reviewed.

## Problem statement

A process killed after a durable `STARTED` record but before destination
publication can leave a private `.fc-tmp-*` entry in the destination directory.
The journal currently stores only source and destination paths. Recovery cannot
prove that an unreferenced temporary entry belongs to a particular move, so it
must not remove it by filename prefix.

The required outcome is to recover only a temporary file whose ownership is
proven by durable journal metadata and filesystem identity checks. Unknown,
ambiguous, replaced, symlinked, or mismatched entries must be preserved.

## Preferred design: anonymous staging, then durable identity, then link

A named temporary file cannot be made atomically consistent with a separate
journal file. There is always a crash window between creating the name and
persisting its identity. A design that merely adds an inode to a later journal
record still leaves that window.

On the supported Linux filesystems, use an anonymous `O_TMPFILE` staging
inode to remove that window:

1. Open and validate the destination directory using the existing
   descriptor-relative, no-symlink directory traversal rules.
2. Record the destination directory identity (`st_dev`, `st_ino`) from its
   open descriptor.
3. Create the staging file with `openat(dirfd, ".", O_TMPFILE | O_RDWR |
   O_CLOEXEC, 0600)`. Do not create a visible `.fc-tmp-*` name yet.
4. Copy the source, verify the source did not change, apply its permitted mode,
   fsync the anonymous staging file, and obtain its `st_dev` and `st_ino`
   with `fstat()`.
5. Append and fsync a new `STAGED` journal record containing:
   - source and final destination paths;
   - destination directory identity (`st_dev`, `st_ino`);
   - a generated, single-component staging name (not a path);
   - staging inode identity (`st_dev`, `st_ino`).
6. Link the already-open anonymous inode into the destination directory with
   `linkat(temp_fd, "", dirfd, temp_name, AT_EMPTY_PATH)`. Then fsync the
   destination directory.
7. Append and fsync `STARTED`, then continue with no-replace publication,
   destination-directory sync, `PUBLISHED`, source retirement, and
   `COMPLETED` as today.

The ordering matters. Before `STAGED` is durable, the staging inode has no
directory entry and disappears automatically if the process dies. After
`STAGED` is durable, the journal already contains the expected directory and
inode identities before the staging name can become visible.

### Filesystem compatibility and capability checks

`O_TMPFILE` and `AT_EMPTY_PATH` are Linux-specific and filesystem support
varies. Do not silently fall back to visible named-temp creation and then claim
the same crash-cleanup guarantee. If the destination filesystem does not
support the required operations, fail safely with a clear, actionable error
(or use a separately designed fallback whose weaker recovery guarantee is
explicitly documented and never performs automatic cleanup of unverified
entries).

**Capability is destination-specific.** A successful probe under `/tmp` does
not prove that the configured destination supports `O_TMPFILE`, `fsync`, or
`linkat(AT_EMPTY_PATH)`. The runtime capability check must use the already
validated, open descriptor for the actual destination directory, under the
same effective UID, mount namespace, and security policy as the real move.
The probe must use a unique disposable name, verify the linked inode identity,
unlink only its own probe entry, and fsync the destination directory. A
capability result must not be cached across different destination filesystems
or mount points.

The project already uses Linux-specific `renameat2(RENAME_NOREPLACE)`; still,
the supported filesystem matrix must be tested rather than assumed. The
capability probe in the test suite is a test of its own temporary filesystem,
not proof that every user destination is compatible.

### Link operation permissions

`linkat(..., AT_EMPTY_PATH)` can be restricted by kernel permissions,
capabilities, filesystem behavior, or security policy. Treat every failure as
a real capability failure. Do not silently switch to a weaker linking method
or named temporary-file fallback. Validate behavior in the actual non-root
production runtime, not only as root in CI.

## Journal format and compatibility

- Introduce a versioned record format (for example, `FCJ3`) for records that
  carry staging metadata.
- Preserve strict parsing of existing `FCJ2` and legacy records. Old records
  have no staging identity and must never trigger temporary-file deletion.
- Model `STAGED` as a durable state before `STARTED`. Recovery must reject
  impossible transitions, duplicate/conflicting staging identities, invalid
  component names, out-of-range numeric fields, and malformed/truncated tails.
- Subsequent state records must preserve the staging identity for that move,
  or the parser must retain it from the preceding `STAGED` record while
  validating the transition sequence.
- Do not break existing source/destination path encoding or reinterpret old
  records.

## Recovery algorithm

For an incomplete operation with durable staging metadata:

1. Open the recorded destination parent directory through descriptor-relative
   traversal with symlinks and dot traversal rejected.
2. Compare its current `st_dev` and `st_ino` with the recorded directory
   identity. If they differ, stop recovery for that record and preserve every
   file.
3. Inspect only the exact recorded staging component with `fstatat(...,
   AT_SYMLINK_NOFOLLOW)`. Never enumerate the directory and never match a
   prefix or glob.
4. If the entry is absent, continue the existing source/destination state
   recovery rules.
5. If present, require a regular file and exact match of recorded `st_dev` and
   `st_ino`. A symlink, special file, inode mismatch, or unexpected type is
   an ambiguity: preserve it and fail closed.
6. Move the exact component into a private quarantine directory using
   descriptor-relative, no-replace rename semantics. The quarantine directory
   must be opened by descriptor, owned by the effective service UID, a real
   directory, and have mode `0700` (no group/other permissions). Verify its
   device/inode identity after opening. Verify the moved entry's type, device,
   and inode from inside that private directory before unlinking it.
7. If the moved entry does not match, do not unlink it. Attempt a no-replace
   restore to the original component; if restore is blocked, leave the entry
   in quarantine and report the ambiguity. Never delete a mismatched entry.
8. Sync the quarantine and destination directories after successful cleanup,
   then append and fsync the appropriate recovery record. Recovery must remain
   idempotent.

The quarantine directory must not be created or accepted inside an attacker-
controlled location without the ownership/mode checks above. A process running
with the same UID as the cleaner can generally modify that UID's files and is
outside this filesystem permission boundary; the product threat model must say
so explicitly. If the quarantine directory cannot be made private to the
cleaner UID, automatic cleanup must be disabled.

A broad `.fc-tmp-*` or `.fc-retire-*` sweep is prohibited. Descriptor-
relative checks alone do not make unlink-by-name race-free. The private
quarantine directory is the required access-control boundary that prevents a
different untrusted UID from swapping the quarantined name between verification
and unlink.

## Required tests before enabling cleanup

All tests use disposable directories and unique payloads. No test may touch
user files.

### Journal and parser

- FCJ2 and legacy records still recover exactly as before and never authorize
  temp cleanup.
- FCJ3 STAGED records round-trip maximum valid path/component lengths and
  device/inode values.
- Truncated, malformed, overflowed, duplicated, reordered, and conflicting
  STAGED metadata fails closed without truncating or rewriting the journal.
- Inject failures before, during, and after STAGED append/fsync and link.

### Crash boundaries

Kill a child process at each durable boundary:

- before STAGED is durable: no visible staging entry remains;
- after STAGED but before link: no named entry; recovery is idempotent;
- after link but before STARTED: exact recorded staging entry is safely removed;
- after STARTED: exact recorded staging entry is safely removed and source is
  preserved;
- after publication / PUBLISHED: never remove the published destination;
- after source retirement / before COMPLETED: preserve the complete destination.

Repeat recovery and verify journal size does not grow after the first
successful recovery.

### Adversarial filesystem cases

- A different file at the recorded name (inode mismatch) is preserved.
- A symlink at the recorded name is preserved and never followed.
- A directory, FIFO, socket, or device at the recorded name is preserved.
- A replaced destination directory (device/inode mismatch) is not traversed for
  cleanup.
- A quarantine-name collision never overwrites an existing entry.
- A rename race or failed restore is reported without deleting an unverified
  entry.
- A file with a similar `.fc-tmp-*` prefix but no matching journal identity is
  never touched.
- Unsupported `O_TMPFILE` / `AT_EMPTY_PATH` filesystems fail safely; no
  silent weaker fallback is enabled.
- The actual destination directory is probed; a successful `/tmp` probe alone
  is insufficient.
- Quarantine directory symlink, wrong owner, permissive mode, replacement, and
  same-UID threat-model cases are covered.
- Run the probe and move tests as the production non-root UID.

### Regression and quality gates

- C17 production build; independent C11 and C23 compatibility builds.
- Debug ASan/UBSan and Release test suites.
- Existing mover, journal, and process-crash tests.
- Static analysis and Docker/non-root smoke tests.
- No C++ source, dependency, or build target; pure C policy remains binding.

## Acceptance criteria

This design is ready for implementation only when reviewers agree on:

1. The `O_TMPFILE` support policy and supported filesystem matrix.
2. The exact FCJ3 schema and valid state-transition table.
3. The threat model and the limits of safe unlinking in writable directories.
4. Backward compatibility for FCJ2 and legacy journal data.
5. The full crash and adversarial test matrix above.

Production recovery must not delete any temporary entry until all acceptance
criteria are implemented and their tests pass.
