# Messy-File-Cleaner — Advanced Feature and Product Roadmap

**Status: proposed future work, not implemented features, release approval, or an automatically executable queue.** Owner requested 2026-11-22. Baseline verified against `main` at `80de73b465ede49eec0882846c50efc0cbdee254` (merged PR #143). The only live task selector is [next.md](../next.md), with [feature-budget.json](feature-budget.json) enforcing **at most five opened PR attempts per stable feature**. This roadmap supplies candidates and dependencies; a task becomes active only after live GitHub reconciliation, a bounded acceptance plan, and registration in the ledger. Documentation does not authorize production code, destructive filesystem operations, spending, SDK release, or deployment.

## Mission and product positioning

Build a **native C17 Linux/POSIX file-management engine** that prioritizes **provable file preservation, explicit operation planning, conservative recovery, bounded resource usage, and measured performance**. A future reviewed public SDK and desktop interface may expand it into a file-management platform. Do **not** call the current program production-safe, cross-platform, enterprise-grade, crash-proof, or nanosecond-fast.

**Implemented baseline, with known limitations:** extension-rule CLI/classifier; scanner/watcher; bounded queues/workers; `--dry-run`; legacy mover/journal/recovery code; Ubuntu 24.04/Docker and C11/C17/C23 CI; FCJ3/FCJ4 and Rust **test-only** experiments. The scanner opened-directory identity check merged in #142; it did **not** resolve subsequent pathname substitution or source retirement. The offline board/ledger consistency check merged in #143; it does **not** verify live GitHub status. Confirm details in the [component evidence inventory](PROJECT_COMPLETION_AND_STATUS_INVENTORY.md) and actual source.

## Risk and dependency order

| Stage | Outcome | Candidates (NOT YET IMPLEMENTED unless independently verified) | Gate |
| --- | --- | --- | --- |
| S0 — security first | No unrelated file lost, even across uncertain operations | FS-01B-FIX source-retirement trust/crash protocol; positive preservation; no-overwrite and recovery verification; REL-01 acceptance | **BLOCKED for destructive/release work** until owner-approved design and evidence |
| S1 — trustworthy preview | Explain all intended changes without modifying files | PLAN-REPORT-01 structured read-only preview; RULES-01 deterministic rule evaluator and exclusions; COLLISION-01 conflict reports | Can start with read-only, disposable-fixture scope |
| S2 — non-destructive intelligence | Find organizational opportunities without deciding deletions | DUP-SCAN-01 duplicate reporting; METADATA-01 validated type/date/size inspection; INTEGRITY-01 read-only integrity inventory | Privacy/data handling, bounded memory, filesystem adversarial tests |
| S3 — continuous reliability | Detect change without silently losing events | WATCH-RESYNC-01 bounded reconciliation on inotify overflow; cancellation and error propagation | Event-loss tests, no automatic destructive action |
| S4 — measurable scale | Large trees with fixed memory/FD bounds and honest benchmarks | SCALE-SCAN-01 resource-budgeted traversal; profiling and representative 100k/1m workloads | Measured on named hardware/filesystem; security invariants retained |
| S5 — developer library | Embeddable native engine rather than CLI-only globals | API-01 `.a`/versioned `.so`; ABI-01 stable C ABI; FFI-01 Rust consumer bindings | Real external consumer, licensing, architecture ADR, ownership and compatibility testing |
| S6 — supported product interfaces | Useful installer/desktop experience | TUI-01 accessible terminal UI; DESKTOP-01 optional reviewed desktop shell | Core release acceptance, security model, support cost and user evidence |
| S7 — distribution/business | Supportable, lawful product rather than prototypes | REL-01 supported Linux release, signed/provenance artifacts; BIZ-01 interviews/licensing/support; optional professional/enterprise channels | Owner go/no-go, backup/recovery, release/security/license and cost approval |

**Feature-ID names in this table are planning labels, not feature-ledger registrations.** Do not consume PR attempts or mark them READY until separately selected. Five PRs is a ceiling, never a requirement or a workaround for unsolved acceptance risks.

## First bounded, independent candidate: PLAN-REPORT-01

**Proposed active task after #143:** inspect the actual existing `--dry-run --once` output and avoid reimplementing existing metrics/preview. Introduce a **read-only, opt-in structured plan/report** only if a verified user-facing gap exists. Initial scope: one reviewable C17 CLI change plus disposable negative tests and necessary docs, not a new framework.

Acceptance for its first feature PR:
- Capture proposed source category, destination category, and disposition (candidate / conflict / skipped / error) with a documented versioned JSON schema where feasible; explicitly avoid promising a stable plan across filesystem mutation.
- The default invocation must keep its existing behavior. Add any opt-in flag only after validating the CLI contract; never silently activate real moves. Do **not** expose private paths or sensitive filenames in routine logs/telemetry.
- Under `--dry-run`, make **no filesystem mutations** to customer input/output, no destination creation, no move journal open/write, no auto-delete or duplicate resolution. Use only owned disposable fixtures in tests; assert positive source preservation and error behavior.
- Handle hostile filenames and path substitutions without injection or pretending a stale preview grants execution authority. Define escaping, encoding, ordering, limits, backpressure, exit codes, partial errors and bounded memory.
- C17 production plus C11/C23 compatibility, ASan/UBSan and applicable GitHub CI at the **exact PR head**. Operator Ubuntu/Docker Desktop/Compose/real-filesystem/backup/power-loss remain NOT RUN until actually performed.
- Register one stable feature ID and actual GitHub PR number in the ledger; close DONE/PARTIAL/BLOCKED as evidence warrants. **Do not touch FS-01B source-retirement behavior** in this task.

## Advanced functional capabilities — requirements and non-goals

| Capability | Example user value | Initial non-destructive deliverable | Must NOT happen automatically |
| --- | --- | --- | --- |
| Rule engine | Category by extension, size, age, verified type, path, priority | Explain which rule wins; bounds and deterministic conflicts | Evaluate arbitrary scripts or move user data |
| Duplicate detector | Identify byte-identical files even with different names | Read-only size/hash candidate report; optional final byte match | Delete a duplicate, treat filename equality as content proof |
| Transactional undo | Reverse verified operations where safe | Threat model, journal state/identity design and controlled replay simulation | Delete/rename on ambiguous recovery evidence |
| Fast scanner | Navigate deep, high-cardinality directory trees | Bounded FDs/memory, cancellation and measured traversal | Sacrifice inode/path integrity for throughput |
| Continuous watch | Organize newly created files reliably | Overflow detection, bounded rescan and reconciliation | Assume inotify never drops events or permit silent moves |
| Collision handler | Prevent overwriting existing destinations | Explicit no-clobber conflict plan and safe refusal | Use user-chosen overwrite without separately reviewed policy |
| Integrity inventory | Verify file copies and read-only catalogues | Digest metadata, error reporting and deterministic test cases | Interpret digest match as authorization to unlink |
| Metadata classification | Date/type aware organization | Parse narrowly accepted formats with size/time limits | Trust arbitrary unverified metadata or transmit files |
| Machine-readable output | Feed a script or UI safely | Versioned JSON reporting, schema and output limits | Treat a preview as an executable signed authorization |
| TUI / desktop | Review operations interactively | UI operating on preview-only APIs | Bypass backend policy or release gates |
| Optional local AI | Suggest categories offline | Explicit opt-in advisory-only prototype after core proof | AI-directed deletion, cloud uploads or hidden telemetry |

## Linux systems and reliability design options (NOT an approved implementation)

Potential mechanisms include descriptor-relative resolution (`openat`, constrained `openat2` where supported), careful `O_NOFOLLOW`, no-replace publication (`renameat2` where available), `O_TMPFILE` on compatible filesystems, exact `fsync`/directory ordering, and `inotify` with overflow reconciliation. Each requires portability, permission, concurrency, durability and failure-mode analysis. **No single syscall closes the known same-UID `fstatat`→`unlinkat` check/use window.** POSIX/link/namespaces, filesystem support and operator threat assumptions must be documented; safe fallback is often to retain source and report uncertainty.

Future transaction objective is **plan → verify → stage → copy → validate → publish no-clobber → durable journal/reconcile → retire only with proved authority**. Actual sequencing needs approved threat/crash model, on-disk compatibility plan, and positive unrelated-file-preservation evidence. The [proposed source-retirement ADR](architecture/ADR_FS_01B_SOURCE_RETIREMENT_BOUNDARY.md) is not accepted production authority; the [known-risk characterization](quality/FS_01B_RETIREMENT_RACE_CHARACTERIZATION.md) passing means the race was reproduced, not solved.

## Engineering review, evidence, and release gates

Use the detailed [advanced-feature acceptance and design review](ADVANCED_FEATURE_ACCEPTANCE.md). The [seven-level engineering operating model](ENGINEERING_OPERATING_MODEL.md) is a proportional review rubric, **not seven independent agents**. Evaluate architecture and SOLID/DRY/KISS/YAGNI, C17 ownership/concurrency, CISSP-style threat/risk controls, Linux/POSIX filesystem conditions, CI/release/SRE, CCIE/network only if a concrete remote interface exists, and CA/MBA/management cost, liability, user value and maintenance. Never fabricate a benchmark, revenue, customer demand or certification.

For each activated feature: define owner/user outcome, threat model, current source behavior, alternatives, one bounded acceptance scope, negative tests, portability, PR budget, manual **NOT RUN** entries, evidence SHAs, required reviewer/owner decisions, and rollback. Separate DESIGN_ONLY, PR_OPEN, MERGED, CI_PASS, RELEASE_ACCEPTED and RELEASED.

## Linked authoritative documentation

| Subject | Authoritative related docs |
| --- | --- |
| Live prioritization, feature PR budget and agent continuation | [next.md](../next.md), [feature ledger](feature-budget.json), [five-PR policy](FEATURE_DELIVERY_AND_ACCEPTANCE_POLICY.md), [agent handoff](AGENT_CONTINUATION_AND_HANDOFF.md) |
| Product/code evidence and documentation authority | [README](../README.md), [completion inventory](PROJECT_COMPLETION_AND_STATUS_INVENTORY.md), [docs map](README.md), [milestone document transaction](MILESTONE_DOCUMENTATION_WORKFLOW.md) |
| System architecture and native interfaces | [HLD](HLD.md), [LLD](LLD.md), [architecture playbook](architecture/ENGINEERING_PLAYBOOK.md), [C API/ABI/FFI](architecture/C-API-ABI-FFI-ROADMAP.md), [Rust boundary](architecture/RUST_TEST_SUPPORT_BOUNDARY.md) |
| Filesystem security, trusted recovery and journal | [threat model](security/THREAT_MODEL.md), [FS-01 audit](security/FS_01_PRODUCTION_FILESYSTEM_AUDIT_2026-11-16.md), [FS-01B ADR](architecture/ADR_FS_01B_SOURCE_RETIREMENT_BOUNDARY.md), [positive preservation plan](quality/FS_01B_FIX_SECURITY_ACCEPTANCE_PLAN.md), [FCJ4 design](quality/FCJ4_SCHEMA_AND_REPLAY_DESIGN.md) |
| Scale, reproducibility and deployment | [performance engineering](performance/PERFORMANCE-ENGINEERING.md), [CI/Makefile](CI_CD_AND_MAKEFILE_OPERATIONS.md), [Docker runbook](docker/DOCKER-RUNBOOK.md) |
| Product, finance, governance and release decision | [product/business strategy](PRODUCT_AND_BUSINESS_STRATEGY.md), [engineering operating model](ENGINEERING_OPERATING_MODEL.md), [autopilot PR merge policy](AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md) |

**Owner approval gates:** no valuable customer-file moves, destructive recovery, production FCJ3/FCJ4/FFI, external SDK release, unsanctioned shell/cloud network, billing, deployment, license commitments, hidden user telemetry, or branch-protection bypass. A docs roadmap is not approval to do those actions.
