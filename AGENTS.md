# Repository Engineering Rules

> **Configuration input contract:** Physical config lines must reject embedded NUL bytes before string parsing; failed loads preserve caller state; loader requires a nonempty path and initialized power-of-two map capacity with size not exceeding capacity; extension names reject control bytes; rejected extension keys and destinations must not be echoed into diagnostics. Hash-map initialization and access reject impossible byte extent or size metadata; callers still own backing storage. Classification uses the last POSIX path component; leading-dot-only and trailing-dot names use the fallback. SPSC queue initialization is exclusive; its unit suite checks concurrent FIFO transfer across repeated wraparound. Worker move diagnostics escape nonprintable/non-ASCII bytes and backslashes under a stream lock; original filesystem paths are unchanged. Root/output validation reports fixed rejection reasons without echoing untrusted paths. Linux watch registration requires a directory and rejects final-component symlinks; ancestor path identity is not established. Shared dispatch rejects empty paths and worker counts outside 1..MAX_WORKERS. Watcher event headers are copied before decoding; actionable names must be nonempty basename components. Worker notification and shutdown reject out-of-range pool metadata before indexing fixed arrays. The process-wide worker storage has one owner; duplicate starts and wrong-context lifecycle operations are rejected under caller serialization. Watcher idle-poll metrics count actual poll timeouts; integration waits for observed readiness instead of a fixed startup sleep. Scanner cancellation is checked before root access and immediately before callback handoff; it remains cooperative. Scanner errors retain the first observed failure even when later cancellation or callback failure stops traversal. Linux scanner strips trailing root separators before no-follow opening, preserving the root slash; ancestor identity is not established. Linux watcher also trims trailing root separators before no-follow registration and builds event paths from the trimmed root. Scanner and watcher share the internal root-path copy helper; it trims only trailing slashes and performs no filesystem resolution. CLI root preflight uses the same lexical root-copy helper before its existing lstat/realpath checks. CLI output preflight, scanner and watcher use a shared checked lexical path-join helper; callers retain component validation and filesystem policy. Scanner path construction and frame growth failures stop discovery and unwind owned frames; deep-tree allocation fault regressions check descriptor balance. Scanner directory acquisition stops on EMFILE, ENFILE or ENOMEM and unwinds owned frames; deterministic child-open fault tests check attempts and descriptor balance. Scanner resource regressions cover directory-stream acquisition faults and a real child-process RLIMIT_NOFILE exhaustion test without changing the parent or system limit. BENCH-SCALE-01 adds a bounded native C scanner benchmark with callback counts, monotonic timing, CPU and process peak RSS; smoke checks do not establish large-scale capacity. Native scanner evidence has a bounded manual-only C17 Release workflow with source/binary and runner provenance; no automatic scale run or spending authorization. BENCH-SCALE-01 final budget slot adds semantic evidence validation before upload; no performance result or release acceptance is inferred. Malformed bare recognized configuration settings use the same C whitespace classification as key normalization, including vertical tab and form feed; rejection preserves caller configuration. Hash-map existing-key updates accept overlapping borrowed value strings, including a suffix, without undefined memcpy overlap. Classifier initialization ignores a NULL context; classification rejects NULL contexts and NULL/empty filenames without changing valid-path routing. SPSC queue APIs reject NULL queue/event arguments without transferring events; NULL initialization is a no-op and NULL emptiness queries return true. Valid initialized SPSC ordering and capacity are unchanged. Workers reject empty or unterminated event paths and NULL/empty classification results before diagnostics or moving; invalid work increments failures and stops intake with worker_error while existing queued work drains. Scanner entry and opened-directory metadata resource exhaustion terminates discovery and unwinds owned frames; entry and opened-directory lookup fault regressions verify traversal termination and descriptor balance, including nested failure and successful rescan. Hash-map insertion refuses an unused slot when the declared size is already at capacity; existing-key updates remain allowed at full capacity. This guard does not prove arbitrary metadata consistency. Configuration initialization treats NULL as a no-op and resets scalar defaults without changing caller-owned extension mappings; regression covers populated-map preservation. Watcher cancellation is checked between buffered records and before actionable path construction; a pending stop prevents new record handoff at those boundaries but cannot revoke in-flight work. Watcher terminal-record regression covers overflow and all four invalidation flags followed by an actionable record; the parser returns the terminal error without handing off subsequent records. CLI --workers is a unique value-bearing override, like --root and --config; repeating it (including identical counts) is rejected before configuration or filesystem access. Scanner directory read resource exhaustion terminates discovery and unwinds all owned frames; depth-targeted test-only faults verify termination, descriptor balance and successful rescan. Ordinary EOF and non-resource errors retain existing behavior. Worker pool lifecycle independently tracks the actual started count; in-range caller count mismatches reject notify/join before mutation under the existing serialized ownership contract. See [live board](next.md) for implementation and verification status. FS-01B/REL-01 remain blocked.

