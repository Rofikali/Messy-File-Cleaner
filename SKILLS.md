# Engineering Skills Catalogue

**Supply-chain reference:** [build integrity](docs/security/SUPPLY_CHAIN_REFERENCE.md) begins SUPPLY-CHAIN-01 with a bounded fail-closed authenticated APT refresh in the existing production-image build after #260's observed mirror inconsistency. UPSTREAM-FAULT-01 scoped DONE 5/5; no runtime binary or dependency-set change.

**Final configuration fault acceptance:** [fault reference](docs/security/UPSTREAM_FAULT_REFERENCE.md) adds 12 simultaneous read/close fault combinations with all acquisition/allocation adapters armed, full rollback, descriptor closure, matching staged free and clean retries. #259 merged, 13 completed jobs PASS, Makefile pending at inspection; #258 Ubuntu image still pending; 70/70 C17 Debug/Release and 24 fault cases PASS. Final candidate CI pending; no production change.

**Configuration acquisition fault reference:** [fault acceptance](docs/security/UPSTREAM_FAULT_REFERENCE.md) adds isolated open and fdopen failure controls, exact staged allocation/free, raw descriptor closure and retry. #258 merged, 13 completed jobs PASS, Ubuntu image pending at inspection; 70/70 C17 Debug/Release and three allocation faults PASS. Candidate CI pending; no production change.

**Configuration allocation fault reference:** [fault acceptance](docs/security/UPSTREAM_FAULT_REFERENCE.md) adds isolated staged malloc ENOMEM at capacities 2/8/64, no stream acquisition, complete rollback and successful retry with one matching staged free. #257 13/13 returned jobs, 70/70 C17 Debug/Release and 3 close faults PASS; candidate CI pending. No production change.

**Configuration close fault reference:** [fault acceptance](docs/security/UPSTREAM_FAULT_REFERENCE.md) adds EIO/ENOSPC/EINTR stream-close reports after successful EOF, full rollback and same-adapter positive commit controls. #256 14/14 jobs, C17 Debug/Release 70/70 and 12 read fault cases PASS; new candidate CI pending. No production change.

**Upstream fault reference:** [fault acceptance](docs/security/UPSTREAM_FAULT_REFERENCE.md) begins UPSTREAM-FAULT-01 with a test-only GNU stdio adapter for deterministic post-open config stream faults, full caller-state rollback, descriptor close and recovery. Linux selftest scoped DONE 5/5. No production change.

**Linux selftest final leak oracle:** [scanner security](docs/security/FILESYSTEM-SCANNER-SECURITY.md) adds intentional deepest-directory inheritance solely in forked test children and exact positive/negative oracle controls. #254 13/13 returned jobs, 69/69 C17 Debug/Release and deep identity checks PASS; final candidate CI pending. No production change.

**Linux deep descriptor reference:** [scanner security](docs/security/FILESYSTEM-SCANNER-SECURITY.md) adds a private 20-directory chain across the initial 16-frame capacity, exact live inode CLOEXEC/exec checks, cancellation and successful rescan. #253 merged, 12 completed jobs PASS, Ubuntu image pending at inspection; candidate CI pending. No production change.

**Linux scanner abort lifecycle reference:** [scanner security](docs/security/FILESYSTEM-SCANNER-SECURITY.md) adds callback EACCES/EIO and cooperative cancellation after actual child exec probes, inode-specific descriptor restoration and immediate successful rescans. #252 13/13 returned jobs and 69/69 C17 Debug/Release PASS; candidate CI pending. No production change.

**Linux scanner exec inheritance reference:** [scanner security](docs/security/FILESYSTEM-SCANNER-SECURITY.md) adds real fork/exec of the same test binary during each private scan callback, with negative and intentional positive inheritance controls, parent restoration and preserved bytes. #251 14/14 jobs and C17 Debug/Release 69/69 passed; new candidate CI pending. No production change.

