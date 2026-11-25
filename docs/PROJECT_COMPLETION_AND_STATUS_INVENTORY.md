# Messy-File-Cleaner — Component Completion and Decision Inventory

**25 November 2026 — memory scoped evaluation:** #217 merged, 13/13 returned exact-head CI jobs passed; FCJ4 TSan and integrated-main CI NOT VERIFIED. Scoped test harness evaluation DONE 4/5, production allocator retained. [Memory evidence](performance/PATH_ALLOCATION_BASELINE.md); next [string evaluation](performance/STRING_EXTENSION_BASELINE.md). Real scanner cancellation, callback lifetime, RSS and whole-program speed not established. FS-01B/REL-01 remain blocked.

**25 November 2026 — hash-map scoped evaluation:** PR #213 merged, 14/14 exact-head jobs passed; integrated-main CI NOT VERIFIED. Existing production 64-slot map retained; synthetic benchmark acceptance complete (4/5), representative performance and hostile-key claims NOT established. [Evidence](performance/HASHMAP_BASELINE.md). Next independent test-only [path allocation baseline](performance/PATH_ALLOCATION_BASELINE.md). FS-01B/REL-01 remain blocked.

**Purpose:** Evidence-conscious companion to the [live NOW / NEXT / LATER board](../next.md). Source snapshot: GitHub `main` `30f14e93a70297d696026f2d3f60549ce50c7a1f`, 2026-11-16. This is a **source/recorded-CI review**, not a newly executed end-to-end safety assessment. When code and this page differ, current source and accepted contracts win. This page classifies **feature completion**, not only merged PRs.

## 2026-11-21 transition checkpoint — current evidence, not universal acceptance