> **Current owner contract (24 Nov 2026):** Every authorized contributor (human, ChatGPT, Codex, Claude Code or other agent) uses the same resume-first [handoff contract](docs/AUTOPILOT_EXECUTION_CONTINUATION.md), [live board](next.md) and [feature ledger](docs/feature-budget.json). Hourly and manual `go ahead` are additive. Reconcile all authors' PRs, finish existing eligible work before starting another, count all opened PRs toward one stable five-PR budget, and update affected docs in the same PR. Actual GitHub state supersedes historical milestones below. No fake approval, security bypass or release authority.

> **Execution recovery (23 Nov 2026):** use the [hourly continuation and stall recovery](docs/AUTOPILOT_EXECUTION_CONTINUATION.md) for checkpoints, resume-first task selection, two-no-change escalation, and proportional L1–L7/CISSP/CCIE/Linux/SRE/CA-MBA review. This is a repository procedure, **not a running scheduler**; current GitHub and actual ChatGPT task history supersede dated snapshots.

> Owner directive (23 Nov 2026): **no required human approving review** for routine eligible autopilot PRs. Conduct documented agent self-review; enforce current-head CI, thread resolution, main rules and FS-01B P0 boundaries. [Authoritative policy](docs/AUTOPILOT_ZERO_REVIEW_AND_DOC_SYNC.md) replaces contrary older independent-approval requirements; [ruleset JSON](.github/rulesets/autopilot-main-no-human-review.json) must be imported separately.

This file is the working contract for every human and automated contributor.

## Owner feature-budget directive (2026-11-21)

Follow [five-PR feature policy](docs/FEATURE_DELIVERY_AND_ACCEPTANCE_POLICY.md), [machine-checked ledger](docs/feature-budget.json), and the compact [NOW board](next.md). **Maximum five OPENED feature PR attempts per stable feature ID**, counting rejected/closed/merged attempts; revisions to a single PR do not consume another slot. Exit earlier when verified, or after five document DONE/PARTIAL/BLOCKED and outstanding safety/manual gaps and advance to a different independent feature. **PERF-01 is CLOSED_LEGACY through merged #128**; no sixth/unlimited PERF-01 fixture-hardening spin-offs. **PLAN-REPORT-01 PARTIAL:** PR #145 merged at `ac9fc87a91a665e2d0f09a3af79ee7f7ca310139` with aggregate-only JSON, not a per-file plan/conflict inventory. **CONFIG-MODE-01 DONE:** PR #146 merged at `d7adc17afa4c015f95ad016bb3817c133a682435` with four exact-head workflow families PASS. **CONFIG-NUMERIC-01 DONE:** PR #147 merged at `d839ad608c554d0c7715dc6264be6952ef07eccf` (four exact-head workflow families PASS). **CONFIG-LINE-01 DONE:** PR #148 merged at `3b32efbe8fad2047f3588fa51c907525a4e4dca4` (four PR workflow families PASS). **CONFIG-EXT-TRAIL-01 DONE:** #150 merged at `74c0639e9656eb37339e96cdc9ed7d312682b3f0` (exact-head CI not independently verified this run). CONFIG-MODE-MISSING-01 is historical; use the live board. PR #143's offline task-selector CI guard is MERGED at `80de73b465ede49eec0882846c50efc0cbdee254`; FS-01B P0 remains blocked. `Feature-ID: ...` metadata and actual PR number registration are required by CI. Emergency file-preservation work must remain a separately justified safety interruption, not bypass the budget. No real source retirement without accepted FS-01B design and positive preservation. CI success is not owner-run Ubuntu/Compose or production release acceptance.

