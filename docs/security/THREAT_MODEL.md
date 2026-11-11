# Security Threat Model

## 1. Status and scope

This is the baseline threat model for Messy File Cleaner, a local Linux/POSIX file-management application written in pure C. It describes required security properties and residual risks. It does not certify the implementation as secure or production-ready.

The application reads directory trees and may create, publish, rename, and remove filesystem entries. A bug can destroy user data; confidentiality, integrity, and availability all matter, with integrity/data preservation the highest priority.

## 2. Assets to protect

1. User source-file contents and metadata.
2. Existing destination-file contents.
3. Journal records and transaction identity.
4. Directory and file ownership/permissions.
5. Configuration and classification rules.
6. Process memory, file descriptors, worker state, and resource limits.
7. User trust: truthful dry-run, result, and recovery reporting.

## 3. Security objectives

- Never overwrite an existing destination.
- Never delete or retire a source before a complete destination is safely published and the protocol permits retirement.
- Never follow an untrusted symlink as if it were a trusted file or directory.
- Never infer temporary-file ownership from a naming prefix.
- Never let malformed or ambiguous journal data trigger destructive cleanup.
- Preserve unknown, mismatched, replaced, or special filesystem entries.
- Bound memory, workers, queues, descriptors, input lengths, and parsing work.
- Run with least privilege and avoid root.
- Fail closed when identity, ownership, capability, or durability cannot be established.
- Report uncertainty honestly; do not manufacture success.

## 4. Trust boundaries

| Boundary | Untrusted input / risk | Required treatment |
|---|---|---|
| CLI/config -> application | Paths, options, extension rules, numeric values | Strict validation, bounded parsing, checked conversions |
| Filesystem -> scanner | Names, symlinks, special files, concurrent mutation | Use no-follow and descriptor-relative operations where appropriate |
| Scanner -> queue | Discovered paths and work metadata | Bounded items, explicit ownership, cancellation contract |
| Worker -> mover | Potentially stale path assumptions | Revalidate identity/type at mutation boundary |
| Mover -> filesystem | Names may be swapped; syscalls can fail | Exclusive create, no-replace publication, descriptor-relative operations |
| Journal -> recovery | Truncated, malformed, duplicated, conflicting records | Versioned parser, strict validation, fail closed |
| Container -> host mounts | Mount contents and permissions | Non-root, minimal mounts/capabilities, no Docker socket |
| Test hooks -> production | Fault controls could weaken behavior | Compile-time test-only isolation |
| FCJ4 test adapter -> temporary journal | Descriptor substitution, partial writes, uncertain fsync, concurrent writers | Linux-only test target; private disposable fixture; descriptor identity/size/owner/mode/link checks; explicit-offset pwrite; O_APPEND rejection; advisory lock; full replay after uncertainty |

## 5. Attacker model

Assume an attacker or concurrent process may:
- Control filenames and parts of the directory tree being scanned.
- Create symlinks, hard links, FIFOs, sockets, devices where permissions permit.
- Replace or rename directory entries between checks and use.
- Create destination-name collisions.
- Supply malformed or oversized configuration and persistent journal data.
- Trigger resource exhaustion through huge trees, deep paths, many files, or repeated events.
- Cause ordinary syscall failures, process termination, disk-full conditions, and power-loss-like durability uncertainty.
- Operate under a different unprivileged UID when directory permissions allow.

Do not claim protection against a fully privileged kernel/root attacker. A same-UID attacker can often interfere with files and process-owned resources that are writable by that UID; this boundary must be explicit.

## 6. Primary threats and required controls

### 6.1 Path traversal and unsafe destination selection
Threat: user-controlled names or configuration escape the intended output/journal root.

Controls:
- Validate configured output folder names as permitted single components.
- Reject absolute paths, dot-dot, nested separators, reserved journal names, and source/destination overlap.
- Anchor operations to validated directory descriptors.
- Do not rely on string prefix checks as a containment proof.

### 6.2 Symlink and special-file attacks
Threat: a path resolves to a symlink or special file instead of the intended regular file/directory.

Controls:
- Reject symlink roots and output directories where required by policy.
- Use `O_NOFOLLOW`, `AT_SYMLINK_NOFOLLOW`, `fstatat`, and descriptor-relative APIs where applicable.
- Verify file type and identity before using an entry.
- Never follow an untrusted entry during cleanup.