**Linux selftest reference:** [scanner descriptor security](docs/security/FILESYSTEM-SCANNER-SECURITY.md) begins LINUX-SELFTEST-01 with actual scanner FD_CLOEXEC checks on private directory inodes during callbacks, descriptor restoration and preserved file bytes. CPU research scoped complete 5/5; retain production classifier. No production change.

**CPU guard-page final reference:** [baseline](docs/performance/CPU_OPT_REFERENCE.md) adds Linux inaccessible pages around read-only input, both edges, all eight entry modes and 82560 exact comparisons. #249 dispatch checks passed; final candidate CI pending. Retain production classifier; no universal speedup or production ISA change.

**CPU dispatch policy reference:** [baseline](docs/performance/CPU_OPT_REFERENCE.md) adds one-time capability-intersected selection, forced portable execution, four allowed masks and an independent policy truth table. Native selection remains test-only; no production change.

**CPU explicit vector reference:** [baseline](docs/performance/CPU_OPT_REFERENCE.md) adds guarded test-only SSE2/AVX2 byte counts, bounded vector loads and tails, generic x86-64 baseline and capability-tagged comparisons. No production change.

**CPU timing CI correction:** PR #247's earlier workflow had two split CTest selectors; restore exact single-line anchored selectors without weakening the matrix. Ubuntu C17 on the earlier head passed 68/68 and GCC reported 16-byte vectorization of the portable loop. Corrected-head CI pending.

**CPU timing reference:** [baseline](docs/performance/CPU_OPT_REFERENCE.md) adds 54 bounded timing rows across three lengths and match densities, retains 165120 correctness comparisons, and requests GNU vectorization diagnostics plus C17 Release disassembly. No production change.

**CPU optimization reference:** [baseline](docs/performance/CPU_OPT_REFERENCE.md) begins CPU-OPT-EVAL-01 with bounded scalar/libc byte-count equivalence and direct Debug sanitizers; cache-layout scoped evaluation closed 5/5, retain production layout. No production change.

**Cache-layout final evaluation:** [evidence](docs/performance/CACHE_LAYOUT_REFERENCE.md) records #244 exact-head 14/14 jobs and C17 Release 67/67 tests, including 256 immutable reader digests and oversized extent rejection. Final candidate instruments the reference target itself with fatal ASan/UBSan in Debug; CI pending. Retain production layout; no production speedup claim.

**Cache-layout capacity and readers reference:** [evidence](docs/performance/CACHE_LAYOUT_REFERENCE.md) compares logical metadata against trusted owner allocation extents and tests four immutable readers (64 digests each), joined before cleanup. Experimental only; no production change.

**Cache-layout ownership reference:** [evidence](docs/performance/CACHE_LAYOUT_REFERENCE.md) adds bounded packed snapshot construction, allocation-failure injection, independent lifetime and metadata corruption rejection. No production change.

**Cache-layout timing reference:** [evidence](docs/performance/CACHE_LAYOUT_REFERENCE.md) adds 36 bounded construction/traversal measurements with independently owned snapshots and verified full-path digests. No production change.

**Cache-layout evaluation:** [reference](docs/performance/CACHE_LAYOUT_REFERENCE.md) compares fixed FileEvent arrays with an independent packed path/index model; prior queue padding remains inconclusive, batch I/O closed 3/5, no production change.

**Batch I/O failure reference:** [evidence](docs/performance/BATCH_IO_REFERENCE.md) adds deterministic bounded short writes, injected EINTR/zero/EIO and exact prefix preservation; timing advantage versus scalar128k is small, retain production mover.

**Batch I/O timing reference:** [evidence](docs/performance/BATCH_IO_REFERENCE.md) adds 18 bounded wall/CPU timing samples with rotated method order; prior syscall counts show no advantage over scalar 128 KiB. No production change.

**Batch I/O evaluation:** [reference](docs/performance/BATCH_IO_REFERENCE.md) compares scalar 32/128 KiB and four-segment vectored transfers on private files. Scoped raw io_uring evaluation closed at 4/5; no liburing dependency or production change.

