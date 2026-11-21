# Engineering Skills Catalogue

This catalogue describes knowledge useful for building, reviewing, securing, operating, and evolving the project. It is **not** a second set of agent instructions, a mandate to introduce every technology, or an authorization to ship them. For binding rules use [AGENTS.md](AGENTS.md); for execution recipes use [skills.md](skills.md); for task status use [next.md](next.md); for approved language scope use the [Rust adoption ADR](docs/architecture/ADR_RUST_ADOPTION_STRATEGY.md).

## Feature budget and evidence classification

Follow [the five-PR delivery policy](docs/FEATURE_DELIVERY_AND_ACCEPTANCE_POLICY.md). This is a prospective per-feature **ceiling**, including closed/failed PRs, not a target. Close PERF-01 legacy iteration and prioritize the safer CLI-DRYRUN-01 user-facing feature. Separate C17 code merged, C11/C17/C23/Ubuntu/Docker CI observed, actual host/filesystem/backup checks NOT RUN, security incident BLOCKED, and owner-approved release. Security/CISSP, Linux/POSIX, QA/SRE, financial liability, MBA delivery cost and management opportunity cost are review lenses rather than unverified credentials.

## Core implementation
- C17/C23, POSIX APIs, Linux syscalls, libc, errno, file descriptors
- Memory ownership, allocation, alignment, lifetime, undefined behavior
- Filesystem semantics: inode, directory entries, hardlinks, symlinks, mounts, rename, EXDEV, fsync
- CMake, Ninja, Make, compiler/linker flags, static analysis
- GDB, sanitizers, Valgrind, strace

## Architecture and design
- HLD, LLD, module boundaries, dependency direction, contracts
- SOLID adapted to C, DRY, KISS, YAGNI, clean code
- Modular monolith, selective Hexagonal Architecture, pipeline, producer-consumer
- Strategy, Adapter, Observer, State Machine, Command where justified
- API design, ABI compatibility, semantic versioning, deprecation
- DDD only where a meaningful domain model exists

## Data structures and concurrency
- Bounded ring buffers, SPSC/MPSC/MPMC queue contracts
- Hash tables, collision handling, path stacks, iterative DFS
- Complexity, cache locality, false sharing, memory pools
- C memory model, atomics, pthreads, mutexes, eventfd, inotify
- Backpressure, cancellation, shutdown, race detection
- Idempotency, journaling, crash consistency, recovery, fault injection

## Security
- Threat modeling, trust boundaries, path traversal, symlink attacks, TOCTOU
- Special files, permissions, least privilege, Linux capabilities and namespaces
- Input validation, integer overflow, bounds checking, secure temporary files
- Supply-chain security, dependency review, SBOM, vulnerability management

## Performance and interoperability
- Benchmark design, perf, flame graphs, strace -c
- CPU vs I/O analysis, page cache, filesystem and storage behavior
- Compiler optimization, LTO, PGO, vectorization, SIMD
- Latency percentiles, throughput, memory peak, syscall count
- Public C headers, stable C ABI, Rust FFI, extern C, repr(C), opaque handles
- Ownership transfer, allocator boundaries, panic containment, shared/static libraries

## Delivery, operations, and management
- Docker multi-stage builds, Compose, non-root containers, read-only root, volume permissions
- GitHub Actions, clean builds, CI gates, release artifacts
- Logging, metrics, incident diagnosis, recovery runbooks
- Customer validation, MVP scope, build-vs-buy, total cost, release planning, risk register, quality metrics, ROI

## Milestone delivery and merge governance
- Scope and acceptance-criteria definition for independent milestones
- Branch-per-milestone workflow, PR review, CI evidence, security sign-off
- After required checks pass, obtain owner review/approval and confirm the PR is actually merged into `main` before beginning a dependent milestone; an agent cannot infer permission to merge from a passing check
- Verify merge commit and main-branch checks; never claim an unconfirmed merge
- Handle failed gates in the same milestone instead of advancing

## Nanosecond-aware performance engineering
- Distinguish clock resolution, measurement precision, and actual operation latency
- Use monotonic clocks and batched microbenchmarks to amortize timer overhead
- Measure median, p95, p99, throughput, variance, and warm/cold filesystem cases
- Optimize nanosecond-scale CPU-bound paths only when repeatable evidence supports it
- Do not promise nanosecond end-to-end file moves; report filesystem and durability costs honestly

## Learning and implementation cycle
1. Explain the concept in plain language.
2. Identify the concrete repository problem it solves.
3. Implement the smallest safe example.
4. Add automated tests.
5. Measure or review the result.
6. Document the decision and limitations.

Do not add technology merely to make the architecture look advanced.

## Filesystem path boundary validation
- Validate destination configuration as untrusted input.
- Canonicalize input roots and compare path components safely (not string-prefix-only checks).
- Test root/output overlap, symlink roots, symlink destinations, reserved journal paths, and forced-root behavior in temporary directories.


## Production-language constraint and approved test-only exception

The owner has selected **pure C17 for the production engine**, not C++ or a Rust rewrite. This is not an open C-versus-C++ evaluation. The accepted [Rust adoption ADR](docs/architecture/ADR_RUST_ADOPTION_STRATEGY.md) explicitly permits a **test-only Rust verification pilot**; [PR #82](https://github.com/Rofikali/Messy-File-Cleaner/pull/82) merged its isolated scaffold. Knowledge of Rust FFI in this catalogue is not permission to link Rust into the production executable.

- Production core, shipped native tools and product build targets use C. Existing native C tests remain C; the approved Rust verification crate is isolated under `tests/rust/` and is not a product dependency.
- Never add C++ source files (`.cpp`, `.cc`, `.cxx`), C++ classes/templates/exceptions/STL, C++ runtime dependencies, or CXX build targets.
- C17 is the production baseline; C11 and C23 are separate validated compatibility build targets, not a promise that all compilers support both equally.
- Use explicit C interfaces, opaque handles where useful, clear ownership/lifetime contracts, and modular boundaries.
- Future production interoperability, if approved, requires a reviewed versioned C ABI, explicit ownership/lifetime rules and compatibility tests. No public production ABI or `.a`/`.so` SDK currently exists.
- Assembly is allowed only as an isolated, benchmark-justified optimization with a portable and tested C fallback.
- Do not add Rust or another implementation language to the production core without a new explicit owner decision and architecture/security review. A future **test-only C/Rust parser shim** is a separate staged milestone and may not link into the shipped executable.

When a proposed design or tool suggests C++, translate the design into idiomatic, safe, maintainable C instead.