### 6.3 TOCTOU and directory-entry replacement
Threat: an attacker swaps a name after validation but before mutation.

Controls:
- Prefer open directory descriptors and relative operations.
- Retain descriptors to validated objects where possible.
- Revalidate device/inode/type at the point of use.
- Use exclusive creation and no-replace rename/publication.
- Document residual races; descriptor-relative APIs reduce but do not automatically eliminate all name-swap races.

### 6.4 Destination collision and overwrite
Threat: existing user data is truncated or replaced.

Controls:
- Never open a destination with truncation as a fallback.
- Use exclusive staging and no-replace publication.
- Treat collisions as explicit errors.
- Tests compare pre-existing destination bytes before and after every failure case.

### 6.5 Partial copy, I/O failure, and source loss
Threat: a failed read/write/fsync/publish/retire sequence leaves partial output or removes the only complete copy.

Controls:
- Handle EINTR and short I/O correctly.
- Verify copied content according to the mover contract.
- Sync file and relevant directories at documented durability boundaries.
- Retain source until destination is safely published.
- Preserve complete copies when durability becomes uncertain; report uncertainty rather than destructive rollback.

### 6.6 Journal corruption or ambiguous recovery
Threat: malformed, partial, duplicated, reordered, or conflicting records cause recovery to guess and delete user data.

Controls:
- Strict versioned parsing with bounded line/field sizes and checked numeric conversions.
- Validate transaction identity and legal transition order.
- Do not silently truncate a malformed tail.
- Do not treat a missing transition as success.
- Recovery must be idempotent and non-destructive when evidence is insufficient.
- Preserve FCJ2/legacy behavior; old records cannot authorize cleanup that requires metadata they do not contain.

### 6.7 Temporary-file ownership confusion
Threat: recovery deletes an unrelated file because its name resembles an application temp prefix.

Controls:
- Never sweep `.fc-tmp-*` or `.fc-retire-*` by prefix.
- A future FCJ3 cleanup operation must be tied to a unique transaction ID plus immutable staging metadata, including the expected parent directory and file identity.
- Verify live device/inode/type and ownership against the journal before any destructive action.
- If any identity differs, preserve the entry and report an unresolved condition.

### 6.8 Quarantine and unlink race
Threat: an attacker swaps a directory entry between identity check and unlink.

Controls:
- A future destructive cleanup design requires a private quarantine directory opened by descriptor, owned by the service UID, and mode 0700.
- Validate the quarantine directory identity and permissions.
- Move the exact candidate into quarantine using a no-replace operation, then verify identity again before unlinking.
- If a private quarantine boundary cannot be established, disable cleanup.
- This does not protect against a fully capable same-UID attacker; state that limitation.
- Never claim that a prior `fstatat` alone makes a later unlink race-free.

### 6.9 O_TMPFILE capability and unprivileged linking
Threat: implementation assumes a capability supported on one filesystem is supported on another, or silently falls back to a weaker named-temp strategy.

Controls:
- Probe the actual validated destination directory under the production UID, mount namespace, and security context.
- A successful `/tmp` probe does not establish destination support.
- Treat `O_TMPFILE` and the selected proc-fd linking method as filesystem/runtime-dependent.
- Do not grant `CAP_DAC_READ_SEARCH` merely to use `AT_EMPTY_PATH`; the initial non-root design selects `/proc/self/fd/<fd>` with `AT_SYMLINK_FOLLOW`.
- Verify procfs and the exact linked inode against the still-open descriptor. Unsupported capability fails safely; no named-temp or privileged fallback is permitted.
- Never cache capability globally across different destination mounts.

### 6.10 Denial of service and resource exhaustion
Threat: huge trees, deep paths, many files, oversized journal records, or unbounded queues exhaust memory, descriptors, CPU, or disk.

Controls:
- Bound worker count, queue capacity, buffers, path lengths, parser records, and open descriptors.
- Check arithmetic and allocation multiplication for overflow.
- Define cancellation and backpressure behavior.
- Avoid unbounded recursion.
- Report resource exhaustion distinctly and preserve recoverable state.

### 6.11 Concurrency and shutdown races
Threat: workers use freed contexts or closed journal descriptors; notifications are lost; queued work is abandoned.