**Linux async I/O synchronization/error reference:** [io_uring evidence](docs/performance/LINUX_URING_REFERENCE.md) adds actual FSYNC, invalid descriptor, EOF and recovery completions after successful writes. Test-only private files; no production change.

**Linux async I/O vectored reference:** [io_uring evidence](docs/performance/LINUX_URING_REFERENCE.md) adds four READV and four WRITEV operations on anonymous private fixture files, with exact bytes and completion validation. Kernel version evidence recorded; no production changes.

**Linux async I/O NOP reference:** [io_uring reference](docs/performance/LINUX_URING_REFERENCE.md) adds bounded actual submission/completion validation with shared ring mappings after successful capability discovery. No file I/O, liburing dependency or production change.

**Linux async I/O capability:** [io_uring reference](docs/performance/LINUX_URING_REFERENCE.md) starts the LIBURING-EVAL-01 reference-only kernel probe; Linux I/O scoped evaluation closed at 3/5, retain buffered mover. No liburing dependency or production changes.

**Linux I/O offset reference:** [baseline](docs/performance/LINUX_IO_REFERENCE.md) adds explicit offset, short EOF transfer and untouched destination byte comparisons. Buffered transfer retained; no production change.

**Linux I/O timing reference:** [baseline](docs/performance/LINUX_IO_REFERENCE.md) adds six bounded transfer pairs with alternating order and verbose C17 Release evidence. Independent content-copy models only; production unchanged.

**Linux I/O evaluation:** [copy reference](docs/performance/LINUX_IO_REFERENCE.md) compares independent buffered content transfer with copy_file_range on private fixture files. Watcher evaluation closed at 3/5; no production mover change.

**Watcher lifecycle reference:** [kernel reference](docs/performance/LINUX_WATCH_REFERENCE.md) adds repeated actual watcher cancellation, root deletion, failed registration and descriptor-count restoration checks. No production change.

**Watcher batch reference:** [kernel reference](docs/performance/LINUX_WATCH_REFERENCE.md) adds real buffered inotify create/move-in events, exact raw record membership, production parser handoff and dry-run worker counters with unchanged file bytes. No production change.

**Linux watcher evaluation:** [kernel reference](docs/performance/LINUX_WATCH_REFERENCE.md) compares production terminal invalidation with a separate raw inotify watch on a private rename/replacement fixture. Path evaluation closed at 3/5; production unchanged.

**Linux path error evaluation:** [path reference](docs/performance/LINUX_PATH_REFERENCE.md) adds real invalid descriptor, path, request-size and resolution-option checks. No unrestricted fallback; no production adapter change.

**Linux path identity evaluation:** [path reference](docs/performance/LINUX_PATH_REFERENCE.md) adds a deterministic private-fixture rename and replacement comparison of held directory descriptors versus reopened names. No production trust change.

**Linux path evaluation:** [path resolution baseline](docs/performance/LINUX_PATH_REFERENCE.md) characterizes openat2 confinement on disposable directories only; conditional Linux UAPI test, no production fallback or trust change. Scanner scoped evaluation closed at 4/5; retain existing scanner.

**Scanner CI evidence:** [reference baseline](docs/performance/SCANNER_REFERENCE_BASELINE.md) adds verbose reporting of the existing bounded scanner reference smoke in the C17 Release CI leg only, with compiler/kernel/libc context. No production or scale-benchmark change.

**Scanner early-stop evidence:** [reference baseline](docs/performance/SCANNER_REFERENCE_BASELINE.md) adds callback error, nftw callback stop and native cooperative cancellation at first/middle/final regular-file callbacks on private fixtures, verifies exact partial counts, descriptor restoration and complete rescans. No production change.

**Scanner descriptor evidence:** [reference baseline](docs/performance/SCANNER_REFERENCE_BASELINE.md) adds untimed Linux procfs callback-visible descriptor observations for native scanner and nftw budgets 1/4/16, exact-set and baseline restoration checks. No production backend change.

