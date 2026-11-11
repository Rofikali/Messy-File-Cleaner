# Engineering Skills and Contributor Workflow

This file is an operational companion to [AGENTS.md](AGENTS.md). It tells humans and coding agents how to work in this repository. The architecture and security rationale is in [the Engineering Playbook](docs/architecture/ENGINEERING_PLAYBOOK.md) and [Threat Model](docs/security/THREAT_MODEL.md).

## 1. Non-negotiable project identity

- Implement the product in **pure C**. C17 is the production baseline; validate C11 and C23 independently.
- No C++, C++ runtime, Rust core, or other implementation language without a new explicit owner decision and written review.
- Assembly is allowed only after profiling proves a CPU-bound bottleneck; it must be isolated behind C, have a portable C fallback, and pass equivalence tests.
- Linux/POSIX is the current platform target. Document platform and filesystem assumptions; do not imply universal portability.
- Data preservation and security outrank speed or delivery convenience.

## 2. Start-of-task workflow

1. Read `AGENTS.md`, this file, relevant architecture/security docs, and the module's public header.
2. Confirm the current branch and latest `main`; do not build new work on an obsolete or unmerged milestone.
3. Inspect callers, tests, error paths, ownership, thread topology, and existing invariants before editing.
4. State the task's scope, expected behavior, risks, and acceptance criteria.
5. For journal formats, recovery, destructive filesystem operations, public API/ABI, or concurrency changes, write or update a design/ADR before implementation.
6. Use a dedicated milestone branch. Keep changes small enough to review, but do not split a logically atomic safety change across uncoordinated PRs.

## 3. Design skills

- Model the domain before selecting a pattern. Use precise terms for source, destination, move operation, journal transaction, publication, retirement, and recovery.
- Keep modules cohesive and interfaces narrow. Prefer opaque context types and explicit status/error returns over hidden global state.
- Separate policy from mechanism: domain decisions should not depend unnecessarily on raw syscalls; filesystem adapters perform OS-specific operations.
- Use dependency inversion at meaningful boundaries (filesystem, journal, clock, randomness, logging) so tests can inject deterministic failures.
- Use state machines for workflows with durable or externally observable transitions. Define states, allowed transitions, terminal states, retry rules, and crash behavior.
- Use event-like notifications for worker coordination where they improve responsiveness. Do not introduce a broker or distributed EDA into this local application without a real multi-process/distributed requirement.
- Apply SOLID, DRY, KISS, and YAGNI pragmatically. Avoid both duplicated business rules and abstraction-heavy frameworks.
- Document ownership, lifetime, nullability, thread safety, cancellation, blocking behavior, and error semantics in public headers.

## 4. C implementation skills

- Compile with the project's explicit C standard and strict warnings. Do not depend on compiler extensions unless isolated and documented.
- Use fixed-width integer types for persistent formats and stable interfaces.
- Validate all external input, path components, lengths, integer conversions, arithmetic, allocation sizes, and syscall results.
- Check for overflow before allocation or buffer arithmetic.
- Prefer bounded formatting and parsing; check truncation and conversion results.
- Define ownership for every allocated object and file descriptor. Close/free exactly once; preserve the primary error during cleanup.
- Do not return pointers to temporary storage or expose internal structs across a stable ABI.
- Avoid undefined behavior, data races, unsafe signal-handler work, and non-atomic access to shared state.
- Do not place side effects inside `assert()`; tests must execute correctly with `NDEBUG`.
- Keep test hooks out of production builds.

## 5. Filesystem and transaction skills

- Treat names, paths, directory entries, symlinks, mount points, and watcher events as untrusted and potentially changing concurrently.
- Prefer descriptor-relative operations, `O_NOFOLLOW`, `AT_SYMLINK_NOFOLLOW`, exclusive creation, and no-replace publication where supported.
- Check source and destination identity/type immediately before operations that depend on them; document remaining race boundaries.
- Never overwrite an existing user destination.
- Never remove the source until the destination is complete, verified, safely published, and the protocol permits retirement.
- Never infer ownership of a temporary file from a filename prefix.
- Journal records are durable evidence, not ordinary log messages. A partial, malformed, conflicting, or ambiguous journal must fail closed.
- Recovery must be idempotent where possible and must preserve unknown or mismatched entries.
- FCJ2 and legacy records must retain their documented compatibility behavior. Old formats must never authorize cleanup of temporary entries that they cannot identify.
- Do not add a silent fallback that weakens atomicity, durability, or security.

## FCJ4 journal-state skills

