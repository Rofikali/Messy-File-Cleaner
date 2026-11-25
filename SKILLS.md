# Engineering Skills Catalogue

> **Configuration input contract:** Physical config lines must reject embedded NUL bytes before string parsing; failed loads preserve caller state; loader requires a nonempty path and initialized power-of-two map capacity with size not exceeding capacity; extension names reject control bytes; rejected extension keys and destinations must not be echoed into diagnostics. Hash-map initialization and access reject impossible byte extent or size metadata; callers still own backing storage. Classification uses the last POSIX path component; leading-dot-only and trailing-dot names use the fallback. SPSC queue initialization is exclusive; its unit suite checks concurrent FIFO transfer across repeated wraparound. Worker move diagnostics escape nonprintable/non-ASCII bytes and backslashes under a stream lock; original filesystem paths are unchanged. Root/output validation reports fixed rejection reasons without echoing untrusted paths. Linux watch registration requires a directory and rejects final-component symlinks; ancestor path identity is not established. Shared dispatch rejects empty paths and worker counts outside 1..MAX_WORKERS. Watcher event headers are copied before decoding; actionable names must be nonempty basename components. Worker notification and shutdown reject out-of-range pool metadata before indexing fixed arrays. The process-wide worker storage has one owner; duplicate starts and wrong-context lifecycle operations are rejected under caller serialization. Watcher idle-poll metrics count actual poll timeouts; integration waits for observed readiness instead of a fixed startup sleep. Scanner cancellation is checked before root access and immediately before callback handoff; it remains cooperative. Scanner errors retain the first observed failure even when later cancellation or callback failure stops traversal. Linux scanner strips trailing root separators before no-follow opening, preserving the root slash; ancestor identity is not established. Linux watcher also trims trailing root separators before no-follow registration and builds event paths from the trimmed root. Scanner and watcher share the internal root-path copy helper; it trims only trailing slashes and performs no filesystem resolution. CLI root preflight uses the same lexical root-copy helper before its existing lstat/realpath checks. CLI output preflight, scanner and watcher use a shared checked lexical path-join helper; callers retain component validation and filesystem policy. Scanner path construction and frame growth failures stop discovery and unwind owned frames; deep-tree allocation fault regressions check descriptor balance. Scanner directory acquisition stops on EMFILE, ENFILE or ENOMEM and unwinds owned frames; deterministic child-open fault tests check attempts and descriptor balance. Scanner resource regressions cover directory-stream acquisition faults and a real child-process RLIMIT_NOFILE exhaustion test without changing the parent or system limit. BENCH-SCALE-01 adds a bounded native C scanner benchmark with callback counts, monotonic timing, CPU and process peak RSS; smoke checks do not establish large-scale capacity. Native scanner evidence has a bounded manual-only C17 Release workflow with source/binary and runner provenance; no automatic scale run or spending authorization. BENCH-SCALE-01 final budget slot adds semantic evidence validation before upload; no performance result or release acceptance is inferred. Malformed bare recognized configuration settings use the same C whitespace classification as key normalization, including vertical tab and form feed; rejection preserves caller configuration. Hash-map existing-key updates accept overlapping borrowed value strings, including a suffix, without undefined memcpy overlap. Classifier initialization ignores a NULL context; classification rejects NULL contexts and NULL/empty filenames without changing valid-path routing. See [live board](next.md) for implementation and verification status. FS-01B/REL-01 remain blocked.

> **Current owner contract (24 Nov 2026):** Every authorized contributor (human, ChatGPT, Codex, Claude Code or other agent) uses the same resume-first [handoff contract](docs/AUTOPILOT_EXECUTION_CONTINUATION.md), [live board](next.md) and [feature ledger](docs/feature-budget.json). Hourly and manual `go ahead` are additive. Reconcile all authors' PRs, finish existing eligible work before starting another, count all opened PRs toward one stable five-PR budget, and update affected docs in the same PR. Actual GitHub state supersedes historical milestones below. No fake approval, security bypass or release authority.

> **Execution recovery (23 Nov 2026):** use the [hourly continuation and stall recovery](docs/AUTOPILOT_EXECUTION_CONTINUATION.md) for checkpoints, resume-first task selection, two-no-change escalation, and proportional L1–L7/CISSP/CCIE/Linux/SRE/CA-MBA review. This is a repository procedure, **not a running scheduler**; current GitHub and actual ChatGPT task history supersede dated snapshots.

> **Autopilot owner scope:** [zero required human approvals and gated full PR lifecycle](docs/AUTOPILOT_ZERO_REVIEW_AND_DOC_SYNC.md). Require real CI, honest self-review and source-preservation restrictions. Uppercase SKILLS remains the knowledge catalogue; lowercase skills.md remains procedural.

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