**Linux scanner evaluation:** [scanner reference baseline](docs/performance/SCANNER_REFERENCE_BASELINE.md) compares actual read-only scanner with POSIX nftw FTW_PHYS on small private wide/deep fixtures; exact file set and no symlink callbacks required. String evaluation closed at 3/5; retain production classifier/scanner.

**String measurement evidence:** [extension baseline](docs/performance/STRING_EXTENSION_BASELINE.md) records standalone exact-blob C11/C17/C23 and ASan/UBSan execution; reverse candidate improved long synthetic prefixes but representative workload and target-host evidence remain absent. Retain production libc classifier.

**String boundary evaluation:** [extension baseline](docs/performance/STRING_EXTENSION_BASELINE.md) adds exhaustive short byte strings, explicit extension-length boundaries and an independent forward routing reference. Verify both classifier implementations and borrowed result identity; no production change.

**String-processing evaluation:** [extension baseline](docs/performance/STRING_EXTENSION_BASELINE.md) compares actual libc-based classifier with test-only reverse basename/extension scan. Preserve exact classification semantics and production code. Memory evaluation closed at 4/5; retain existing allocator.

**Memory unwind evaluation:** [path allocation baseline](docs/performance/PATH_ALLOCATION_BASELINE.md) adds deterministic interruption at each nested frame depth, real bounded arena exhaustion and modeled owned-join failure. Verify ancestor preservation, balanced releases and reuse; not actual scanner cancellation or system malloc-failure acceptance.

**Nested path lifetime evaluation:** [path allocation baseline](docs/performance/PATH_ALLOCATION_BASELINE.md) adds variable-length synthetic directory frames and transient siblings to the existing owned/arena comparison. Verify ancestor preservation and strict LIFO rewind; no production lifetime or allocator change.

**Bounded arena comparison:** [path allocation baseline](docs/performance/PATH_ALLOCATION_BASELINE.md) compares the actual owned helper with an isolated test-only batch-reset byte arena. Check exact output, bounds, independent live copies, allocation/free counts and reset lifetime. Retain production allocator and ownership; no speedup claim.

**Memory allocation evaluation:** [path allocation baseline](docs/performance/PATH_ALLOCATION_BASELINE.md) measures the actual owned lexical path helper with bounded simultaneous copies and exact output/independence checks. Hash-map scoped evaluation closed at 4/5; retain production map and allocator. No arena, new dependency, ABI or filesystem change.

**Hash-map collision evaluation:** [baseline](docs/performance/HASHMAP_BASELINE.md) discovers fixed colliding keys through the actual map, checks slot-63 wraparound, full-table rejection, existing-key updates and exact hit/miss results across 64/128/256 slots. Test-only; retain production capacity, no speedup or hostile-key resistance claim.

**Classifier workload evaluation:** [hash-map baseline](docs/performance/HASHMAP_BASELINE.md) compares actual classifier plus map across 64/128/256 logical slots using fixed synthetic mixed POSIX paths. Verify mapped, absent, extensionless, hidden, trailing-dot and multi-extension cases; no production capacity or ABI change, no speedup claim.

**Hash-map capacity comparison:** [lookup evidence](docs/performance/HASHMAP_BASELINE.md) compares 64/128/256 slots with identical fixed key sets: four shipped default mappings and a 64-key synthetic stress set. Record longest occupied cluster, logical backing bytes and verified lookup timings. No production capacity, hash, ownership or ABI change.

**Hash-map evaluation:** [bounded lookup baseline](docs/performance/HASHMAP_BASELINE.md) measures verified hits and misses at 25/50/75/100% synthetic occupancy. Preserve caller-owned storage, bounded probing, full-map updates and production behavior. Queue evaluation closed at four PRs; no external imports or performance claims without evidence.