Controls:
- Document producer/consumer topology and ownership transfer.
- Stop producers, wake workers, drain or explicitly cancel work, join threads, then close shared resources.
- Check all pthread/eventfd/read/write/close failures.
- Do not assume concurrent journal close and append are safe without a proven lifecycle protocol.
- Test partial startup and notification failures.

### 6.12 Container privilege escalation / host exposure
Threat: compromised process gains unnecessary host access.

Controls:
- Run non-root.
- Drop capabilities.
- Use read-only root filesystem where practical.
- Mount only required data directories.
- Never mount Docker socket or host root.
- Separate build dependencies from runtime image.
- Do not place secrets or credentials in image layers.

### 6.13 FCJ4 test-only append adapter

The current FCJ4 append adapter is a test harness, not a production journal facility. It is confined to `tests/support` and a Linux-only test executable. The harness supplies an already-open descriptor for a disposable private temporary file; the adapter does not create or open application journal paths.

Controls include `O_CLOEXEC`, `O_RDWR`, rejection of `O_APPEND`, regular-file/device/inode/owner/mode/link-count/size checks, non-blocking advisory `flock`, explicit-offset `pwrite`, `fsync`, and an explicit writer-commit acknowledgement before another append. A post-write identity/size check is defense in depth only. `flock` does not constrain non-cooperating writers, and same-UID interference cannot be eliminated by these checks.

Any uncertain write/fsync or missing writer acknowledgement requires preserving bytes and rebuilding state from complete replay. The adapter must not truncate, repair, accept a valid prefix, or perform filesystem recovery. Process-exit tests do not establish power-loss durability. No production journal path, CLI, mover, or recovery path may link or invoke this adapter.

The adapter's atomic try-guard is fail-fast exclusion for overlapping calls on a live context; it is not a blocking mutex, queue, or context-lifetime mechanism. The caller must prevent new API entries once destruction begins and join all threads that may use the context before successful destruction. Calls racing with or following completed destruction are outside the contract. The dedicated cross-thread integration tests exercise selected overlap schedules; a clean ThreadSanitizer run is evidence only for the executed paths and does not prove arbitrary schedules safe. Pthread linkage and TSan instrumentation are confined to the Linux-only test executable; the production executable must not gain a dependency on the test adapter or its test synchronization harness.

## 7. Security testing requirements

At minimum, test:
- Traversal, absolute paths, dot-dot, separators, reserved paths, overlapping roots.
- Symlink root, symlink output, symlink source, symlink destination, and symlink swap.
- FIFO, socket, directory, and device entries.
- Existing destination collision and byte-for-byte preservation.
- EINTR, short read/write, ENOSPC, EIO, EACCES, fsync and close errors.
- Journal partial tail, malformed fields, overflow, invalid state, duplicate/conflicting transaction identity.
- Crash at each durable transition and repeated recovery.
- Replacement of source, destination, staging, and parent directory between operations.
- Quarantine symlink, wrong owner/mode, identity replacement, collision, and restore failure.
- Actual-destination filesystem capability probing as non-root.
- Queue exhaustion, partial thread startup, cancellation, and shutdown.
- C11/C17/C23 Debug and Release, sanitizers, static analysis, Docker and non-root execution.

Tests must use disposable fixtures. Assert exact bytes, types, identities, permissions, and journal outcomes—not only path existence.

## 8. Residual risks and explicit limitations

- Filesystem semantics vary by kernel, filesystem, mount options, and container environment.
- POSIX/Linux descriptor-relative operations reduce path races but do not make every check-then-unlink sequence race-free.
- A same-UID attacker may be able to interfere with process-owned writable state; private directory boundaries reduce exposure but do not create isolation from a fully capable same-UID adversary.
- fsync success is not a universal guarantee against every hardware/controller failure.
- The current FCJ2/legacy journal cannot safely authorize cleanup of a staging file whose identity it never recorded.
- The FCJ3/O_TMPFILE/quarantine behavior remains a design target until implemented and adversarially tested. Do not describe it as an existing production guarantee.

## 9. Security acceptance gate

A security-sensitive milestone is accepted only when:
1. The threat and affected assets are documented.
2. The design names the trust boundary and attacker capability.
3. Failure and crash semantics are explicit.
4. No weaker fallback is hidden.
5. Tests cover normal, failure, crash, and adversarial paths.
6. Tests run under the intended non-root identity and actual destination filesystem where required.
7. The PR records evidence, limitations, and rollback/recovery plan.
8. Required owner review and repository protections are satisfied.