- Treat FCJ4 as a separate protocol, not an extension of FCJ2/legacy or FCJ3.
- Keep replay parsing pure and all-or-nothing. Never perform filesystem or journal I/O in the parser.
- Reconstruct writer state only from a successfully parsed complete replay, and deep-copy all evidence needed beyond replay lifetime.
- Validate every proposed SEALED/ACTION_DEF record against immutable BEGIN/SEALED evidence before prepare. Writer acceptance must not create a history the parser rejects.
- Preserve exact move and action sequences, independent lifecycle states, unique IDs, quarantine-action dependencies, and protocol resource limits.
- Keep prepare/commit/abort semantics explicit: in-memory commit is not durable append or fsync.
- Keep FCJ4 parser, serializer, and writer-state modules isolated from production. The separately approved Linux-only test append adapter belongs only under `tests/support` and may operate only on caller-supplied descriptors for disposable private temporary files.
- Use explicit-offset `pwrite()`, reject `O_APPEND`, validate descriptor/file identity and private-file properties, and acquire a non-blocking advisory lock. Document that advisory locks do not constrain non-cooperating writers.
- Enforce the durable-write → writer-commit acknowledgement handshake. Do not accept another append while a durable record is pending in-memory commit; on commit mismatch, poison the session and rebuild from complete replay.
- Test short writes, EINTR, partial-write and fsync uncertainty, cleanup errors, descriptor ownership on init failure, restart/replay, and process exit. Process termination is not a power-loss test.
- Never open/create the production FCJ4 journal path, link the adapter into production, expose a CLI, or add recovery planners/executors or filesystem mutations as part of this milestone.

## 6. Concurrency skills

- State the number and roles of producers/consumers for every queue.
- Document who owns each item before enqueue, while queued, after dequeue, and on cancellation/shutdown.
- Bound queue capacity, worker count, and memory use.
- Handle partial thread startup and every synchronization/notification failure.
- Shutdown must stop new work, wake blocked workers, account for active and queued work, join threads, and only then close shared resources.
- Do not assume a mutex makes concurrent close safe unless the lifecycle contract proves it.
- Use atomics only with a written memory-ordering and topology rationale. Prefer simpler synchronization until profiling proves a need.

## 7. Test skills

Use layered tests:
- Unit: parsing, validation, classification, state transition rules, checked arithmetic.
- Integration: module boundaries and real temporary directories.
- End-to-end: CLI behavior, exit codes, dry-run and explicit execution guard.
- Fault injection: syscall failures, short reads/writes, EINTR, ENOSPC, EIO, fsync, close, publication, and cleanup errors.
- Crash/restart: terminate a child process at each durable transition and verify exact bytes, paths, journal state, and repeat recovery.
- Adversarial filesystem: symlink swaps, path replacement, hard links, special files, mount/filesystem capability differences, name collisions, and permission boundaries.
- Compatibility: C11/C17/C23; Debug and Release.
- Runtime: sanitizers, static analysis, Docker, and non-root tests where configured.

Assertions:
- Verify exact source and destination bytes, not only existence.
- Check file type, device/inode identity, permissions, and directory durability when relevant.
- Verify unknown or unowned entries are untouched.
- Verify retry behavior after transient and permanent errors.
- Ensure tests are deterministic and do not depend on root privileges unless root-specific behavior is the subject.
- Keep fixtures isolated and clean only files created by that fixture.

## 8. Makefile workflow

The Makefile is the primary developer interface:
- `make` — default Debug/sanitizer build.
- `make test` — Debug CTest suite.
- `make validate` — isolated C11/C17/C23 validation.
- `make release` — Release build.
- `make run` — dry-run.
- `make run-execute CONFIRM_MOVE=YES` — real file operation; only after explicit owner request and input review.
- `make benchmark` / `make benchmark-moves` — inspect workload and fixture isolation before use.
- `make clean` — build/profiling artifacts only.

Keep parallelism bounded (default `JOBS=2`). The Makefile is not a security boundary; application validation remains mandatory.

## 9. Security review checklist

Before approving code, ask:
- What are the assets and trust boundaries?
- Can an attacker control the path, filename, file type, directory, mount, configuration, or timing?
- Is there a symlink, traversal, TOCTOU, hard-link, special-file, or same-UID race?
- Can a failure cause overwrite, data loss, partial publication, or false recovery?
- Are integer overflow, truncation, resource exhaustion, and unbounded work possible?
- Are permissions, ownership, descriptor lifetime, and cleanup correct?
- Are errors fail-closed without destroying evidence?
- Are test-only controls absent from production?
- Does the threat model accurately state residual risks?

## 10. Pull request quality

Every PR must include:
- Problem and user/business impact.
- Scope and explicit non-goals.
- Architecture/design decision and affected module contracts.
- Data-safety and threat-model impact.
- Compatibility/API/journal-format impact.
- Tests actually run, with commands and CI links/results.
- Tests not run and known limitations.
- Performance measurements only when relevant, with environment and repeat count.
- Rollback or recovery plan.

Do not claim a milestone passed until its acceptance criteria pass. Do not merge without required owner review and repository governance. Verify the merge on `main` before starting dependent work.

## 11. Documentation maintenance

Update the relevant documents in the same change:
- User-visible behavior: README and CLI help.
- Module contract: header comments and LLD.
- Architecture or persistent protocol: HLD/LLD and ADR.
- Security behavior: threat model and security runbook.
- Operations: build, recovery, troubleshooting, and release notes.
- Tests: describe what is proven and what remains unproven.

Documentation must distinguish implemented behavior from planned design. Never present a design proposal as a production guarantee.