**Queue affinity evaluation:** [queue baseline](docs/performance/QUEUE_BASELINE.md) adds optional test-process-only pinning of producer and consumer to two distinct allowed Linux logical CPUs, verifies resulting masks, rejects invalid requests, and leaves normal CI unpinned. Same feature budget; no production layout or memory-order change.

**Queue cache-layout experiment:** [queue evaluation](docs/performance/QUEUE_BASELINE.md) compares the existing ring with a test-only 64-byte-aligned index candidate compiled from the same source and workload. Report layout, memory cost, verified transfers and variability; retain production C atomics/layout until representative comparative evidence justifies a change.

**Concurrent queue evidence:** [queue baseline](docs/performance/QUEUE_BASELINE.md) adds a distinct two-thread, fixed-workload measurement with unique sequence and full payload verification, cooperative retry deadline and CTest timeout. Same QUEUE-UPSTREAM-EVAL-01 budget; no production layout, synchronization or performance claim change.

**Queue performance evidence:** [bounded in-memory baseline](docs/performance/QUEUE_BASELINE.md) measures the existing C atomic SPSC queue in three fixed-size batched samples, verifying payloads and FIFO ordering. Test-only C target, no production code/layout change, no time threshold or speedup claim. Linux circular-buffer reference informs evaluation; retain modulo and C atomics pending comparative evidence.

**Scanner special-file regression:** Linux inode-type reference confirms regular files, FIFOs and sockets are distinct. Existing descriptor-relative regular-file-only selection is retained; disposable FIFO and pathname Unix-socket fixtures exercise no callback handoff, repeated scans, file preservation and descriptor balance. No imported implementation, production change or performance claim. See [scanner security](docs/security/FILESYSTEM-SCANNER-SECURITY.md).

**Upstream audit:** [Reference-only module assessment](docs/upstream/UPSTREAM_MODULE_AUDIT.md) records existing native interfaces, candidate tradeoffs and deferred work. Retain current C atomic SPSC queue; full-ring recycling regression verifies rejection at capacity, FIFO ordering and complete event payload across wraparound. No imported implementation, performance claim or license clearance.

**Owner-approved upstream-first policy (2026-11-25):** Follow [reuse policy](docs/upstream/UPSTREAM_REUSE_POLICY.md) and [master plan](docs/upstream/UPSTREAM_MASTER_PLAN.md). Prefer supported Linux/POSIX APIs and mature compatible components; retain working code unless comparative evidence justifies change. External source imports and new third-party dependencies are BLOCKED pending explicit owner license decision and component acceptance. C17, C11/C23 CI, FS-01B/REL-01, no-spend and five-PR constraints remain binding.

