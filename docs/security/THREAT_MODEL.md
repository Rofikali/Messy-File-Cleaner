# Security Threat Model

## Worker live-count consistency

The serialized worker lifecycle tracks the actual started count independently of the caller context. A live in-range count mismatch rejects notification (EINVAL) and join (EBUSY) before changing running state, indexing worker storage or closing descriptors. Tests temporarily understate and overstate the count of a two-worker dry-run pool, verify unchanged ownership/descriptor metadata and restore the actual count before existing drain/join tests. Partial-start cleanup and successful join reset the authoritative count. This is not synchronization for concurrent lifecycle mutation or validation of arbitrary descriptors; no mover or retirement change.

## Scanner directory-read resource exhaustion

A readdir failure with EMFILE, ENFILE or ENOMEM records the first error and stops discovery before subsequent frames or sibling entries are visited. Owned streams and paths unwind through the existing cleanup loop. Test-only depth-targeted faults cover root, first child and deeper frames for all three errors, with child-open counts at root/first-child boundaries, descriptor balance, fixture existence and successful rescan. Normal EOF and other directory-read errors retain existing behavior. No real system-wide exhaustion, source-retirement guarantee or mover change.

## CLI worker override uniqueness

Repeated --workers options fail at CLI parsing before configuration loading or filesystem operations, including identical counts and overrides separated by other arguments. This prevents silent last-value-wins resource configuration. Single valid counts retain their existing range and behavior; disposable CLI tests assert usage failure and no configuration/journal creation. No worker scheduling, filesystem retirement, or ABI change.

## Watcher terminal-record regression

Synthetic inotify batches cover IN_Q_OVERFLOW (ENOSPC), IN_IGNORED, IN_DELETE_SELF, IN_MOVE_SELF and IN_UNMOUNT (ESTALE), including overflow precedence and a following actionable file record. Assertions require zero events seen/enqueued and no worker-error side effect in the parser. This is test coverage of existing fail-closed behavior, not automatic resynchronization or proof of real kernel overflow recovery. No production source or filesystem policy changes.

## Watcher buffered cancellation boundary

A pending stop signal is checked before decoding each buffered inotify record and again before constructing actionable paths. Cancellation at those boundaries returns ECANCELED without handing off the pending record; start_watcher retains its existing graceful-stop handling. Tests exercise a valid pending record and a truncated record under pre-existing cancellation with zero events seen/enqueued. Earlier accepted work and races after a cooperative check remain outside this guarantee. No filesystem retirement or mover policy changes.

## Configuration initialization boundary

Configuration initialization returns immediately for NULL and resets only scalar defaults for a valid context. Regression covers NULL initialization/cleanup and preservation of caller-owned populated extension mappings during scalar reset. This does not validate arbitrary pointers or authorize changed ownership or filesystem policy.

## Hash-map insertion capacity consistency

An unused slot encountered with size equal to capacity is rejected before mutation, preventing the size count from exceeding capacity. Unit tests compare the unchanged backing-storage bytes on contradictory metadata and verify normal full-map rejection and existing-key replacement. This does not establish that arbitrary caller-owned metadata, pointer lifetime or storage extent is valid; no ABI or filesystem-policy change.

## Scanner metadata resource exhaustion

Entry metadata lookup or opened-directory identity lookup returning EMFILE, ENFILE or ENOMEM stops discovery, preserves the first recorded error and unwinds owned frames. Ordinary ENOENT and non-resource entry failures retain their previous behavior. Disposable entry-lookup injections check zero callbacks, descriptor balance, fixture preservation and successful rescan. Opened-directory fstat faults are injected after zero or three successful identity lookups for each resource errno; exact child-open attempts, raw descriptor/frame cleanup, absence of deep-file callbacks and subsequent successful rescans are asserted. The injection is compiled only in scanner tests. No source-retirement guarantee or mover change.

## Worker event validation boundary

Workers reject empty or non-NUL-terminated fixed-buffer event paths before classification, and reject NULL/empty routing results before diagnostics or move handling. A fixed diagnostic avoids echoing malformed bytes; rejection increments move failures, sets worker_error and stops intake while existing queued work follows the existing drain policy. Dry-run disposable unit/integration inputs exercise both malformed path cases and missing/empty classification, including queued valid-event drain. This does not validate arbitrary pointers or filesystem containment, nor change mover or source-retirement semantics.

## SPSC queue argument boundary

Null queue and event arguments cannot reach atomic accesses or event copies; invalid transfers return zero, null initialization does nothing and a null emptiness query returns true. Unit cases verify a queued event survives invalid calls and the existing concurrent FIFO regression remains unchanged. This is not validation of arbitrary non-null pointers, uninitialized storage, or multi-producer/multi-consumer access. No filesystem retirement changes.

## Classifier API argument boundary