## Milestone documentation with implementation (owner-directed, 2026-11-20)

Borrow IMGENGINE's **compact living work board plus historical archive** and per-PR documentation-impact transaction, adapted to this native **C17/POSIX** project: [current NOW/NEXT board](next.md), [dated archive](next-history.md), [documentation transaction](docs/MILESTONE_DOCUMENTATION_WORKFLOW.md), and [PR checklist](.github/pull_request_template.md). Each meaningful code/test/CI milestone PR must update current `next.md` scope/status/evidence and the affected README, scoped AGENTS/skills, HLD/LLD, security, journal, Docker, perf or business docs **only when facts change**, explicitly recording UPDATED or N/A in the PR. Do not maintain parallel live queues or open repetitive docs-only PRs after every merge; after actual merge, reconcile main/merge SHA in the next authorized implementation PR. Historical labels do not outweigh live GitHub. C17 production, Rust test-only, same-UID source-retirement P0 and release limits remain unchanged. A Markdown checklist is not an autonomous scheduler or GitHub CI check.

## Advanced feature and product roadmap authority (2026-11-22)

The owner's future-feature candidates, staged dependencies and scope are in [advanced roadmap](docs/ADVANCED_FEATURE_ROADMAP.md), with [acceptance matrix](docs/ADVANCED_FEATURE_ACCEPTANCE.md) applying L1–L7 engineering, security/CISSP, Linux, proportionate networking/CCIE, SRE, CA, MBA and management lenses. Use [product strategy](docs/PRODUCT_AND_BUSINESS_STRATEGY.md), [FS-01B ADR](docs/architecture/ADR_FS_01B_SOURCE_RETIREMENT_BOUNDARY.md) and [API/ABI/FFI roadmap](docs/architecture/C-API-ABI-FFI-ROADMAP.md) for their authoritative domains. These docs are **DESIGN_ONLY**; `next.md` and live GitHub select actual authorized work, and `docs/feature-budget.json` owns active feature ID/budget. New feature selection requires explicit dependencies, acceptance, a reviewable bounded scope and a unique stable ID; never convert every roadmap row into a parallel open PR. No automatic duplicate deletion, production source retirement, remote upload, production Rust FFI, SDK release, paid resource or desktop deploy is authorized by a roadmap. Existing P0 FS-01B and REL-01 remain blocked until their approvals and positive safety evidence.

## Project operating dashboard and agent continuation

- Start every new task by reading the [living work register](next.md), [documentation authority map](docs/README.md), [seven-level engineering and business operating model](docs/ENGINEERING_OPERATING_MODEL.md), [CI/CD and Makefile operations](docs/CI_CD_AND_MAKEFILE_OPERATIONS.md), and [agent continuation/handoff](docs/AGENT_CONTINUATION_AND_HANDOFF.md), then the affected code and security contracts. These documents supplement, never override, the safety invariants here.
- The **top LIVE NOW / NEXT / LATER board** of `next.md` is the canonical cross-milestone status/dependency selector. The historical section is audit evidence, **not** a current queue; [component completion inventory](docs/PROJECT_COMPLETION_AND_STATUS_INVENTORY.md) separates built vs partial vs test-only vs production/release. Confirm current main/PR/check SHAs before treating a dated entry as current. A design, implementation, checked PR, merged PR and released binary are distinct.
- After a milestone, checkpoint exact SHA, reviewed diff, actual test/CI outcomes, blockers, risk, next READY item and owner decision. Use one bounded PR per milestone; never start its dependent implementation before the earlier PR is merged and verified.
- A running or scheduled coding agent may follow the documented next-task-selection procedure in its authorized session. Markdown alone does **not** start an automation, grant permissions, or authorize release, destructive user-file operations or spending. The owner's **separate, explicit, standing PR review/merge authorization** is recorded in [GOV-02](docs/AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md); it never bypasses branch protections or safety checks.
- Apply the seven engineering levels as proportional **review perspectives**, not seven automatic code-changing agents. Security, SRE, CTO and CA/MBA review should scale with risk and data-loss consequences.
- Keep heavy passing test logs in CI/artifacts; report exact SHA + job conclusions, retrieve only bounded failures. Change-scoped local tests never waive existing required PR/CI gates.