Live audited main `3d7e14ed16040d6918f5d88330c510db77be0b96` includes [PERF-01U PR #128](https://github.com/Rofikali/Messy-File-Cleaner/pull/128). **PERF-01 CLOSED_LEGACY / engineering implemented**: disposable benchmark provenance and negative fixture-preservation coverage integrated, without a validated production million-file scale or real-host power-loss proof. The [feature policy](FEATURE_DELIVERY_AND_ACCEPTANCE_POLICY.md) prospectively caps each new feature at five opened PRs; **NOW CLI-DRYRUN-01** improves user-facing safe preview and carries 0/5 at this snapshot. C11/C17/C23, Makefile, Ubuntu 24.04, Docker and performance main-head CI passed; owner-run local Compose, external filesystems, deliberate power cuts and real customer data testing are NOT RUN/NOT AUTHORIZED. FS-01B P0 and REL-01 stay BLOCKED. Rust and FCJ3/FCJ4 remain isolated test-only/proposed.

## Advanced roadmap status clarification (2026-11-22)

The [advanced feature roadmap](ADVANCED_FEATURE_ROADMAP.md) and [acceptance matrix](ADVANCED_FEATURE_ACCEPTANCE.md) describe **proposed** preview/rules, duplicate detection, transactional recovery, watcher/scale, C SDK/FFI, desktop and product channels. They do not upgrade the below historical completion statuses. Current live selection (after merged PR #143) is read-only PLAN-REPORT-01 at 0/5; exact current state remains in [next.md](../next.md), verified against GitHub. FS-01B/REL-01 continue BLOCKED.

## A. Status and evidence semantics

Two independent dimensions must be recorded:

1. **Implementation maturity:** `NOT_STARTED` (no implementation), `DESIGN_ONLY` (proposal), `STARTED` (initial work), `PARTIAL` (some components or happy path exist, journey not complete), `IMPLEMENTED` (specified bounded scope in source), `LEGACY` (retained support), `DEFERRED` (explicitly postponed), `SUPERSEDED` (historical).
2. **Delivery/evidence stage:** `LOCAL_ONLY`, `PR_OPEN`, `CI_PENDING`, `RECORDED_TESTS`, `READY_FOR_REVIEW`, `MERGED`, `RELEASE_CANDIDATE`, `RELEASED`, `UNKNOWN`. `BLOCKED` describes a dependency or approval, not implementation completeness.

**Examples:** FCJ4 parsing can be **IMPLEMENTED / MERGED / TEST-ONLY** and still **BLOCKED for production adoption**. Rust Cargo scaffolding is **IMPLEMENTED / MERGED** but Rust-to-C FFI is **NOT_STARTED**. Docker Compose safety changes are **MERGED** but that is not broad production release approval. `CI_PASS` applies only to a stated SHA, suite and environment.

For every meaningful task record ID, priority, customer/risk value, prerequisites, owner decision, implementation state, delivery state, changed scope, acceptance/negative tests, exact PR/head/merge SHA and CI links, residual risks, and next action. A task is **READY** only if prerequisites and authority are satisfied.

## B. What is already built and merged (bounded claims)

| Area | Implementation / delivery | Source and recorded evidence | Remaining gap |
| --- | --- | --- | --- |
| CLI/config/classifier | **IMPLEMENTED / merged** | `src/main.c`, `src/config/config.c`, `src/classifier/classifier.c`, CMake production source list, CTest | Customer documentation, future validation and release evidence |
| Directory scanning / watcher | **IMPLEMENTED / merged** | `src/scanner/scanner.c`, `src/watcher/watcher.c`, scanner/watcher tests | Runtime/filesystem/host coverage and untested race boundaries |
| Queues/worker lifetime | **IMPLEMENTED / merged** for current CLI | `src/core/lf_queue.c`, `queue.c`, `thread_pool.c`; queue, pipeline and worker lifecycle tests | SPSC topology and embedding/global-state constraints; do not claim general MPMC |
| Production move/journal/recovery | **IMPLEMENTED / merged** for existing legacy behavior | `src/mover/mover.c`, `src/journal/journal.c`; mover/journal/fault/crash tests | **FS-01 release review**: untrusted paths, crash windows, source identity and other filesystems need evidence; never claim crash-proof |
| C portability and build | **IMPLEMENTED / recorded CI** | CMake C17 production, independent C11/C17/C23 Debug/Release tests; 28 CTest names visible in current CMake; ASan/UBSan on Debug | Matrix proves selected runner/compiler/fixtures, not all Linux filesystems |
| Makefile local workflow | **IMPLEMENTED / merged** | `Makefile`: Debug, Release, Profile, `test`, `validate`, dry-run `run`, guarded `run-execute`, benchmarks | `make self-check`, `make status` and `make rust-check` are now present in the root Makefile; recheck exact target behavior and CI before promising additional guarantees |
| Docker safe preview | **IMPLEMENTED / MERGED** | [PR #84](https://github.com/Rofikali/Messy-File-Cleaner/pull/84), merge `0184cc1`: readonly `/input` default; `compose.execute.yaml` explicit writable move path; Docker PR-head CI passed | Real data/backup/host permissions still operator responsibility; no release certification |
| CI workflows | **IMPLEMENTED / recorded CI** | Eight tracked workflows for CMake, Makefile, Ubuntu 24.04, Docker, TSan, Rust, performance and large-scale tests | Not all are run on every PR; no deployment/CD pipeline or universal release gate |
| Rust pilot policy/scaffold | **IMPLEMENTED / MERGED** **test-only** | [PR #81](https://github.com/Rofikali/Messy-File-Cleaner/pull/81) ADR and [PR #82](https://github.com/Rofikali/Messy-File-Cleaner/pull/82) Cargo scaffold; `tests/rust/fcj4-test-support` | No Rust FFI call to C; no Rust product binary/runtime |
| Engineering governance | **IMPLEMENTED / MERGED** documents | [PR #83](https://github.com/Rofikali/Messy-File-Cleaner/pull/83) roadmap, [#85](https://github.com/Rofikali/Messy-File-Cleaner/pull/85) retention, [#86](https://github.com/Rofikali/Messy-File-Cleaner/pull/86) language/authority | This status inventory and live NOW/NEXT/LATER reconciliation are separate proposed documentation work |
| Benchmark harness | **IMPLEMENTED / recorded benchmark** | `tests/performance/benchmark_cli.py`, `benchmark_moves.py`; [dated CI baseline](performance/CI-BASELINE-2026-11-04.md) | Current revision/target-host RSS, cold-cache and representative 100k/1m scaling not established |
| FCJ3 diagnostic/test modules | **IMPLEMENTED IN TESTS / merged, not production** | `tests/unit/test_fcj3_*`, `tests/integration/test_fcj3_*`, `docs/quality/FCJ3_*` | No production FCJ3 mover/recovery; do not execute inferred cleanup |
| FCJ4 parser, serializer, writer state | **IMPLEMENTED IN TESTS / merged, not production** | `tests/unit/test_fcj4_parser.c`, `test_fcj4_writer.c`, parser/writer source included in CTest only | Persistent production integration, recovery/FS truth and external compatibility remain unapproved |
| FCJ4 test-only append/concurrency | **IMPLEMENTED IN TESTS / merged, not production** | `tests/integration/test_fcj4_test_append*.c`, standalone conditional TSan job | Test adapter cannot be advertised as production journal, transactional move or recovery executor |
| FCJ4 malformed/restart test matrix | **IMPLEMENTED / MERGED** test scope | [PR #80](https://github.com/Rofikali/Messy-File-Cleaner/pull/80), merge `4362c41`, `test_fcj4_restart_matrix.c` | No evidence of power-loss or arbitrary filesystem crash consistency |

**Important:** “Merged” in these rows means the bounded component exists on main, not that every end-to-end user journey is complete. Current source and tests have priority over old design documents (some `docs/quality/` pages still describe earlier in-progress branches).

## C. Started, partially complete, or under ongoing review

| Topic | Correct status | Why it is not DONE | Smallest next action |
| --- | --- | --- | --- |
| FCJ4 recovery story | **PARTIAL overall**: grammar/replay/writer/test append exist, **not production** | Parser acceptance ≠ journal authenticity, user file identity, authorization to delete or durability of a real end-to-end move | FS-01 source-first risk review; future design/negative tests and explicit owner safety decision |
| Rust property/fuzz system | **STARTED**: isolated Rust byte-input test scaffold | Rust code does **not** yet call C parser through FFI; property/fuzz test results absent | RUST-01 design/implementation in separate bounded test-only PR after review |
| Product release readiness | **PARTIAL evidence; release BLOCKED** | Existing CI/Docker/test baseline is not a signed artifact, support matrix, installer/license policy or live-user backup/recovery assurance | REL-01 candidate checklist and owner go/no-go |
| Benchmark capacity claims | **PARTIAL evidence** | 5k/500 fixture baselines and harness exist; 100k/1m stress, resource limits and target-device behavior need execution and provenance | PERF-01 bounded benchmark evidence, manual million-file tier |
| SDK/embedding | **DESIGN_ONLY** | Proposed public `fc_engine_t` and `.a`/`.so` lack implemented stable API/ABI, independent consumer and lifecycle isolation | ABI-01 only after real consumer/owner decision |
| Customer/business fit | **DESIGN_ONLY / NOT VALIDATED** | No verified user research, adoption, recurring retention, willingness-to-pay, support or licensing plan | BIZ-01 interviews and simple metrics before paid features |

## D. Open, blocked, and conditional decisions

- **FS-01 P0 for broader release:** [FS-01A source-first audit](security/FS_01_PRODUCTION_FILESYSTEM_AUDIT_2026-11-16.md) identifies unresolved source-retirement check/unlink, legacy pathname recovery and crash-orphan risks. [FS-01B known-risk characterization](quality/FS_01B_RETIREMENT_RACE_CHARACTERIZATION.md) **MERGED PR #89**, and the exact-head CI reproduced the unsafe check/use behavior on private fixtures while retaining backup hard links. **PASS means risk reproduced, not resolved**. [FS-01B-FIX ADR](architecture/ADR_FS_01B_SOURCE_RETIREMENT_BOUNDARY.md) and [positive test plan](quality/FS_01B_FIX_SECURITY_ACCEPTANCE_PLAN.md) are **PROPOSED only**: the [FS-01B-GUARD mitigation](security/FS_01B_SHARED_SOURCE_GUARD.md) is an independent code review candidate restricting group/other-writable source directories with `EPERM`; it **does not close the same-UID check/use race**. Complete production correction requires accepted threat/crash design, a positive preservation regression and verified gates. Preserve uncertain files rather than guessing ownership.
- **REL-01 P0 for distribution:** release artifact provenance, supported environment/filesystems, license, backup guidance, smoke, rollback and incident handling. No automatic deployment is established.
- **RUST-01 P1:** permitted *scope* is future C parser test shim and Rust test harness only. An accepted test-pilot ADR is not automatic merge authorization.
- **RUST-02 P1:** property/fuzz test improvements depend on RUST-01 or a separate reviewed design that does not duplicate protocol parsing.
- **HYG-02 (owner decision):** `perf.data` (binary, not inspected) and `gprof.txt` remain **tracked**, despite ignore patterns. Deletion/history rewrite was not authorized by PR #85.
- **CI/TOOL-01 (optional):** `make self-check`, Rust menu and change-scoped CI optimization could improve developer efficiency, but no existing required checks may be silently skipped.
- **FCJ4 production adoption:** **BLOCKED / NOT APPROVED.** No upgrade by simply compiling test-only source into `file-cleaner`; requires journal compatibility, real filesystem protocol, ownership, fault tests and explicit decision.
- **ABI/public SDK:** **DEFERRED** pending consumer and licensing policy.
- **Cloud/GUI/desktop/billing/AI features:** **NOT_STARTED / NOT APPROVED**; not needed for the current local CLI.

## E. Future customer-visible journeys and acceptance ideas

| Journey | Current status | What would make it real |
| --- | --- | --- |
| Preview a directory and see proposed categories without file mutation | **Implemented CLI + default Compose** | UX and accessibility are CLI concerns; user documentation, representative fixtures and support guidance |
| Deliberately execute safe batch moves | **Implemented bounded CLI flow** | High-risk filesystem and recovery validation on supported Linux targets |
| Recover an interrupted operation safely without guessing | **PARTIAL** | Reproducible crash/failure and durable evidence; no destructive automatic recovery when object identity cannot be proven |
| Embed reusable engine in another app | **Not started** | Public C API, stable ABI, independent consumer and packaging/license gates |
| Provide desktop GUI | **Not started** | Demand, threat model, platform/installer and support economics |
| Offer hosted API/SaaS | **Not started** | Real need, consent/security/privacy, cost controls and separated product ADR |
| Offer paid support/enterprise workflow | **Hypothesis only** | Interviews, support/license terms, viable distribution cost |

Do not assign fictitious completion percentages. Work can be 100% merged for a **bounded test-only module** while the customer product journey remains partial.

## F. Recorded verification and CI interpretation

- Current `CMakeLists.txt` lists **28 CTest names**; this is **not** a claim that all 28 were rerun locally during this documentation audit.
- Four GOV-01 PR-head workflows passed at [PR #86](https://github.com/Rofikali/Messy-File-Cleaner/pull/86), head `1228ebfe916922cb836ee853ec1d99c6091ae862`, merge `30f14e9`: CMake matrix, Docker, Makefile, Ubuntu 24.04.
- [PR #84](https://github.com/Rofikali/Messy-File-Cleaner/pull/84) head `92c1d3d` passed Docker, CMake and Makefile; the standalone path-filtered Ubuntu validation was not triggered for that PR. Do not treat conditional nonexecution as pass.
- TSan and large-scale benchmark workflows have limited triggers, and manual workflows are not required on every doc PR. A standard PR does not prove TSAN clean, 1m scale or actual power-loss durability by default.
- Distinguish historical recorded CI from **fresh** exact-head CI for a new PR, and from unsupported host/release claims.

## G. Updating this document without duplicating history

- [`next.md`](../next.md) is the **only live task selector** (NOW/NEXT/LATER). This inventory records per-component completeness and gates; do not keep a second competing list of “today's next task” here.
- Retain older detailed FCJ3/FCJ4 design notes, but add a `Historical status; consult next.md` label if their branch/PR claims are stale. Do not delete technical evidence.
- After every confirmed merge, update the live register in the **next authorized PR** with exact merge SHA and distinguish code-merged from verified workload and released product.
- If source, docs and CI disagree, disclose uncertainty, identify a bounded research action, and do not auto-implement destructive recovery.