Classification returns NULL before dereferencing a NULL context or scanning a NULL/empty filename. Initialization with a NULL context is a no-op. Disposable unit cases exercise invalid arguments and preserve valid-path routing and map size. This is a defensive C API guard, not validation of arbitrary non-NULL pointers or proof of filesystem containment. No mover or retirement changes.

## Hash-map borrowed-value update

Existing-key replacement uses `memmove` for potentially overlapping borrowed value strings from `hashmap_get`. The unit test checks identical and suffix aliases, preserved map size and subsequent independent replacement. This does not relax backing-storage ownership, require a new ABI, or provide concurrent access synchronization.

## Bare configuration token boundary

Malformed bare recognized settings separated by vertical tab or form feed must fail rather than silently retain defaults. Token recognition uses `isspace` with unsigned-char input consistently with key normalization. Disposable regression checks late failure preserves scalar and routing state. No filesystem retirement, ABI, or release change.

## Scanner directory resource exhaustion

Child-directory open and fdopendir failures with EMFILE, ENFILE or ENOMEM stop discovery and unwind owned frames, retaining the first observed failure. Other entry-specific failures keep their existing handling. Test-only child-open injection checks immediate and nested failures, exact attempt counts, Linux descriptor balance and subsequent successful scan. Additional tests inject ENOMEM before root and child fdopendir ownership transfer, check raw descriptor balance and nested unwind, and exercise real EMFILE in a disposable forked child using a soft RLIMIT_NOFILE of at most 32, with four free slots before scanning. The parent and system-wide limits are unchanged. The child restores its limit, checks descriptor balance and rescans successfully. Real ENFILE/ENOMEM exhaustion is not attempted. No earlier callback rollback, universal descriptor bound or source-retirement guarantee.

## Scanner allocation failure boundary

Path construction and frame-array growth failures terminate discovery rather than repeatedly allocating under resource pressure; first failure errno is retained, and all owned frames are unwound. A test-only one-shot frame-growth failure hook exercises initial allocation and growth beyond 16 frames on a disposable 20-level tree. Linux /proc/self/fd inventory checks descriptor balance after each failure and a subsequent successful scan. This does not roll back earlier callbacks, prove a universal memory bound, or establish source-retirement safety. The injection hook is absent from production.

## Shared lexical path joining

CLI output preflight, scanner and watcher use a common internal owned-copy path join. It rejects null/empty inputs, checks allocation arithmetic and inserts a separator only when needed (including a single root slash). Caller-side component validation is unchanged; the helper does not establish containment, canonicalize or resolve paths. Direct C regressions cover root/relative paths, existing separators, spaces, literal backslash, invalid inputs and independent ownership. Existing integration tests remain in place. No mover or retirement changes.

## Shared root path normalization

CLI root preflight, scanner and watcher share an internal static-inline lexical helper that allocates an owned root copy and removes trailing slashes only. NULL and empty inputs fail with EINVAL; all-slash roots reduce to a single slash. Direct regression covers absolute and relative paths, internal separators, dot components, literal backslash and spaces, input preservation and independent ownership. Existing scanner and watcher symlink regressions remain in place. CLI dry-run tests exercise one to three trailing separators on symlink, regular-file and real-directory roots, checking rejection diagnostics, preserved fixture bytes and absence of output directories. CLI lstat/realpath and dangerous-root policy remain unchanged; preflight does not establish race-free identity. This consolidation does not canonicalize paths, validate ancestors or close source-retirement races.

## Scanner root separator boundary

The Linux scanner removes trailing root separators (preserving a single root slash) before O_NOFOLLOW directory opening. This avoids a trailing slash making a final symlink an intermediate component. Disposable tests cover one to three trailing separators on a root symlink and real directory, verify no rejected-root callbacks and unchanged discovered paths for valid roots. This does not reject ancestor symlinks or dot-component aliases, prove pathname identity, or close source-retirement races. Watcher registration now applies the equivalent separator normalization; see the watch registration boundary below.

## Scanner failure reporting boundary

Traversal retains the first observed failure errno; later callback failure or cancellation still stops traversal without masking an earlier discovery failure. An unset failure errno falls back to EIO. A single-threaded, test-only metadata failure hook exercises EACCES followed by EPERM, cancellation, or successful callback completion on disposable fixtures. This improves diagnostic integrity, not filesystem identity or rollback. The hook is not compiled into production.

## Scanner cancellation boundary

Valid pre-cancelled scans return ECANCELED before opening the root; discovery rechecks cancellation immediately before file callback handoff. Input validation still takes precedence over cancellation. Disposable tests cover missing roots with and without cancellation, cancellation requested by the first callback, and source existence; known fixtures are removed on successful test completion. Checks are cooperative, not atomic with syscalls or callback execution, and do not undo queued work or establish source-retirement safety.

## Worker pool ownership boundary

Process-wide static thread storage has a single owning context. Duplicate starts and wrong-context notification/join fail before overwriting live metadata or touching descriptors. Disposable C regression exercises duplicate and cross-context calls, original pool preservation, partial-start cleanup through existing tests, and ownership transfer after a successful join. This guard assumes caller-serialized lifecycle operations and stopped producers before join; it does not provide concurrent lifecycle synchronization or repair failed pthread joins. Filesystem retirement remains outside scope.