## Standing owner shorthand: go ahead = next eligible next.md work

When the owner says **"go ahead", "go ahead next", "continue" or "next milestone"** in the context of this repository, **do not ask them to restate the task list**. In the currently authorized agent session, first fetch actual GitHub `main`, open PRs, merged SHAs and required checks; read the **LIVE NOW / NEXT / LATER board** at the top of `next.md`; reconcile any stale labels; and select the **highest-priority dependency-unblocked bounded work item**. If the highest priority is blocked, state the blocker and choose a separate independent READY item. If an existing PR already covers the item, check its latest head CI and diagnose failures rather than repeating code. Create only the smallest justified reviewable change/PR, checkpoint the exact evidence and next action, and report what is implemented versus still open.

**Authority limits:** The bare phrase **"go ahead"** selects the next task; it is not by itself a blanket permission for dangerous operations. **Separately, on 2026-11-17, the owner explicitly authorized the existing Messy-File-Cleaner hourly autopilot to review and merge eligible PRs** with exact-head CI, review and protection gates. See [GOV-02](docs/AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md) for scoped merge authority and its limits. This does **not** authorize deleting customer files, destructive recovery, production releases, paid infrastructure, secrets/protection changes, history rewriting, or new production FCJ3/FCJ4/FFI without task-specific decisions. Markdown cannot run an agent on its own.

## Dual-trigger hourly autopilot (external scheduler)

The existing **Messy-File-Cleaner Autopilot** runs on an independent hourly ChatGPT schedule while enabled. The owner's manual `go ahead` is an immediate trigger **in addition** to that schedule, never a reason to pause or postpone it. User inactivity does not block authorized work. Each run must attempt one bounded material engineering advancement: resume an open PR and repair CI/reviews, merge a verified eligible PR, or implement source/tests/CI in a new reviewable PR. A successful scheduled invocation alone is not engineering progress. If checkout fails, try the GitHub connector; do not claim local tests ran. Escalate a repeated no-change blocker after two attempts and choose an independent READY safe task. Preserve C17 production, C11/C23 compatibility, test-only Rust isolation and all P0 filesystem safety restrictions. Report exact SHAs, checks, files, blockers, and next executable job. [GOV-02](docs/AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md) is the detailed dual-trigger and merge-gate authority. Repository text does not guarantee platform execution or authorize releases.

## Approved narrow Rust test-tooling exception

- The owner merged [Rust adoption ADR](docs/architecture/ADR_RUST_ADOPTION_STRATEGY.md) in PR #81 and the isolated test-support scaffold in PR #82. The **production engine remains pure C17**. The repository now contains Rust **test support only** under `tests/rust/`, with no production FFI, library or runtime dependency.
- Where this file or `skills.md` says “pure C,” interpret it as the production core/production native targets. It does not prohibit the **approved, isolated Rust test-support pilot**. No extra language may enter the production core without a separate approved architecture/security decision.
- No `.a`/`.so` product library, public ABI or production FCJ4 recovery is authorized by test-tooling acceptance. [Artifact policy](docs/architecture/RUST_TEST_SUPPORT_BOUNDARY.md) remains binding.
- `skills.md` is the operational recipe; uppercase `SKILLS.md` is a skill catalogue, not a competing agent authority.

