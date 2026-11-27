# Messy-File-Cleaner — Component Completion and Decision Inventory

**Private shared sanitizer and ownership:** [Linux-inspired architecture](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #283 merged; 10/10 returned jobs, C17 Release 76/76 and Debug 73/73, runtime and ELF hardening PASS; Ubuntu workflow and formal review unverified. #284 (4/5) sanitizer/ownership candidate pending.

**Private shared runtime and hardening:** [Linux-inspired architecture](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #282 merged; 10/10 returned jobs, C17 Release 75/75, Debug 72/72 and ELF controls PASS. #283 (3/5) runtime/hardening checks pending; Ubuntu workflow and formal review unverified.

**Private shared ELF boundary:** [Linux-inspired architecture](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #281 merged; 10/10 returned jobs, C17 Release 74/74, Debug 72/72, eight consumer scenarios PASS. #282 (2/5) ELF metadata and two negative controls pending; Ubuntu workflow and formal review unverified.

**Private shared library reference:** [Linux-inspired architecture](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #280 merged; 10/10 returned jobs, C17 Release 73/73, Debug 71/71 and final artifact gate PASS; STATIC-LIB-01 PARTIAL 5/5. #281 starts SHARED-LIB-01 (1/5), CI pending.

**Final private static archive acceptance:** [Linux-inspired architecture](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #279 merged; 10/10 returned jobs, C17 Release 72/72 and Debug 71/71, extended consumer and dependency test PASS. Ubuntu workflow and independent review unverified; #280 (5/5) final artifact gate pending.

**Static archive matrix and ownership:** [Linux-inspired architecture](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #278 merged; 10/10 returned jobs, C17 Debug/Release 71/71; Ubuntu workflow and formal review unverified. Local exact-blob checks PASS. #279 (4/5) matrix ownership candidate pending.

**Static archive dependency and sanitizer boundary:** [Linux-inspired architecture](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #277 merged at f8c54bfa1485a21a4b57e66d0c3e15fe2bff7972; 13/13 returned jobs, GCC C17 Release 71/71, five definitions and two negative controls PASS. #278 (3/5) dependency checks and sanitizer consumer pending.

**Static archive symbol boundary:** [Linux-inspired architecture](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #276 merged at 696eff11165e9ed90049eabe81e32f6430650d2c; 14/14 returned jobs, GCC C17 Release 71/71 and identical archive pair PASS. #277 (2/5) symbol boundary pending.

**Owner-approved Linux-inspired library architecture (2026-11-27):** [Engineering requirements](architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #276 initial Clang Release failure on three missing terminal newlines corrected on same PR; repeat-build archive comparison pending. Production library remains gated.

**Private static archive reference:** [API roadmap](architecture/C-API-ABI-FFI-ROADMAP.md). #275 merged at f000517ef7ad81934cfed141e05489831833c82a; 13/13 returned jobs and three artifact isolation checks plus six negative controls PASS; PUBLIC-API-01 scoped DONE 5/5. #276 STATIC-LIB-01 (1/5) private archive CI pending.

**Public API final isolation candidate:** [API roadmap](architecture/C-API-ABI-FFI-ROADMAP.md). #274 merged at 84ea67e73cd93ad2fe3e5b5595d7f60072b78ce0; 13/13 returned jobs, 140 synthetic calls, 112 matched allocations/releases and hosted sanitizer checks PASS. #275 (5/5) final candidate CI pending.

**Public API synthetic ownership:** [API roadmap](architecture/C-API-ABI-FFI-ROADMAP.md). #273 merged at 0e72e1d60e5032563548f453f2ea00fefc6d08e1; 13/13 returned jobs and 60 mock scenarios PASS; formal independent review not verified. #274 (4/5) adds synthetic heap ownership and recovery; CI pending.

**Public API mock caller:** [API roadmap](architecture/C-API-ABI-FFI-ROADMAP.md). #272 merged at 1071b60baf6be4768e70f45932d133429068a8ab; 13/13 returned jobs PASS, formal independent review not verified. #273 (3/5) adds synthetic caller checks; CI pending.

**Public API consumer boundary:** [API roadmap](architecture/C-API-ABI-FFI-ROADMAP.md). #271 merged at 0083d7ff25edecc85ecd6d8fc718b8e0c2213b92; 13/13 returned PR-head jobs PASS. #272 (2/5) remains design-only and CI pending.

**Compatibility closure:** #270 merged, 13/13 returned jobs, hosted Clang Debug 70/70, container Release 69 passed/1 skipped, 324 headers probes, host 5/container 4 capability passes. [API candidate](architecture/C-API-ABI-FFI-ROADMAP.md) design/compile-only pending CI.

**Compatibility checkpoint:** #269 merged, 13/13 returned jobs, 324/324 header probes, hosted Clang Debug 70/70, container Release 69 passed/1 skipped. [Final capability report candidate](upstream/UPSTREAM_COMPAT_REFERENCE.md) pending CI.

**Compatibility checkpoint:** #268 merged, 13/13 returned jobs, GNU C17 Debug/Release 70/70, hosted Clang Debug 70/70, sanitizer probe PASS, container Release 69 passed/1 skipped. [Header candidate](upstream/UPSTREAM_COMPAT_REFERENCE.md) pending CI.

**#268 failure:** Ubuntu job 139925552427 failed linking missing ASan archives; same-PR hosted runner correction CI pending.

**Compatibility checkpoint:** #267 merged, 13/13 returned jobs, GNU C17 Debug/Release 70/70, Clang C17 Release 69 passed/1 io_uring capability skipped. [Debug candidate](upstream/UPSTREAM_COMPAT_REFERENCE.md) pending CI.

**Compatibility checkpoint:** #266 merged, 13/13 returned jobs, GNU C17 Debug/Release 70/70, Clang three production builds and CLI checks PASS. [Full Clang suite candidate](upstream/UPSTREAM_COMPAT_REFERENCE.md) pending CI.

**Supply-chain closure:** #265 merged, 13/13 returned jobs, 70/70 C17 Debug/Release, seven APT and 11 gate fixtures PASS. [Compatibility candidate](upstream/UPSTREAM_COMPAT_REFERENCE.md) pending CI.

**Supply-chain checkpoint:** #264 merged, 15/15 returned jobs, 70/70 C17 Debug/Release, pinned checkout/build-push execution, seven APT cases and 35 references PASS. [Final candidate](security/SUPPLY_CHAIN_REFERENCE.md) pending CI.

**Supply-chain checkpoint:** #263 merged, 13/13 returned jobs, 70/70 C17 Debug/Release, seven APT cases and 35 direct inventory references PASS. [Action pin candidate](security/SUPPLY_CHAIN_REFERENCE.md) pending CI.

**Supply-chain checkpoint:** #262 merged, 13/13 returned jobs, 70/70 C17 Debug/Release and seven APT retry cases PASS. [Direct build-input inventory](security/SUPPLY_CHAIN_REFERENCE.md) pending CI.

**Supply-chain checkpoint:** #261 merged, 13/13 returned jobs, 70/70 C17 Debug/Release, Docker build/smoke PASS. [Retry oracle candidate](security/SUPPLY_CHAIN_REFERENCE.md) pending CI.

**Fault reference closure:** #260 merged, 13/13 returned jobs after Docker retry, 70/70 C17 Debug/Release, 36 faults and recoveries PASS. [Supply-chain candidate](security/SUPPLY_CHAIN_REFERENCE.md) pending CI.

**Fault checkpoint:** #259 merged, 13 completed jobs PASS, Makefile pending; #258 Ubuntu image pending; 70/70 C17 Debug/Release and 24 isolated faults PASS. [Final combined fault candidate](security/UPSTREAM_FAULT_REFERENCE.md) pending CI.

**Fault checkpoint:** #258 merged, 13 completed jobs PASS, Ubuntu image running, 70/70 C17 Debug/Release and three allocation faults PASS. [Acquisition candidate](security/UPSTREAM_FAULT_REFERENCE.md) pending CI.

**Fault checkpoint:** #257 merged, 13/13 returned exact-head jobs, 70/70 C17 Debug/Release, 12 read and three close fault cases PASS. [Allocation candidate](security/UPSTREAM_FAULT_REFERENCE.md) pending CI.

**Fault checkpoint:** #256 merged, 14/14 exact-head jobs, C17 Debug/Release 70/70, 12 read fault rollback/closure/retry cases PASS. [Close fault candidate](security/UPSTREAM_FAULT_REFERENCE.md) pending CI.

**Linux selftest closure:** #255 merged, 13/13 returned jobs, 69/69 C17 Debug/Release, 30 exec probes and 3 leak rejections PASS. [Fault injection candidate](security/UPSTREAM_FAULT_REFERENCE.md) pending CI.

**Linux selftest checkpoint:** #254 merged, 13/13 returned jobs, C17 Debug/Release 69/69, depth 20 and six deep exec probes PASS. [Final leak oracle candidate](security/FILESYSTEM-SCANNER-SECURITY.md) pending; production unchanged.

**Linux selftest checkpoint:** #253 merged, 12 completed jobs PASS, Ubuntu image running, C17 Debug/Release 69/69 and 18 exec probes PASS. [Deep lifecycle candidate](security/FILESYSTEM-SCANNER-SECURITY.md) pending; production unchanged.

**Linux selftest checkpoint:** #252 merged, 13/13 returned exact-head jobs, 69/69 C17 Debug/Release, three negative and three positive real exec probes PASS. [Abort lifecycle candidate](security/FILESYSTEM-SCANNER-SECURITY.md) pending; production unchanged.

**Linux selftest checkpoint:** #251 merged, 14/14 exact-head jobs and 69/69 C17 Debug/Release; prior #250 Ubuntu image PASS. [Actual exec inheritance candidate](security/FILESYSTEM-SCANNER-SECURITY.md) pending CI; production unchanged.

**CPU optimization checkpoint:** #250 merged, C17 Debug/Release 68/68, 82560 guard-page checks PASS. Ubuntu image job still pending at inspection; retain production classifier. [Linux selftest candidate](security/FILESYSTEM-SCANNER-SECURITY.md) pending CI.

**CPU optimization checkpoint:** #249 merged, 13/13 returned exact-head jobs, 68/68 C17 Debug/Release, dispatch policy and 660480 comparisons PASS; [final guard-page candidate](performance/CPU_OPT_REFERENCE.md) pending. Retain production classifier; no release claim.

**CPU optimization checkpoint:** cache layout scoped DONE 5/5 (#245 merged, 14/14 returned jobs; C17 Debug/Release 67/67); retain fixed layout. [Next CPU reference](performance/CPU_OPT_REFERENCE.md) is pending CI; no release claim.

**Cache-layout checkpoint:** [evidence](performance/CACHE_LAYOUT_REFERENCE.md) records #244 merged, 14/14 returned PR-head jobs and 67/67 C17 Release tests; final direct sanitizer candidate pending, retain production layout. No release or source-retirement safety claim.

**26 November 2026 — batch I/O scoped evaluation:** #240 merged, 13/13 returned PR-head jobs, 66/66 C17 Release and three controlled partial/error scenarios PASS. Scoped evaluation DONE 3/5; production scalar128k retained. [Batch evidence](performance/BATCH_IO_REFERENCE.md); next [cache layout](performance/CACHE_LAYOUT_REFERENCE.md). Native benchmark failure unresolved.

**26 November 2026 — async I/O scoped evaluation:** #237 merged, 13/13 returned PR-head jobs, 65/65 C17 Release and 32 raw io_uring completions PASS. Scoped raw UAPI evaluation DONE 4/5; actual liburing integration NOT EVALUATED. Production buffered mover retained. [Async evidence](performance/LINUX_URING_REFERENCE.md); next [batch I/O](performance/BATCH_IO_REFERENCE.md). Native benchmark push failure unresolved.

**26 November 2026 — Linux I/O scoped evaluation:** #233 merged, 13/13 returned PR-head jobs, 64/64 C17 Release and explicit-offset evidence PASS. Scoped evaluation DONE 3/5, production buffered transfer retained. [I/O evidence](performance/LINUX_IO_REFERENCE.md); next [io_uring capability](performance/LINUX_URING_REFERENCE.md). Integrated-main in progress; native benchmark push failure unresolved.

**26 November 2026 — watcher scoped evaluation:** #230 merged, 13/13 returned PR-head jobs, 63/63 C17 Release and watcher lifecycle PASS. Scoped evaluation DONE 3/5, production watcher retained. [Watcher evidence](performance/LINUX_WATCH_REFERENCE.md); next [I/O reference](performance/LINUX_IO_REFERENCE.md). Integrated-main in progress at inspection; separate native benchmark push failure unresolved.

**26 November 2026 — path scoped evaluation:** #227 merged, 13/13 returned PR-head jobs, C17 Release 63/63 and real UAPI error reference PASS (not skipped). Scoped evaluation DONE 3/5; production path handling retained. [Path evidence](performance/LINUX_PATH_REFERENCE.md); next [watcher reference](performance/LINUX_WATCH_REFERENCE.md). Integrated-main in progress at inspection, separate native benchmark push failure unresolved; FS-01B/REL-01 blocked.

**26 November 2026 — scanner scoped evaluation:** #224 merged, 13/13 returned exact-head PR jobs passed; C17 Release verbose evidence captured. Scoped evaluation DONE 4/5, production scanner retained. [Scanner evidence](performance/SCANNER_REFERENCE_BASELINE.md); next [path resolution](performance/LINUX_PATH_REFERENCE.md). Integrated-main CI partial at inspection; separate dispatch-only native benchmark push failure unresolved; FS-01B/REL-01 blocked.

**25 November 2026 — string scoped evaluation:** #220 merged, 13/13 returned exact-head jobs passed; FCJ4 TSan and integrated-main CI NOT VERIFIED. Scoped evaluation DONE 3/5, production libc classifier retained. [String evidence](performance/STRING_EXTENSION_BASELINE.md); next [scanner reference](performance/SCANNER_REFERENCE_BASELINE.md). Target-host and representative throughput remain unproven; FS-01B/REL-01 blocked.

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
