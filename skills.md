# Engineering Skills and Contributor Workflow

**Upstream audit:** [Reference-only module assessment](docs/upstream/UPSTREAM_MODULE_AUDIT.md) records existing native interfaces, candidate tradeoffs and deferred work. Retain current C atomic SPSC queue; full-ring recycling regression verifies rejection at capacity, FIFO ordering and complete event payload across wraparound. No imported implementation, performance claim or license clearance.

**Owner-approved upstream-first policy (2026-11-25):** Follow [reuse policy](docs/upstream/UPSTREAM_REUSE_POLICY.md) and [master plan](docs/upstream/UPSTREAM_MASTER_PLAN.md). Prefer supported Linux/POSIX APIs and mature compatible components; retain working code unless comparative evidence justifies change. External source imports and new third-party dependencies are BLOCKED pending explicit owner license decision and component acceptance. C17, C11/C23 CI, FS-01B/REL-01, no-spend and five-PR constraints remain binding.

> **Configuration input contract:** Physical config lines must reject embedded NUL bytes before string parsing; failed loads preserve caller state; loader requires a nonempty path and initialized power-of-two map capacity with size not exceeding capacity; extension names reject control bytes; rejected extension keys and destinations must not be echoed into diagnostics. Hash-map initialization and access reject impossible byte extent or size metadata; callers still own backing storage. Classification uses the last POSIX path component; leading-dot-only and trailing-dot names use the fallback. SPSC queue initialization is exclusive; its unit suite checks concurrent FIFO transfer across repeated wraparound. Worker move diagnostics escape nonprintable/non-ASCII bytes and backslashes under a stream lock; original filesystem paths are unchanged. Root/output validation reports fixed rejection reasons without echoing untrusted paths. Linux watch registration requires a directory and rejects final-component symlinks; ancestor path identity is not established. Shared dispatch rejects empty paths and worker counts outside 1..MAX_WORKERS. Watcher event headers are copied before decoding; actionable names must be nonempty basename components. Worker notification and shutdown reject out-of-range pool metadata before indexing fixed arrays. The process-wide worker storage has one owner; duplicate starts and wrong-context lifecycle operations are rejected under caller serialization. Watcher idle-poll metrics count actual poll timeouts; integration waits for observed readiness instead of a fixed startup sleep. Scanner cancellation is checked before root access and immediately before callback handoff; it remains cooperative. Scanner errors retain the first observed failure even when later cancellation or callback failure stops traversal. Linux scanner strips trailing root separators before no-follow opening, preserving the root slash; ancestor identity is not established. Linux watcher also trims trailing root separators before no-follow registration and builds event paths from the trimmed root. Scanner and watcher share the internal root-path copy helper; it trims only trailing slashes and performs no filesystem resolution. CLI root preflight uses the same lexical root-copy helper before its existing lstat/realpath checks. CLI output preflight, scanner and watcher use a shared checked lexical path-join helper; callers retain component validation and filesystem policy. Scanner path construction and frame growth failures stop discovery and unwind owned frames; deep-tree allocation fault regressions check descriptor balance. Scanner directory acquisition stops on EMFILE, ENFILE or ENOMEM and unwinds owned frames; deterministic child-open fault tests check attempts and descriptor balance. Scanner resource regressions cover directory-stream acquisition faults and a real child-process RLIMIT_NOFILE exhaustion test without changing the parent or system limit. BENCH-SCALE-01 adds a bounded native C scanner benchmark with callback counts, monotonic timing, CPU and process peak RSS; smoke checks do not establish large-scale capacity. Native scanner evidence has a bounded manual-only C17 Release workflow with source/binary and runner provenance; no automatic scale run or spending authorization. BENCH-SCALE-01 final budget slot adds semantic evidence validation before upload; no performance result or release acceptance is inferred. Malformed bare recognized configuration settings use the same C whitespace classification as key normalization, including vertical tab and form feed; rejection preserves caller configuration. Hash-map existing-key updates accept overlapping borrowed value strings, including a suffix, without undefined memcpy overlap. Classifier initialization ignores a NULL context; classification rejects NULL contexts and NULL/empty filenames without changing valid-path routing. SPSC queue APIs reject NULL queue/event arguments without transferring events; NULL initialization is a no-op and NULL emptiness queries return true. Valid initialized SPSC ordering and capacity are unchanged. Workers reject empty or unterminated event paths and NULL/empty classification results before diagnostics or moving; invalid work increments failures and stops intake with worker_error while existing queued work drains. Scanner entry and opened-directory metadata resource exhaustion terminates discovery and unwinds owned frames; entry and opened-directory lookup fault regressions verify traversal termination and descriptor balance, including nested failure and successful rescan. Hash-map insertion refuses an unused slot when the declared size is already at capacity; existing-key updates remain allowed at full capacity. This guard does not prove arbitrary metadata consistency. Configuration initialization treats NULL as a no-op and resets scalar defaults without changing caller-owned extension mappings; regression covers populated-map preservation. Watcher cancellation is checked between buffered records and before actionable path construction; a pending stop prevents new record handoff at those boundaries but cannot revoke in-flight work. Watcher terminal-record regression covers overflow and all four invalidation flags followed by an actionable record; the parser returns the terminal error without handing off subsequent records. CLI --workers is a unique value-bearing override, like --root and --config; repeating it (including identical counts) is rejected before configuration or filesystem access. Scanner directory read resource exhaustion terminates discovery and unwinds all owned frames; depth-targeted test-only faults verify termination, descriptor balance and successful rescan. Ordinary EOF and non-resource errors retain existing behavior. Worker pool lifecycle independently tracks the actual started count; in-range caller count mismatches reject notify/join before mutation under the existing serialized ownership contract. Worker shutdown regression injects a notification failure inside join, checks error propagation and complete metadata cleanup, then verifies restart. Configuration loading opens with nonblocking and close-on-exec flags, validates the opened object is a regular file before parsing, and preserves symlink-to-regular-file compatibility. FIFO and directory inputs fail; arbitrary devices or remote filesystem opens are not proven nonblocking. See [live board](next.md) for implementation and verification status. FS-01B/REL-01 remain blocked.