## Mission and priorities
Build a dependable file-management engine that is safe with user data. Priorities: correctness and preservation of files; security and least privilege; recoverability and observability; measured performance; maintainability and delivery speed. Never trade data safety for a benchmark result. No performance claim without a reproducible benchmark and recorded environment.

## Supported development environment
- Ubuntu 24.04 LTS is the canonical Codespaces and container-development baseline.
- Use the repository Dev Container definition in `.devcontainer/`; do not install toolchains ad hoc into an unrelated base image.
- The development image must provide GCC, Clang, CMake, Ninja, Python 3, Git, and static-analysis/debugging tools.
- The production image is a separate multi-stage image based on Ubuntu 24.04 LTS. Keep build dependencies out of its runtime stage.
- Keep compiler, C standard, and build type explicit in validation commands. Record OS/image, compiler version, CMake version, flags, and container limits with benchmark results.
- Do not assume that Arch Linux, Debian, or host-installed tools are equivalent to the canonical development image.

## Makefile developer workflow and command safety
- Treat the repository `Makefile` as the primary documented local developer interface. Inspect `make help` and use its targets before inventing ad-hoc build/test commands; use direct CMake commands only when a needed target is not provided or when diagnosing the build system itself.
- Use `make` for the default Debug + ASan/UBSan build, `make test` for the Debug CTest suite, and `make validate` for isolated C11/C17/C23 Debug build-and-test runs.
- Keep build parallelism bounded. The Makefile defaults to `JOBS=2`; raise it only after checking available CPU and memory.
- `make run` must remain a dry-run. Never invoke `make run-execute` unless the owner explicitly requests real file operations and the input root/configuration have been reviewed; the target requires `CONFIRM_MOVE=YES` as an additional guard.
- Treat `make benchmark-moves` as a destructive test of its isolated temporary fixtures. Inspect the harness before changing its fixture/output behavior; never direct it at user data.
- `make clean` is limited to build/profiling artifacts. Do not broaden its deletion scope without explicit review and isolated tests.
- The Makefile is an interface, not a security boundary. Continue validating paths and options in the C application itself; quoting in shell recipes does not replace input validation.

## Architecture
- Keep the first product as a modular C application. Do not introduce microservices, brokers, databases, or a web server without a demonstrated requirement.
- Keep CLI, configuration, scanning, classification, queueing, moving, journaling, and platform filesystem operations behind clear module boundaries.
- Use Hexagonal Architecture selectively for filesystem, clock, logging, and persistence boundaries where tests benefit.
- Use patterns only to solve a real problem; avoid speculative factories, singletons, and abstractions.
- Document thread safety, ownership, lifetime, error, and cancellation contracts.
- The current lock-free queue must only be used with its proven producer/consumer topology. Never silently change SPSC assumptions to MPSC.

## C standards
- C17 is the default project language baseline for production builds.
- C11 compatibility is tested because the implementation relies on C11 atomics and should remain usable with mature toolchains.
- C23 is an optional compatibility target, not a requirement for users or the production release until supported compilers are validated.
- Select the standard explicitly with `-DFC_C_STANDARD=11`, `17`, or `23`; CI must build and test all three independently.
- Do not add C23-only syntax to core modules unless a documented need outweighs the portability cost.
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
- Configured output folders must be relative single components; reject absolute paths, dot-dot, nested separators, and the reserved journal directory.
- Canonicalize the input root and reject overlap with configured outputs, fallback output, and journal paths.
- Reject symlink input roots and symlink/non-directory output destinations; --force must not bypass these invariants.
- Never delete source until destination is fully written and safely published.
- Use exclusive creation; never truncate an existing user file.
- Journal intent and completion. Recovery must be idempotent and must not guess when state is ambiguous.
- Force must not bypass path validation or filesystem invariants.
- Destructive tests must use isolated temporary directories and assert source preservation on failures.

## FCJ4 journal and recovery safety gates

