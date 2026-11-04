# Repository Engineering Rules

This file is the working contract for every human and automated contributor.

## Mission and priorities
Build a dependable file-management engine that is safe with user data. Priorities: correctness and preservation of files; security and least privilege; recoverability and observability; measured performance; maintainability and delivery speed. Never trade data safety for a benchmark result. No performance claim without a reproducible benchmark and recorded environment.

## Architecture
- Keep the first product as a modular C application. Do not introduce microservices, brokers, databases, or a web server without a demonstrated requirement.
- Keep CLI, configuration, scanning, classification, queueing, moving, journaling, and platform filesystem operations behind clear module boundaries.
- Use Hexagonal Architecture selectively for filesystem, clock, logging, and persistence boundaries where tests benefit.
- Use patterns only to solve a real problem; avoid speculative factories, singletons, and abstractions.
- Document thread safety, ownership, lifetime, error, and cancellation contracts.
- The current lock-free queue must only be used with its proven producer/consumer topology. Never silently change SPSC assumptions to MPSC.

## C standards
- Use clear names, small functions, single-purpose modules, and explicit status returns.
- Validate external input, sizes, integer conversions, path lengths, and allocations.
- Check all correctness- and security-relevant system-call results; preserve errno across cleanup.
- Avoid unbounded recursion and unchecked fixed-size path buffers.
- Avoid global mutable state in new code; pass context explicitly.
- Define who allocates, frees, and owns every pointer.
- Do not hide warnings, use undefined behavior, or rely on undocumented platform assumptions.

## Filesystem safety
- Treat paths, filenames, configuration, and watcher events as untrusted.
- Default new user-facing workflows to dry-run.
- Do not follow symlinks accidentally. Prefer descriptor-relative APIs and no-follow flags where supported.
- Defend against traversal, symlink swaps, TOCTOU, special files, and source/destination overlap.
- Never delete source until destination is fully written and safely published.
- Use exclusive creation; never truncate an existing user file.
- Journal intent and completion. Recovery must be idempotent and must not guess when state is ambiguous.
- Force must not bypass path validation or filesystem invariants.
- Destructive tests must use isolated temporary directories and assert source preservation on failures.

## Concurrency
- Document producer/consumer counts, ownership transfer, synchronization, and shutdown.
- Check pthread, eventfd, read, write, close, and allocation failures.
- No busy-spin or lock-free optimization without measurement and a documented correctness proof.
- Bound queues, worker counts, and memory. Shutdown must wake workers and account for pending work.

## Performance
- Establish a baseline before optimizing.
- Benchmark small files, large files, deep trees, many directories, and cross-filesystem moves.
- Record files/sec, bytes/sec, wall time, CPU, syscalls, memory, filesystem, storage, kernel, compiler, flags, and container limits.
- Prefer algorithmic and syscall improvements over speculative assembly.
- Do not use -march=native in portable release artifacts.
- Run sanitizers and correctness tests before performance comparisons.

## API, ABI, and FFI
- Keep the public C API small, versioned, and independent of internal structs.
- Prefer opaque handles and fixed-width integer types.
- Define ownership and matching release functions.
- Define ABI version negotiation and size/version fields for extensible structs.
- Never expose pthread types, FILE pointers, internal queues, or platform details in the stable ABI.
- Rust FFI must use the C ABI, repr(C) where required, explicit pointer/length contracts, and documented safety invariants.
- No Rust component until there is a concrete consumer and a benchmark or safety justification.
- Panics, exceptions, and longjmp must never cross the C ABI boundary.

## Security and delivery
- Containers run non-root, drop capabilities, use read-only root filesystems, and mount only required paths.
- Never mount Docker socket or host root.
- Do not commit credentials, user files, profiling dumps, or runtime data.
- Every behavior change needs focused tests or a documented reason tests are not yet possible.
- A test suite with no registered tests is not evidence of correctness.
- Docker changes must build in CI and runtime tests must use disposable mounted directories.
- Update README, HLD/LLD, and runbooks when behavior changes.
- PRs state scope, risk, security impact, tests actually run, gaps, and rollback plan.
- Never claim tests, builds, merges, or runtime behavior without evidence.

## Product and management
Prioritize customer value, data safety, reliability, support cost, and total cost of ownership. Build the smallest reliable product first. Add GUI, API server, cloud, and distributed processing only when customer demand justifies them.