> **Current owner contract (24 Nov 2026):** Every authorized contributor (human, ChatGPT, Codex, Claude Code or other agent) uses the same resume-first [handoff contract](docs/AUTOPILOT_EXECUTION_CONTINUATION.md), [live board](next.md) and [feature ledger](docs/feature-budget.json). Hourly and manual `go ahead` are additive. Reconcile all authors' PRs, finish existing eligible work before starting another, count all opened PRs toward one stable five-PR budget, and update affected docs in the same PR. Actual GitHub state supersedes historical milestones below. No fake approval, security bypass or release authority.

> **Execution recovery (23 Nov 2026):** use the [hourly continuation and stall recovery](docs/AUTOPILOT_EXECUTION_CONTINUATION.md) for checkpoints, resume-first task selection, two-no-change escalation, and proportional L1–L7/CISSP/CCIE/Linux/SRE/CA-MBA review. This is a repository procedure, **not a running scheduler**; current GitHub and actual ChatGPT task history supersede dated snapshots.

> **Current PR workflow:** the [zero-human-review owner policy](docs/AUTOPILOT_ZERO_REVIEW_AND_DOC_SYNC.md) authorizes autopilot self-review and gated merges without human approval; never present the self-review as external. Inspect current GitHub before using dated references below. Import [ruleset JSON](.github/rulesets/autopilot-main-no-human-review.json) separately.

This file is an operational companion to [AGENTS.md](AGENTS.md). It tells humans and coding agents how to work in this repository. The architecture and security rationale is in [the Engineering Playbook](docs/architecture/ENGINEERING_PLAYBOOK.md) and [Threat Model](docs/security/THREAT_MODEL.md).

## Five-PR execution and handoff recipe

For every new feature PR, select the stable active ID in [docs/feature-budget.json](docs/feature-budget.json), put `Feature-ID: ID` in the PR body, open one focused PR, then **add GitHub's actual assigned PR number to the ledger in that same PR**. Local CI validates five slots and the PR association. Count closed/failed PRs; never reset by renaming tasks. End after up to five, document DONE/PARTIAL/BLOCKED with exact head/main SHA, manual NOT RUN and remaining FS/data-preservation risks, and switch to the next independent READY feature. Historical PLAN-REPORT-01 is PARTIAL; select the current task from the live board. Do not enable production moves or treat reproduced unsafe unlink as fixed. Full rules: [feature policy](docs/FEATURE_DELIVERY_AND_ACCEPTANCE_POLICY.md).

## Advanced-feature intake checklist

Consult the [advanced roadmap](docs/ADVANCED_FEATURE_ROADMAP.md) and its [acceptance matrix](docs/ADVANCED_FEATURE_ACCEPTANCE.md) **after** live GitHub, [next.md](next.md) and the [feature ledger](docs/feature-budget.json). If the live board selects a read-only preview gap: inventory actual dry-run output, write a bounded schema/negative-case plan, require positive source/destination/journal non-mutation assertions and protect untrusted filename output. Do not start blocked P0 mover/undo/recovery changes or future C API/Rust FFI/desktop work without accepted design, real consumer or owner approval. Apply L1–L7, CISSP, Linux/SRE, proportionate CCIE, CA/MBA/management review with measurable evidence, no marketing claims. When a candidate becomes active, keep one stable ID and <=5 opened feature PRs, record actual PR number in same PR and update affected docs with UPDATED/N/A. The roadmap is not a second NOW queue.

## Project-specific documentation-impact recipe (2026-11-20)