- FCJ4 is a separate future protocol; never append FCJ4 records to the FCJ2/legacy `journal/move.log` and never mix FCJ3 with FCJ2/legacy.
- The FCJ4 parser/replay must remain pure, whole-buffer, all-or-nothing, and read-only. Replay validity is not authenticity, filesystem truth, or authorization to recover.
- Keep FCJ4 parser, serializer, and in-memory writer-state modules isolated from the production executable and CLI until a separately reviewed integration milestone is approved.
- Writer prepare/commit is in-memory state management only. Never describe it as a durable journal commit.
- The approved Linux-only FCJ4 test adapter may use a caller-supplied descriptor only for a disposable private temporary file. It must remain in the test-support tree and a Linux-only test target, use explicit-offset `pwrite()`, reject `O_APPEND`, validate identity/size/ownership/mode/link count, acquire a non-blocking advisory lock, and require the fsync → writer-commit acknowledgement handshake.
- The test adapter must never open/create `journal/move.fcj4`, enter the production source list/executable, or be exposed through the CLI. Advisory locking does not stop non-cooperating writers; process-exit tests do not prove power-loss durability.
- No production FCJ4 descriptor, lock, journal creation, append/write, fsync/close adapter, planner, executor, quarantine, publication, source retirement, deletion, or recovery mutation is authorized by the test-adapter design or implementation.
- Recovery action lifecycle is independent from move lifecycle. Move completion must never imply action completion.
- Never unlink or retire a source until a separately approved protocol and execution design proves durable recovery evidence and exact filesystem identity checks.

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
- Treat nanoseconds as a measurement unit, not a promise that filesystem operations complete in nanoseconds.
- Target nanosecond-level latency only for suitable CPU-bound operations (for example, in-memory classification or hashing) when repeatable measurements support it.
- For filesystem operations, report realistic distributions and throughput; separate metadata-only rename from cross-filesystem copy, cold-cache traversal, and durable journal writes.
- Use monotonic clocks, batched microbenchmarks, repeated runs, and median/tail percentiles. Account for timer overhead and measurement noise; never optimize from a single minimum sample.

## API, ABI, and FFI
- Keep the public C API small, versioned, and independent of internal structs.
- Prefer opaque handles and fixed-width integer types.
- Define ownership and matching release functions.
- Define ABI version negotiation and size/version fields for extensible structs.
- Never expose pthread types, FILE pointers, internal queues, or platform details in the stable ABI.
- Rust FFI must use the C ABI, repr(C) where required, explicit pointer/length contracts, and documented safety invariants.
- The accepted Rust test-support crate in `tests/rust/` is the sole approved exception to C-only development. Any **new** Rust component, C/Rust FFI connection, or production use requires its applicable staged ADR review, concrete test consumer or measured safety benefit, and explicit scope approval.
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

## Milestone completion and merge governance

Every milestone is an independently reviewable delivery unit with written scope and acceptance criteria.

1. Work on a dedicated milestone branch created from the latest `main`.
2. Before implementation, record the objective, scope, risks, security impact, and measurable acceptance criteria.
3. A milestone passes only when its required tests, security checks, documentation, and acceptance criteria pass. A successful compile alone is not a pass.
4. Record exact commands, CI run links/results, test outcomes, and known limitations in the pull request.
5. After the milestone passes, review the **exact PR diff, comments, unresolved threads, relevant tests and current-head CI**. If working under the owner's **active GOV-02 standing autopilot merge authorization**, merge only eligible, correctly scoped, non-blocked PRs in compliance with [merge policy](docs/AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md) and GitHub protections, using the expected head SHA. Otherwise **request specific owner approval**.
6. Verify the merge commit and rerun/inspect required checks on `main`.
7. Start the next milestone from the updated `main`; do not stack the next milestone on an unmerged branch.
8. If any gate fails, keep work within that milestone, fix it, and rerun the gates. Do not declare it complete or advance.
9. Never merge a failing, unreviewed, or unverified milestone just to maintain schedule. If GitHub permissions or branch protections prevent the merge, report the blocker and ask the repository owner to resolve it.

