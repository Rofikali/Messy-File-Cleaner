# Messy-File-Cleaner — Product, Business and Engineering Strategy

**Build versus reuse:** [Master plan](upstream/UPSTREAM_MASTER_PLAN.md) requires comparable-horizon TCO across integration, testing, compliance, maintenance, operations and opportunity cost. Free acquisition is not zero TCO. Owner license decision and any spend or commercial commitments remain separate.

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


## Maintenance reconciliation and measured TCO baseline

#291 merged at `f0f95f66bc9800ba4e9fea87ef9e512f84446681`, tested head `105c13e1ff521b859a47a3c361edc1506fce24b5`. 13/13 returned latest jobs PASS after APT HTTP 403 retries (Makefile 140303420197; Ubuntu C23 140303430639). Ubuntu GCC C11/C17/C23 Release 81/81; hosted Clang Debug 77/77 and static/shared sanitizer consumers PASS (job 140303431369), isolated Clang Release 80 pass/1 io_uring skip, correlation 4 pass/1 unverified versus hosted 5/5. Historical static/shared acceptance stays PARTIAL; later evidence does not rewrite historical exact-head checks. Formal independent review and TSan workflow not returned. UPSTREAM-MAINT-01 PARTIAL 5/5, no sixth PR. #292 begins distinct [measured cost baseline](upstream/UPSTREAM_TCO_EVIDENCE.md) with actual selected CI log spans, comparable-horizon TCO and explicit unknown billing/owner time; no ROI claim or automatic spending. Design only, CI pending; license/import, FFI, FS-01B/REL-01 and production ABI gates unchanged.


## UPSTREAM-COST-01 — checked integer comparison

#292 merged at `cbf8b068564cb493213cfa55fef8ac9a262b07c5`, tested head `29ac7703fb87cc720fd99e11d59beecc710074be`; 13/13 returned exact-head jobs PASS (CMake 45937918563, Ubuntu 45937918525, Docker 45937918686, Makefile 45937918541); formal independent review and TSan workflow not returned. #293 adds a [BUILD_TESTING-only reference](upstream/UPSTREAM_TCO_EVIDENCE.md), using unsigned 64-bit minor-unit amounts, three one-time and four constant monthly cost categories, matching three-uppercase-letter currency identifiers and horizons of 1–120 months, explicit known/unknown flags, checked addition and horizon multiplication, and unsigned difference plus direction. Reject unknown amounts, invalid metadata, null arguments and arithmetic overflow; do not publish a result on failure. The currency identifier is syntactic, not an ISO currency registry lookup; amounts must use the same minor-unit scale. Seventeen synthetic checks include equal/higher/lower options and error controls. Categorical values and horizons are illustrative, not project financial measurements; no labor-rate conversion, tax, discounting, inflation, varying monthly costs, billing ingestion, revenue or ROI model. Local NOT RUN; exact-head CI pending. No dependency, production linkage, import, spend or release; Rust FFI and FS-01B/REL-01 remain blocked. Rollback: revert PR.


## UPSTREAM-COST-01 — bounded horizon sensitivity

#293 merged at `f7ae637aea60eb831dede7fd89b0a809bc551d1e`, tested head `c22f4457091c655006d277814cf6b1490bb35220`; 14/14 returned jobs PASS (CMake 45938409448, Ubuntu 45938409320, Docker 45938409369, Makefile 45938409298, FCJ4 TSan 45938409307); GCC C17 Release 82/82 (job 140306836990), Debug 78/78 (job 140306836973), TCO reference CTest passed. Formal independent review not returned. #294 extends the [private reference](upstream/UPSTREAM_TCO_EVIDENCE.md) with a bounded 1–120 month sweep using the checked comparison per horizon, lower/equal/higher counts and first strictly lower horizon (zero when none). Output remains unchanged on failed evaluation, including later overflow. Twelve additional synthetic checks cover equal alternatives, crossover at month 12 with first strictly lower month 13, reversed comparison, truncated horizon, nonintegral crossover, immediate savings, late overflow, unknown and invalid arguments. Fixed monthly costs, no extrapolation, discounting, inflation, changing costs or financial recommendation. Existing 17 checks retained; local NOT RUN, exact-head CI pending. No actual charges, ROI, dependencies, imports, production changes or spend. Licensing, Rust FFI and FS-01B/REL-01 remain blocked. Rollback: revert PR.


## UPSTREAM-COST-01 — cash and noncash economic cost separation

#294 merged at `f77a47765a2907757aab74d209a5cb0a374e860f`, tested head `71000f1d27d8b7f62354ed62b5d112534ce3b260`; 13/13 returned jobs PASS (CMake 45939596896, Ubuntu 45939596771, Docker 45939596841, Makefile 45939596844); GCC C17 Release 82/82 (job 140310606069), Debug 78/78 (job 140310606118), existing TCO CTest with 29 synthetic checks passed. Formal independent review and TSan workflow not returned. #295 extends the [private reference](upstream/UPSTREAM_TCO_EVIDENCE.md) with separate cash and noncash owner-time option records, same currency and horizon, checked combined total and atomic failure. Cash can be evaluated independently even when noncash inputs remain unknown. Caller must allocate each economic amount once; the reference does not detect overlapping categories or convert hours to rates. Ten additional synthetic checks cover zero cash/nonzero noncash, independently known cash, unknown amounts, metadata mismatch, combined overflow and null arguments. Fixed monthly assumptions; not a financial accounting, billing or profitability system. Local NOT RUN; exact-head CI pending. No actual charges, ROI, dependencies, imports, production changes or spending. Licensing, Rust FFI and FS-01B/REL-01 remain blocked. Rollback: revert PR.