> **Configuration input contract:** Physical config lines must reject embedded NUL bytes before string parsing; failed loads preserve caller state; loader requires a nonempty path and initialized power-of-two map capacity with size not exceeding capacity; extension names reject control bytes; rejected extension keys and destinations must not be echoed into diagnostics. Hash-map initialization and access reject impossible byte extent or size metadata; callers still own backing storage. Classification uses the last POSIX path component; leading-dot-only and trailing-dot names use the fallback. SPSC queue initialization is exclusive; its unit suite checks concurrent FIFO transfer across repeated wraparound. Worker move diagnostics escape nonprintable/non-ASCII bytes and backslashes under a stream lock; original filesystem paths are unchanged. Root/output validation reports fixed rejection reasons without echoing untrusted paths. Linux watch registration requires a directory and rejects final-component symlinks; ancestor path identity is not established. Shared dispatch rejects empty paths and worker counts outside 1..MAX_WORKERS. Watcher event headers are copied before decoding; actionable names must be nonempty basename components. Worker notification and shutdown reject out-of-range pool metadata before indexing fixed arrays. The process-wide worker storage has one owner; duplicate starts and wrong-context lifecycle operations are rejected under caller serialization. Watcher idle-poll metrics count actual poll timeouts; integration waits for observed readiness instead of a fixed startup sleep. Scanner cancellation is checked before root access and immediately before callback handoff; it remains cooperative. Scanner errors retain the first observed failure even when later cancellation or callback failure stops traversal. Linux scanner strips trailing root separators before no-follow opening, preserving the root slash; ancestor identity is not established. Linux watcher also trims trailing root separators before no-follow registration and builds event paths from the trimmed root. Scanner and watcher share the internal root-path copy helper; it trims only trailing slashes and performs no filesystem resolution. CLI root preflight uses the same lexical root-copy helper before its existing lstat/realpath checks. CLI output preflight, scanner and watcher use a shared checked lexical path-join helper; callers retain component validation and filesystem policy. Scanner path construction and frame growth failures stop discovery and unwind owned frames; deep-tree allocation fault regressions check descriptor balance. Scanner directory acquisition stops on EMFILE, ENFILE or ENOMEM and unwinds owned frames; deterministic child-open fault tests check attempts and descriptor balance. Scanner resource regressions cover directory-stream acquisition faults and a real child-process RLIMIT_NOFILE exhaustion test without changing the parent or system limit. BENCH-SCALE-01 adds a bounded native C scanner benchmark with callback counts, monotonic timing, CPU and process peak RSS; smoke checks do not establish large-scale capacity. Native scanner evidence has a bounded manual-only C17 Release workflow with source/binary and runner provenance; no automatic scale run or spending authorization. BENCH-SCALE-01 final budget slot adds semantic evidence validation before upload; no performance result or release acceptance is inferred. Malformed bare recognized configuration settings use the same C whitespace classification as key normalization, including vertical tab and form feed; rejection preserves caller configuration. Hash-map existing-key updates accept overlapping borrowed value strings, including a suffix, without undefined memcpy overlap. Classifier initialization ignores a NULL context; classification rejects NULL contexts and NULL/empty filenames without changing valid-path routing. SPSC queue APIs reject NULL queue/event arguments without transferring events; NULL initialization is a no-op and NULL emptiness queries return true. Valid initialized SPSC ordering and capacity are unchanged. Workers reject empty or unterminated event paths and NULL/empty classification results before diagnostics or moving; invalid work increments failures and stops intake with worker_error while existing queued work drains. Scanner entry and opened-directory metadata resource exhaustion terminates discovery and unwinds owned frames; entry and opened-directory lookup fault regressions verify traversal termination and descriptor balance, including nested failure and successful rescan. Hash-map insertion refuses an unused slot when the declared size is already at capacity; existing-key updates remain allowed at full capacity. This guard does not prove arbitrary metadata consistency. Configuration initialization treats NULL as a no-op and resets scalar defaults without changing caller-owned extension mappings; regression covers populated-map preservation. Watcher cancellation is checked between buffered records and before actionable path construction; a pending stop prevents new record handoff at those boundaries but cannot revoke in-flight work. Watcher terminal-record regression covers overflow and all four invalidation flags followed by an actionable record; the parser returns the terminal error without handing off subsequent records. CLI --workers is a unique value-bearing override, like --root and --config; repeating it (including identical counts) is rejected before configuration or filesystem access. Scanner directory read resource exhaustion terminates discovery and unwinds all owned frames; depth-targeted test-only faults verify termination, descriptor balance and successful rescan. Ordinary EOF and non-resource errors retain existing behavior. Worker pool lifecycle independently tracks the actual started count; in-range caller count mismatches reject notify/join before mutation under the existing serialized ownership contract. Worker shutdown regression injects a notification failure inside join, checks error propagation and complete metadata cleanup, then verifies restart. Configuration loading opens with nonblocking and close-on-exec flags, validates the opened object is a regular file before parsing, and preserves symlink-to-regular-file compatibility. FIFO and directory inputs fail; arbitrary devices or remote filesystem opens are not proven nonblocking. See [live board](next.md) for implementation and verification status. FS-01B/REL-01 remain blocked.

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