## Worker lifecycle bounds

Notification validates pool count and index before accessing descriptor arrays. Shutdown rejects counts outside 0..MAX_WORKERS before changing running state or joining threads. C regression covers negative and excessive counts, null contexts, invalid indexes and zero-worker shutdown. Invalid metadata rejection does not repair a corrupted live pool; callers must serialize lifecycle operations and preserve stable metadata. No new filesystem mutation or source-retirement guarantee.

## Watcher event record boundary

Inotify headers are copied into aligned local storage before decoding; length checks precede name reads. Actionable non-directory create/move records reject empty, unterminated, slash-containing, dot and dot-dot names. Synthetic tests cover truncated and deliberately unaligned records, invalid names, overflow and invalidation errors, and a non-actionable directory event. The test entry point is compiled only into the watcher integration target. This is parser hardening, not proof of kernel event authenticity, root identity or filesystem retirement safety.

## Shared dispatch bounds

The shared batch/watcher dispatch helper rejects null context, null or empty path, and worker counts outside 1..MAX_WORKERS before hashing. Valid paths retain the existing hash and deterministic routing; callers must keep the worker count stable during dispatch and initialize the queues before use. Direct C regression checks invalid inputs and all supported worker counts. This does not establish concurrent context mutation safety or filesystem identity.

## Watch registration boundary

Linux watch registration trims trailing root separators (preserving a single root slash) before IN_ONLYDIR and IN_DONT_FOLLOW to reject regular files and final-component symlinks, and constructs event paths from the trimmed root. Disposable direct-API tests cover both rejection cases, one to three trailing separators on symlink and real directory roots, unchanged file bytes, and a bounded CTest timeout. The running integration watches a real directory with trailing separators and verifies dry-run event processing. This does not verify ancestor identity, dot-component aliases, later pathname replacement, or move safety; FS-01B remains blocked.

## Root/output validation diagnostic boundary

Root and output preflight rejection messages omit raw input paths and canonical output paths, retaining the reason and existing errno behavior. Disposable CLI tests exercise inaccessible roots, regular-file roots, symlinks, root/output overlap and output symlinks with control-containing path components, preserving source content. This is diagnostic hardening only; root validation policy and filesystem mutation behavior are unchanged. Other log sites remain outside this scope.

## Worker diagnostic output boundary

Worker preview, move-success and move-failure messages encode nonprintable and non-ASCII path bytes and literal backslashes as printable ASCII hex escapes. A stderr stream lock keeps each worker record together. This is presentation only; paths passed to classification and filesystem operations are unchanged. Disposable read-only preview regression covers newline, ESC and backslash and verifies source preservation. Other logging sites and actual execution-mode output are not covered by this regression; this does not resolve FS-01B or authorize release.

## SPSC queue initialization and transfer boundary

Queue atomics are initialized exclusively before publication or after all users stop. The C unit suite checks FIFO payload/order and final emptiness during concurrent producer/consumer wraparound, with a bounded CTest timeout. This does not establish MPSC/MPMC correctness, race freedom under every schedule, or filesystem preservation. Pthread test linkage and timeout do not alter production queue capacity, ordering operations or ABI.

## Classifier routing boundary

Extension extraction examines the final POSIX path component only; parent-directory dots do not determine routing. A leading dot alone and trailing dots use the default route; multi-extension names retain the final suffix and matching remains case-sensitive. Disposable C unit tests exercise these cases. This corrects classification semantics, not filesystem identity, move safety or release approval.

## Hash-map metadata trust boundary

The C hash map rejects capacity whose entry-byte extent exceeds SIZE_MAX and rejects size greater than capacity before lookup or insertion. Invalid initialization resets map metadata without touching backing entries. This validates representability, not actual caller-owned buffer extent or entry integrity; no ABI or filesystem mutation contract changes. Unit regressions exercise malformed metadata and preserve valid routing entries.

## Configuration diagnostic trust boundary

Extension routing names reject control bytes before insertion into the map; rejection uses a fixed diagnostic. Existing safe destinations and ordinary extension keys retain their behavior. Disposable integration tests cover short control-containing keys and exact diagnostic bytes.

The configuration loader rejects null/empty paths and invalid hash-map capacity/size metadata before staging. Callers remain responsible for actual backing-storage extent and initialized entries; metadata validation is not proof of memory ownership.

Extension keys and destination values are untrusted input. Rejection messages must not echo their raw bytes into stderr; fixed diagnostics preserve the reason category without terminal escape injection or disclosure of rejected values. Numeric setting names may be emitted only after matching a fixed allowlist. Regression tests capture exact stderr bytes from disposable inputs. This scope does not certify all CLI logging, filesystem safety or release readiness. See [live board](../../next.md) for verification.

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

