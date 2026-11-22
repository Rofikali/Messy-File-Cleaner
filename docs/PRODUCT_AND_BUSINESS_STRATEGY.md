# Messy-File-Cleaner — Product, Business and Engineering Strategy

**Status: hypotheses and decision framework, not customer validation, prices, approved monetization, or launch.** Reviewed C17 CLI + test-only Rust at `main` `559ba1e` on 2026-11-16.

## Advanced product hypotheses and financial gate (2026-11-22)

The [advanced feature roadmap](ADVANCED_FEATURE_ROADMAP.md) proposes staged **safe preview/rules, duplicate and metadata reports, reliable watcher/scanner, C static/shared SDK, Rust FFI, optional desktop and distribution**. The [review/acceptance matrix](ADVANCED_FEATURE_ACCEPTANCE.md) imposes measurable security, Linux/CI/SRE and CA/MBA/management evidence for each stage. These are **NOT_STARTED or DESIGN_ONLY**, not an approved product tier, revenue forecast, paid service, guaranteed ROI or production release. Validate first-user jobs and willingness to pay before implementing desktop/SDK products; define open-source or commercial licensing, third-party obligations and support liability before public redistribution. Compare staff time, tests/CI minutes, maintenance, privacy, platform/installer costs and potential file-loss exposure against actual evidence. Never buy infrastructure or automatically enable remote user-data collection. For the immediate NEXT, prioritize read-only PLAN-REPORT-01 rather than destructive moves; FS-01B P0 and REL-01 remain release blockers.

## Five-PR cost and customer-outcome discipline (2026-11-21)

Each feature now has a maximum of five opened PR attempts, not five automatically required iterations; earlier accepted outcomes exit sooner. Closing the repetitive historical PERF-01 fixture lane freed capacity for bounded safety and CLI work; current planned read-only preview reporting follows merged PR #143. Use the live board for today's work rather than old snapshot selections. Treat actual engineering hours, CI compute/energy, security review, support obligations and possible file-loss liability as business costs; do not invent revenue, market fit or claim a production-safe release. A release remains blocked on FS-01B same-UID source-retirement trust/crash safety regardless of schedule efficiency.

## 1. Problem and customer outcome

Proposed job: "Organize a local Linux file tree into understandable categories **without unexpectedly overwriting or losing user files**, and explain what happened if execution fails." Competing differentiation is a **testable safety contract**, dry-run, journal evidence and reproducible performance, not an unsupported nanosecond guarantee.

Initial audiences to research, not assume:
- Linux developers, power users and teams organizing downloads or disposable workspaces.
- Desktop support technicians and administrators who need predictable batch processing.
- Script/automation integrators who would value a small public C API *after* the CLI proves useful.

Do not advertise universal recovery or use on irreplaceable files without verified backups and a supported environment.

## 2. What exists and what is only a proposal

| Area | Reality at current baseline | Conditional next stage |
| --- | --- | --- |
| CLI | Implemented C17 Linux/POSIX tool with config, scanning, classification, workers, journal | Make safety and recoverability trustworthy for targeted users |
| Container | Ubuntu 24.04 dev/prod Docker with non-root runtime and Compose | Tighten source mount policy and installation guidance |
| FCJ4 | Test-only parser/writer/append adapter | Additional evidence; future production safety design requires independent approval |
| Rust | Dependency-free test-support crate, **no FFI** | Test shim and property/fuzz only |
| Library | No public static/shared deliverable | Real consumer + stable C API/ABI, packaging, versioning |
| UI / desktop | Not implemented | Only after demand/support economics justify product channel |
| Cloud/SaaS/API | Not implemented | Not necessary for a local file organizer; separate security and business ADR |
| Paid product | Not implemented | Customer discovery, support/licensing and honest free/paid test first |

## 3. Strategy order

**S0 — protect data:** source/destination identity, no-replace, journal/recovery failure handling and crash tests. This is the core quality bar.

**S1 — usable local tool:** reliable dry-run, clear summary, safe defaults, packaging instructions, documented non-goals, user feedback and support diagnostics.

**S2 — adoption:** test small community distribution, measurable usage/feedback and compatibility. Avoid tracking users or collecting private filenames without explicit consent/privacy design.

**S3 — optional library:** C static `.a` and later shared `.so` only with a concrete customer/developer consumer, versioned ABI, symbol visibility, ownership, license and release support plan.

**S4 — optional desktop/API/cloud:** new channels only when user research, security analysis and maintenance economics justify them. No speculative microservices or database.

## 4. CA / MBA / CTO operating economics

Track actual **engineering time**, CI runner minutes, artifact storage, security review, test fixture maintenance, support tickets and regressions as project cost. Do not invent revenue, marketing conversion or cash runway. Distinguish fixed engineering costs from variable support/release costs.

Suggested KPIs after instrumenting honest data:
- File preservation regressions (release blocker target: zero known high-severity issues).
- Reproducible build and required CI pass rate.
- Crash/recovery negative cases and affected filesystem matrix.
- Median/p95/p99 durations for representative scan/move workloads with CPU/RSS/syscall data.
- Time per reviewed milestone and PR cycle time.
- New-user activation (first safe dry-run), successful repeated use, issues per user and support time.
- Potential commercial willingness-to-pay only if interviews support it.

**Free/paid hypotheses**, not product scope: free core CLI for trust and distribution; paid support, enterprise integration or tested SDK licensing could be considered if customers want it. A licensing decision is required before redistribution/embedding: current README declares **no license**.

## 5. Business risk register

| Risk | Consequence | Response |
| --- | --- | --- |
| User data loss | Severe trust/legal/support impact | Conservative file semantics, backups, tests, no unsupported guarantees |
| Confusing recovery claims | Unsafe user action | Separate parser validity from authenticity and FS truth |
| Premature `.so` ABI | Compatibility/support burden | Define consumers first |
| Rust complexity without value | More CI/maintenance, unsafe FFI | Keep testing pilot isolated and evaluate findings |
| Expensive large-scale CI | Runner/disk/inode costs | Manual bounded million-file workload; focus local lane |
| Feature proliferation | Slower trustworthy release | One niche, small milestones, explicit no-go list |
| Missing license/distribution terms | Commercial/reuse uncertainty | Legal/owner decision before public SDK or product claims |

## 6. Milestone business review question

For every next task: *What loss or user friction does it prevent? What measurable outcome will establish success? What maintenance cost does it create? Does it change data-liability, licensing, privacy or user expectations?* Record answers without inventing numeric ROI.

Pricing, SaaS accounts, telemetry, analytics, updater/installer, signed release and public APIs require separate accepted decisions and specific security/privacy review. Use [next.md](../next.md) for execution order.