The repository owner expects eligible, reviewed milestones to reach `main` before their dependents begin. **Specific PR approval OR the owner's active GOV-02 standing authorization** is required for merge; the latter is not permission to bypass P0 data-safety/CI/design gates. Never claim a merge until GitHub confirms the exact resulting commit.

## Product and management
Prioritize customer value, data safety, reliability, support cost, and total cost of ownership. Build the smallest reliable product first. Add GUI, API server, cloud, and distributed processing only when customer demand justifies them.


## Mandatory production language: pure C17; isolated Rust test support

The project owner has made a firm product and engineering decision: **the production engine and shipped native targets are pure C, not C++**. The accepted Rust ADR permits a narrow **test-only** Rust-support crate; it does not authorize a Rust production core.

- Implement the application, core modules, production tests/tools and shipped native build targets in C; the approved isolated Rust verification harness under `tests/rust/` is the only present exception.
- Do not introduce C++ source files or C++ implementation: no `.cpp`, `.cc`, or `.cxx` files; no C++ classes, templates, exceptions, STL, or C++ runtime dependency.
- Do not enable CXX in CMake or add C++ compiler/build targets.
- Use C17 as the default production language baseline. Preserve C11 compatibility where practical and validate C23 as an optional compatibility target; do not let C23-only features silently raise the minimum compiler requirement.
- Use portable, well-defined C and documented POSIX/Linux interfaces. Keep platform-specific code behind narrow C module boundaries.
- If interoperability is ever required, expose a documented, versioned C ABI with explicit ownership, lifetime, error, and thread-safety contracts. A C ABI does not authorize rewriting the core in C++.
- Assembly may be considered only for a measured, CPU-bound bottleneck. Keep it isolated behind a C interface, provide a tested portable C fallback, and verify correctness and security implications. Never use assembly speculatively.
- Do not introduce Rust, C++, or another implementation language into the core without an explicit new decision from the project owner and a documented architecture/security review.

This policy overrides generic examples or recommendations that suggest C++ as an option. When in doubt, choose C.


## Mandatory engineering playbook and skills

The detailed, enforceable engineering standards live in:
- `skills.md` — day-to-day contributor and coding-agent workflow, review checklist, and implementation recipes.
- `SKILLS.md` — non-binding knowledge catalogue; it does not override `AGENTS.md`, the accepted ADRs or `skills.md`.
- `next.md` — living milestone status, sequencing and evidence board; it is not itself approval for a risky operation.
- `docs/architecture/ENGINEERING_PLAYBOOK.md` — HLD/LLD, module design, architecture styles, design patterns, SOLID/DRY/KISS, testing, and documentation rules.
- `docs/security/THREAT_MODEL.md` — assets, trust boundaries, attacker model, filesystem threats, controls, and security acceptance gates.

These documents are normative together with this file. If guidance conflicts, apply this precedence:
1. Explicit current project-owner decisions and accepted ADRs (C17 production, isolated Rust test-only pilot), without weakening mandatory data-preservation rules.
2. Safety and security invariants in this file and the threat model.
3. Public API/ABI and journal-format compatibility contracts.
4. Architecture playbook and skills.
5. Local implementation convenience.

### Design and implementation rules
- Apply SOLID as a design aid, not as a requirement to create one-file-per-function abstractions. In C, use cohesive modules, opaque context structs where useful, narrow headers, and explicit function contracts.
- Apply DRY to duplicated knowledge and policy, not merely repeated syntax. Do not create premature generic frameworks.
- Prefer KISS and YAGNI. Every abstraction, dependency, thread, cache, state, and configuration option must have a documented reason.
- Use DDD selectively: name core concepts consistently (scan, classification, move operation, journal transaction, recovery); keep domain decisions independent from OS calls where practical. Do not create artificial aggregates/repositories for simple local data.
- Use Hexagonal Architecture at real boundaries (filesystem, journal persistence, clock, logging, randomness, and test fault injection). Keep adapters replaceable in tests; do not build a framework around every call.
- Use pipeline/producer-consumer for bounded work flow. Preserve documented queue topology and ownership transfer.
- Treat journal processing as a durable transaction state machine. Journal records are not a distributed event bus. Do not claim full Event-Driven Architecture unless independent producers/consumers communicate through an explicit event contract and delivery semantics.
- Design state transitions explicitly: allowed predecessor, next state, immutable transaction identity, idempotency, failure semantics, and crash points must be documented and tested.
- Keep platform-specific Linux/POSIX behavior behind narrow C interfaces and document unsupported filesystems/platforms.