Start with the compact [live work board](next.md) and live GitHub status; use [next-history.md](next-history.md) only as a dated archive. For every meaningful change, keep documentation in the **same implementation PR**: identify current milestone, tested SHA, PR/CI evidence and next READY task in `next.md`; review AGENTS, README, this contributor recipe, the docs index and the relevant FS-01/FCJ3/FCJ4, HLD/LLD, CMake/Makefile, Docker, C11/C17/C23, Rust test-only and benchmark contracts. List UPDATED or N/A and why in the [PR template](.github/pull_request_template.md). Do not duplicate live status throughout docs, create status-only PRs after every merge, or claim a green test fixes the same-UID source-retirement P0. [Canonical transaction](docs/MILESTONE_DOCUMENTATION_WORKFLOW.md).

## Always-on task selection and evidence recipe

1. Open the **LIVE NOW / NEXT / LATER board** in [next.md](next.md), the [component completion inventory](docs/PROJECT_COMPLETION_AND_STATUS_INVENTORY.md) and [docs index](docs/README.md). Confirm actual `main` SHA, outstanding PRs and CI before choosing the next READY milestone. Retained historical `next.md` notes must never override the live board.
2. Read [agent continuation](docs/AGENT_CONTINUATION_AND_HANDOFF.md), [engineering operating model](docs/ENGINEERING_OPERATING_MODEL.md), [CI/Makefile operations](docs/CI_CD_AND_MAKEFILE_OPERATIONS.md), and the affected module's specific contracts/tests.
3. Choose **one** bounded dependency-unblocked task. Define acceptance, negative cases, filesystem and data preservation invariants, scope, non-goals, rollback and finance/support cost.
4. Execute focused local tests, open review PR when authorized, inspect the **current exact-head** required CI checks, and summarize only statuses and failed-step excerpts.
5. Do not call CI pass MERGED; do not call MERGED RELEASED. The owner has **separately granted scoped standing PR merge authorization** to the hourly repository autopilot; use the exact-head review/protection checks in [GOV-02](docs/AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md). Human approval remains required for risky or blocked changes, real customer-file operations, production releases and paid/irreversible actions.
6. A task file is a continuation instruction, not an always-running AI system. Resume later in a new approved session/runner from GitHub and the checkpoint.

### Standing owner command: "go ahead"

In a repo-related conversation, interpret **"go ahead" / "continue" / "next"** as **start the next eligible bounded item from the live `next.md` NOW/NEXT/LATER queue without requiring another copy-paste prompt**. First reconcile GitHub main/PRs/CI. Work on the existing PR if it is still open or its checks failed; never create duplicate work from stale history. Otherwise choose highest-priority unblocked work, scope it, make a review PR, record tests/NOT RUN and update the queue. If blocked, explain the missing prerequisite and advance a different independent READY item only when it stays within ordinary non-destructive authorization. See [agent continuation](docs/AGENT_CONTINUATION_AND_HANDOFF.md). The separate [owner-authorized GOV-02 PR review/merge process](docs/AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md) permits **eligible** automatic PR merges only after mandatory checks; **no bypass of security, destructive user-file operation, release, spending or background execution from Markdown alone**.

### Test-only Rust exception

PR #81 accepted a narrow Rust verification pilot; PR #82 merged a dependency-free `tests/rust/fcj4-test-support` crate. Production remains pure C17. For this isolated crate, run:

```sh
cd tests/rust/fcj4-test-support
cargo fmt --all -- --check
cargo clippy --locked --all-targets -- -D warnings
cargo test --locked
```

Rust FFI is **not implemented**; a future test-only shim requires a separate safety review. Public static/shared libraries, production Rust and FCJ4 real recovery are not authorized. Existing C11/C17/C23, Docker, sanitizer and required CI checks remain mandatory. `SKILLS.md` is a catalogue; this lower-case file owns contributor workflow recipes.

## 1. Non-negotiable project identity

- Implement the **production product in pure C**. C17 is the production baseline; validate C11 and C23 independently. The accepted, isolated Rust test-support pilot under `tests/rust/` is an explicit exception, not permission for production Rust.
- No C++, C++ runtime, production Rust core or additional product implementation language without a new explicit owner decision and written review. The approved test-only Rust crate is governed by the accepted ADR.
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

Do not claim a milestone passed until its acceptance criteria pass. A passing PR is READY_FOR_REVIEW, not MERGED. Merge only with **specific owner approval or the active GOV-02 owner-authorized autopilot policy**, after full exact-SHA review, CI/protections and security gates. Verify the merge on `main` before starting dependent work.

## 11. Documentation maintenance

**Authority separation:** `AGENTS.md` sets binding engineering/safety rules, the accepted ADRs constrain architecture, this lower-case `skills.md` provides task execution recipes, uppercase `SKILLS.md` is a non-binding knowledge catalogue, `next.md` is the single current-status board and `docs/README.md` is the documentation index. Do not duplicate a task's status or contradict a newly accepted ADR in multiple places.


Update the relevant documents in the same change:
- User-visible behavior: README and CLI help.
- Module contract: header comments and LLD.
- Architecture or persistent protocol: HLD/LLD and ADR.
- Security behavior: threat model and security runbook.
- Operations: build, recovery, troubleshooting, and release notes.
- Tests: describe what is proven and what remains unproven.

Documentation must distinguish implemented behavior from planned design. Never present a design proposal as a production guarantee.