### Required change discipline
- Before changing a module, read its public header, implementation, callers, tests, and relevant design docs.
- Before changing persistent formats, public APIs, concurrency, filesystem mutation, or recovery, write/update an ADR or design document and obtain review before implementation.
- For every behavior change, update focused unit/integration/fault-injection tests and user/operator documentation.
- Never weaken an invariant to make a test pass. Fix the test only when its assumption is demonstrably wrong and explain why.
- Do not use `assert()` for expressions with side effects. Tests must remain meaningful in Release/NDEBUG builds.
- Keep production behavior independent from test-only hooks. Compile test fault injection only under the designated test configuration.
- Preserve errno and original failure context across cleanup paths. Cleanup failures must be reported without hiding the primary failure.
- Use bounded inputs, checked arithmetic, explicit integer conversions, and allocation overflow checks.
- No silent fallback to a weaker security or durability guarantee. Unsupported capability means an explicit safe failure or documented non-destructive mode.

### Required verification and reporting
- Run the narrowest relevant test first, then `make test`, then `make validate` for changes that can affect core behavior.
- Run static analysis, sanitizer, Docker/non-root, and crash/fault-injection checks according to the risk of the change.
- For journal, mover, recovery, path, or deletion changes, require adversarial filesystem tests and crash-boundary tests; a normal happy-path test is insufficient.
- Report exact commands and observed results. Separate tests passed, tests not run, known limitations, and inferred risks.
- A benchmark is evidence only for the measured workload and environment. Never generalize one filesystem, one machine, or one run into a universal performance claim.
- Do not modify or delete user data during development. Destructive tests must create and verify disposable fixtures.


CONFIG-EXT-MISSING-01: bare or empty ext.* destination mappings are rejected; PR pending CI. CONFIG-MODE-MISSING-01 merged in #152 at `aba6200dd4d6ecaa6306d720e81e733c6e6e5af3`. Manual acceptance NOT RUN; FS-01B/REL-01 blocked.


## 23 Nov 2026 bounded config-read handoff

CONFIG-READ-ERROR-01: fail closed on `ferror`/`fclose` in configuration parsing; disposable Linux directory-read negative regression. PR exact-head CI and merge are pending. No customer files, source retirement, release, production Rust, or deployment authorized. CONFIG-EXT-KEY-01 #154 merged at `5b858c90c4c63b4a978f6fcaf9e159de5c4e02a1`; manual acceptance NOT RUN. Other architecture/ABI/FCJ/security/Docker/performance/business documents N/A: no interfaces, filesystem mutation, operational or financial behavior changed.


> **CONFIG-ATOMIC-LOAD-01 (23 Nov 2026, proposed PR):** C17 configuration loading now stages scalar settings and caller-owned extension map entries, committing only after successful parse/read/close. Failed loads retain prior caller state; disposable late-failure regression added. This is not a release or filesystem-retirement safety approval. Exact-head CI and manual acceptance remain NOT RUN pending PR verification. FS-01B and REL-01 remain BLOCKED.

> **CONFIG-KEY-WIDTH-01 (24 Nov 2026, proposed PR):** Reject overlong config keys before bounded sscanf parsing; disposable regression and same-PR feature-ledger reconciliation. C17 production; C11/C23 compatibility requires CI verification. No source retirement, deployment, release or ABI change. FS-01B/REL-01 BLOCKED; manual acceptance NOT RUN.
