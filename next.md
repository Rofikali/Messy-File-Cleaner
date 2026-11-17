# Messy-File-Cleaner — Master Work Register

## LIVE BOARD — NOW / NEXT / LATER (authoritative task selection)

**Historical baseline reconciled 2026-11-17** against actual GitHub `main` `18661c8291a8afd2129573dfb57ca88146665995` after roadmap reconciliation [PR #97](https://github.com/Rofikali/Messy-File-Cleaner/pull/97) merged. RUST-01A [PR #96](https://github.com/Rofikali/Messy-File-Cleaner/pull/96) merged. [PR #95](https://github.com/Rofikali/Messy-File-Cleaner/pull/95) has **MERGED** the test-only RUST-01 **design document**; it has not implemented a C shim or any Rust FFI. The limited **RUST-01A C-only shim** is implemented in isolated tests and merged through PR #96. GitHub reported **no open PRs** before this follow-up PR was opened; recheck live state each run. No Rust FFI or production FCJ4 linkage is authorized. FS-01B-FIX and REL-01 remain P0 BLOCKED: the known same-UID source-retirement vulnerability is **not fixed**, and parser test coverage is not a filesystem safety claim. Verify actual PR/main/CI state on each future run.

**Latest TOOL-01 rust-check merge reconciliation (2026-11-18):** Verified `main` `e1ab35558240113934126196a57cc22f68ecd563` includes [PR #103](https://github.com/Rofikali/Messy-File-Cleaner/pull/103), reviewed head `1c07b11b09f7c4e5ca73835b0a38e98476888320`; zero open PRs at this checkpoint. `make rust-check` is **IMPLEMENTED / MERGED (NOT RELEASED)** within the existing Rust test-only crate, with Cargo fmt, Clippy and locked unit tests in Ubuntu 24.04 CI. Exact-head evidence: [Rust](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/44158446471) **PASS**, [C11/C17/C23 matrix](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/44158446455) **6 PASS**, [Docker](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/44158446574) **PASS**, [Makefile](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/44158446593) **PASS**: **9 PASS / 0 FAIL** at PR head; local tests **NOT RUN**. No production Rust/FFI, FCJ3/FCJ4 integration, filesystem safety fix or release. TOOL-01's three scoped commands (`make status`, `make self-check`, `make rust-check`) are merged; reassess any further TOOL-01 scope separately. FS-01B-FIX and REL-01 remain **P0 BLOCKED** by unresolved same-UID source-retirement risk. This documentation follow-up requires its **own** exact-head CI and review before merge.

**PERF-01A active implementation (2026-11-18):** A focused PR branch adds post-warmup and post-measurement preservation checks to the disposable dry-run benchmark fixture, negative regression cases (removed, changed, extra, symlink replacement) and Ubuntu 24.04 CI execution. **Maturity: IMPLEMENTED ON BRANCH; delivery: PR OPEN / exact-head CI NOT YET VERIFIED; local full repository tests NOT RUN.** The existing benchmark does not run customer-data moves, and these checks do not mitigate FS-01B-FIX or authorize release. Next: inspect exact-head CI and merge only under GOV-02 gates. Full PERF-01 workload/resource benchmarking remains PLANNED.

**Product mission:** A safe, explainable Linux/POSIX file organizer. The current product is a C17 CLI, with C11/C23 compatibility tests, Docker dry-run / explicitly writable execute, and legacy journal-backed moves. FCJ3/FCJ4 are **test-only**. Rust is **test-only**, not connected via FFI. Broader release, public C SDK, GUI/API and monetization are not demonstrated. User data protection outranks speed and features.

**Navigation:** [GOV-02 owner-authorized PR review/merge policy](docs/AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md) · [FS-01B-GUARD interim mitigation](docs/security/FS_01B_SHARED_SOURCE_GUARD.md) · [Proposed FS-01B-FIX ADR](docs/architecture/ADR_FS_01B_SOURCE_RETIREMENT_BOUNDARY.md) · [Positive acceptance plan](docs/quality/FS_01B_FIX_SECURITY_ACCEPTANCE_PLAN.md) · [FS-01B known-risk characterization](docs/quality/FS_01B_RETIREMENT_RACE_CHARACTERIZATION.md) · [FS-01A production filesystem safety review](docs/security/FS_01_PRODUCTION_FILESYSTEM_AUDIT_2026-11-16.md) · [Component completion inventory](docs/PROJECT_COMPLETION_AND_STATUS_INVENTORY.md) · [AGENTS](AGENTS.md) · [skills](skills.md) · [docs index](docs/README.md) · [CI/Makefile operations](docs/CI_CD_AND_MAKEFILE_OPERATIONS.md) · [handoff](docs/AGENT_CONTINUATION_AND_HANDOFF.md) · [product/business](docs/PRODUCT_AND_BUSINESS_STRATEGY.md).


**TOOL-01 verified merge checkpoint (2026-11-17):** [PR #101](https://github.com/Rofikali/Messy-File-Cleaner/pull/101) **MERGED** at `27a06701c8203a3279ee647543ce5b485bbdc405` from exact reviewed head `cd156241ebfd91ef5c0c89c881e1bb530f08043f`. `make self-check` is **IMPLEMENTED / MERGED** and explicitly ran successfully in Ubuntu Makefile workflow [44124849390](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/44124849390); C11/C17/C23 Debug/Release **6 PASS** ([44124849188](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/44124849188)), Docker **1 PASS** ([44124849204](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/44124849204)), Makefile **1 PASS**: **8 PASS / 0 FAIL**. Local repository tests **NOT RUN**. `make status` is also merged (#99); overall TOOL-01 remains **PARTIAL**, with separate test-only `make rust-check` **IMPLEMENTED ON BRANCH / CI PENDING**. The self-check inspects Make recipes, not real filesystem safety. FS-01B-FIX and REL-01 remain **P0 BLOCKED**, same-UID source-retirement risk unresolved; no release claim.

**TOOL-01 checkpoint (2026-11-17):** [PR #99](https://github.com/Rofikali/Messy-File-Cleaner/pull/99) **MERGED** at `114e934a42c9d8b9fd7d3cb26fd2e3b8b3909e90` (verified identical to `main`); read-only `make status` is **IMPLEMENTED**, while the overall TOOL-01 milestone is **PARTIAL**. Exact PR head `cd4b86a7b6aa010f4cff6cf5c4b968e1e99054b8` passed 8/8 jobs (six C11/C17/C23 Debug/Release CMake, Docker smoke, Makefile validation); local tests **NOT RUN**. `make self-check` is implemented and merged in PR #101; `make rust-check` remains **NOT_STARTED**. The known same-UID P0 risk, FS-01B-FIX and REL-01 remain blocked; no production safety or release claim.


**TOOL-01 rust-check checkpoint (2026-11-18; PR pending):** Base `cad6b4684b1170dcfc9cb562c31546ab7e187d08`. `make rust-check` is **IMPLEMENTED ON BRANCH / PR PENDING**, wrapping only the existing dependency-free `tests/rust/fcj4-test-support` crate with `cargo fmt --all -- --check`, `cargo clippy --locked --all-targets -- -D warnings`, and `cargo test --locked`. No FFI, production link, C journal/recovery or file-operation changes. Local Rust/C11/C17/C23/Docker tests **NOT RUN**; exact-head CI **NOT YET VERIFIED**. This does not fix FS-01B-FIX or unblock REL-01; same-UID source-retirement P0 remains. Next: verify PR diff and CI on its exact head; check that Rust workflow triggers on Makefile changes or run isolated Rust verification before any merge.

### Live work lanes

| Lane | Milestone / accurate current status | Next bounded action | Acceptance and gating |
| --- | --- | --- | --- |
| **FINISHED — documentation board** | **ROADMAP-02 — MERGED PR #87, merge `9dfb2ac`** | IMGENGINE-style live NOW/NEXT/LATER plus completed/partial/test-only inventory now exists. | Four PR-head workflows passed at `df48832`; no runtime release implied. |
| **FINISHED — FS-01A source review** | **FS-01A P0 — MERGED PR #88, merge `5e41a6`** | Read-only mover/journal audit and standing `go ahead` governance. | Four PR-head CI workflows passed at `715dc7d`; FS-01 P0 release gate remains OPEN. |
| **FINISHED — confirmed safety risk evidence** | **FS-01B P0 — MERGED PR #89, merge `fc09ae2`** | Test-only controlled quarantine-entry substitution reproducibly unlinks wrong entry while preserving fixture bytes via hard links. | 5 triggered workflows passed at `4d03027`, including 29/29 C17 Debug CTests. **PASS confirms vulnerability, not fix**. |
| **FINISHED — decision package** | **FS-01B-FIX-DESIGN P0 — MERGED PR #90, merge `54d70b`; ADR PROPOSED** | Alternatives, private quarantine and fail-closed options, 12 positive/crash acceptance cases documented. | Three triggered PR-head checks passed; **documentation merge is not design approval, production fix or release sign-off**. |
| **FINISHED — partial risk containment** | **FS-01B-GUARD P0 — MERGED PR #91, merge `23a2d2e`** | Production parent-directory ownership/mode guard and disposable preservation regression are merged. | Five PR-head workflows passed at `c8dd667`; existing same-UID known-risk test still demonstrates residual P0 risk, so **full fix not done**. |
| **FINISHED — owner-authorized merge workflow** | **GOV-02 P1 — IMPLEMENTED / MERGED PR #93, main `cfee1bf`** | Policy and AGENTS/skills/handoff/index alignment landed; follow exact-head review/merge gates for future eligible PRs. | Documentation-only governance, **not** independent review approval, filesystem safety remediation, or release authorization. [Merge policy](docs/AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md). |
| **NEXT — P0 complete source-retirement fix** | **FS-01B-FIX P0 — BLOCKED on accepted trust/crash design and positive regression** | Choose private pinned quarantine or no-unlink behavior, defend same-UID/writable-parent assumptions, and resolve scanner/crash-orphan visibility. Convert existing known-risk test to **positive preservation** for approved attacker model. | C11/C17/C23 Debug/Release, fault/crash and negative-fixture tests, no unowned unlink, exact-head PR CI and explicit owner review; FS-01/REL-01 remain independent gates. |
| **NOW — separate release planning** | **REL-01 P0 release gate — BLOCKED** | Inventory supported Linux/fs targets, backup/restore, distribution license, build provenance, operational rollback and release evidence. | Owner decisions, complete exact-candidate safety validation; CI pass alone is not release certification. |
| **FINISHED — RUST-01 design review** | **RUST-01 DESIGN_ONLY / MERGED PR #95, main `3a978a8`** | Approved boundary *for review* proposes C-owned test parser shim, Rust FFI isolated from production. | Design merge is not a production FFI or filesystem safety authorization. |
| **FINISHED — test-only C parser seam** | **RUST-01A P1 — IMPLEMENTED IN TESTS / MERGED PR #96, main `0f6694a`** | Private C FCJ4 parse-bytes shim, deterministic outputs, parser-lifetime ownership and negative tests now exist only under `BUILD_TESTING`. | [Shim contract](docs/quality/RUST_01A_C_PARSER_TEST_SHIM.md); no Rust unsafe changes or production link. PR-head workflow results must be checked separately; merge is not release evidence. |
| **FINISHED — RUST-01A roadmap reconciliation** | **RUST-01A-STATUS — MERGED PR #97, main `18661c8291a8afd2129573dfb57ca88146665995`** | Board records PR #96 C-only shim as test-implemented and merged; PR #97 was documentation-only. | No Rust FFI, production link, filesystem safety fix or release claim; prior CI evidence belongs to the reviewed PR head, not this follow-up. |
| **NEXT — separate Rust FFI review** | **RUST-01B P1 — BLOCKED on explicit Rust unsafe boundary approval (RUST-01A merged)** | Only after the C shim passes, review a separately isolated Rust `extern "C"` test harness with documented ownership, negative cases and no production link. | Rust fmt/clippy/test, C unit+sanitizer gates and link inspection, no change to existing `#![forbid(unsafe_code)]` crate. |
| **NEXT — Rust property/fuzz** | **RUST-02 P1 — BLOCKED on RUST-01** | Extend test-only Rust harness with deterministic valid/invalid byte generation and bounded parser fuzz/property tests. | Minimal reproducible counterexamples, C regression fixtures and CI, no second FCJ4 grammar or unsafe prod link. |
| **FINISHED — scoped project tooling** | **TOOL-01 P2 — IMPLEMENTED / MERGED PRs #99, #101, #103** | `make self-check` merged in #101; test-only `make rust-check` merged in #103 (9/9 exact-head CI). No production Rust/FFI or safety certification. Preserve Debug default and explicit real-move guard. | PR #99 head `cd4b86a` passed 8/8 CI jobs; no new safety certification. Future changes need their own exact-head checks and isolated refusal/no-user-mutation tests. |
| **NEXT — reproducible benchmarks** | **PERF-01 P2 — PLANNED** | Representative 5k/100k/optional manual 1m disposable scan and real-move benchmarks with memory and filesystem context. | Exact SHA/runner, median/tails where supported, RSS/disk/inodes/CPU, no nanosecond claims. |
| **LATER — artifact cleanup choice** | **HYG-02 P2 — OWNER DECISION OPEN** | Privately inspect tracked binary `perf.data` and legacy `gprof.txt`; decide archival versus removal from HEAD. | Specific owner approval before `git rm` or history modification; ignored paths alone do not untrack. |
| **LATER — reusable engine** | **ABI-01 P2 — DESIGN_ONLY / DEFERRED** | Find independent C consumer; design opaque engine lifecycle and stable API then `.a` and optionally `.so`. | License, external consumer, exact ABI/ownership/fault/link and compatibility review. |
| **LATER — product expansion** | **BIZ-01 P2 — HYPOTHESIS**, GUI/API/SaaS **NOT_STARTED** | Interview Linux users, validate file safety advantage, activation/support costs; decide whether any UI, desktop, paid or hosted channel has a real need. | Observed customer evidence and owner decision; no assumed pricing or infrastructure. |
| **LATER — FCJ4 prod adoption** | **FCJ4-PROD — BLOCKED, NOT AUTHORIZED** | Only if FS-01, durable object evidence and compatibility design justify another separate decision. | Approved ADR, independent threat/recovery and actual crash evidence; never just link test-only objects into production. |

**Owner shorthand and standing merge authorization:** **"go ahead" / "continue" / "next milestone"** selects the highest-priority dependency-unblocked READY item from the **live** board, verifies current GitHub PRs/merges/CI, and progresses one bounded task without re-requesting a task prompt. **Separately, on 2026-11-17 the owner authorized the existing hourly autopilot to review, correct and MERGE eligible PRs** after full exact-head checks and branch protections. Follow [GOV-02](docs/AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md) and [AGENTS](AGENTS.md); the standing merge permission does **not** authorize unapproved P0 filesystem/recovery changes, real file deletion, history rewriting, secrets/branch-protection bypass, paid actions or release. Markdown itself does not start or schedule an agent.

**No-repeat rule:** A previously merged PR must never be repeated because a retained historical note calls it “next.” After each milestone check actual `main`, PR head/merge SHA, and required checks; a CI-passing PR becomes `READY_FOR_REVIEW`, not `MERGED`. A merged test module does not mean production adoption; a clean PR does not mean RELEASED.

### Status dimensions

- **Feature maturity:** `NOT_STARTED`, `DESIGN_ONLY`, `STARTED`, `PARTIAL`, `IMPLEMENTED`, `LEGACY`, `DEFERRED`, `SUPERSEDED`.
- **Delivery/evidence:** `LOCAL_ONLY`, `PR_OPEN`, `CI_PENDING`, `RECORDED_TESTS`, `READY_FOR_REVIEW`, `MERGED`, `RELEASE_CANDIDATE`, `RELEASED`, `UNKNOWN`.
- **Constraints:** `READY` only if prerequisites, scope and authority are established; `BLOCKED` when dependent on an unresolved gate or explicit owner decision; `P0` risk labels do not authorize irreversible actions.

Every task checkpoint must include **ID, intended outcome, owner decision, exact base/head/merge SHAs, prerequisites, status in both dimensions, relevant changed files, tests executed/NOT RUN, CI links and conclusions, negative/security cases, cost/resource considerations, residual risk, and next action**. Full logs stay in CI; minimal failed-step excerpts enter agent context. Docs cannot restart an inactive agent.

### Finished / implemented (current verified scope, not global certification)

| ID / component | Result in main | Evidence and open qualification |
| --- | --- | --- |
| DOC-01 | **MERGED** PR #83, merge `6529bd9` | Initial governance, `next.md` and agents docs |
| SAFE-01 | **MERGED** PR #84, merge `0184cc1` | Read-only default Compose source; explicit writable execution; PR-head Docker/CMake/Makefile pass |
| HYGIENE-01 | **MERGED** PR #85, merge `d3fd576` | Retention/ignore policy; tracked profiles **not deleted** |
| GOV-01 | **MERGED** PR #86, merge `30f14e9` | C production and Rust test-only policy; 4/4 triggered CI workflows passed at PR head |
| ROADMAP-02 | **MERGED** PR #87, merge `9dfb2ac` | Live NOW/NEXT/LATER and separate per-component maturity inventory; four PR-head workflows passed |
| FS-01A | **MERGED** PR #88, merge `5e41a6` | Read-only source/journal audit and `go ahead` continuation rule; **not** a production fix |
| FS-01B | **MERGED** PR #89, merge `fc09ae2` | Deterministic C `FC_TESTING` check/use characterization; **reproduces unsafe unlink**, not a production fix |
| FS-01B-FIX-DESIGN | **MERGED** PR #90, merge `54d70b` | PROPOSED ADR and positive acceptance plan only; no accepted crash/privilege authority or runtime fix |
| FS-01B-GUARD | **MERGED** PR #91, merge `23a2d2e` | Partial production containment, five PR-head workflows passed at `c8dd667`; same-UID race and release blockers remain |
| ROADMAP-03 | **MERGED** PR #92, merge `8c242c1` | Reconciles PR #91 status and residual P0 blocker; three triggered PR-head workflows passed at `68abd5a` |
| GOV-02 | **IMPLEMENTED / MERGED** PR #93, merge `cfee1bf` | Owner-authorized gated PR merge policy and aligned governance docs; PR-head workflow details not revalidated in this reconciliation; no runtime or release certification |
| FCJ4 restart tests | **MERGED** PR #80, merge `4362c41` | Malformed/truncated test scope; **not** production FCJ4 recovery |
| Rust test scaffold | **MERGED** PRs #81 / #82 | Toolchain/Cargo crate; parser FFI **not implemented** |
| RUST-01 design | **DESIGN_ONLY / MERGED** PR #95, merge `3a978a8` | Test-only shim proposal; C shim subsequently landed in PR #96, Rust FFI still absent |
| RUST-01A C parser shim | **IMPLEMENTED IN TESTS / MERGED** PR #96, main `0f6694a` | Private C-only parser shim and negative CTest cases; no Rust FFI, product link or P0 filesystem remediation |
| Existing C17 CLI | **IMPLEMENTED, merged** | Scanner/classifier/worker/mover/legacy journal and 28 declared CTest cases; no broad release certification |
| FCJ3/FCJ4 components | **IMPLEMENTED IN TESTS, merged** | Parser/writer/staging/append and fault tests; **not linked into product** |
| Docker/CI/baseline | **IMPLEMENTED, recorded** | Eight workflows with distinct triggers; local Makefile, 2026-11-04 performance evidence; no deployment pipeline |

### Started / half complete / unfinished customer journeys

| Workflow | Current maturity | Missing for full customer outcome |
| --- | --- | --- |
| Journal recovery under hostile/crash conditions | **PARTIAL evidence** | Verify preserved identity/durability and supported FS, no unsupported automatic deletion |
| Source retirement under concurrent writable-directory substitution | **CONFIRMED P0 CHECK/USE RISK; FS-01B PR #89 MERGED** | FS-01B-GUARD merged mitigation narrows other-UID shared-writable source exposure, but same-UID race persists; **full FS-01B-FIX not implemented** |
| FCJ4 end-to-end production moves | **PARTIAL test-only building blocks / production NOT_STARTED** | Real durable journal/mover integration is blocked and unapproved |
| Rust-based FCJ4 parser verification | **STARTED** | Safe C test seam, Rust FFI, property/fuzz run and regression evidence |
| Public use/distribution | **PARTIAL preparation / RELEASE BLOCKED** | License/support matrix, backups, release artifacts, rollback and actual environment validation |
| Large-scale and worker tuning | **PARTIAL benchmark evidence** | Representative machine/filesystem resource and workload results |
| Stable API/SDK and UI/cloud | **DESIGN_ONLY or NOT_STARTED** | Customer evidence and separately reviewed architecture |

For detailed source paths and constraints see the [component-level inventory](docs/PROJECT_COMPLETION_AND_STATUS_INVENTORY.md). This deliberately avoids false completion percentages.

---

## Historical work inventory and dated checkpoints (preserved, **not** the live task selector)

> The material below is retained from prior `next.md` iterations for traceability. **Its older "current", "READY", "not merged" and execution-order labels may now be stale.** The live NOW / NEXT / LATER board above is authoritative; review exact GitHub evidence before changing code. Do not delete historical evidence simply to make the file look shorter.


> **Source-of-truth dashboard.** Reconciled 2026-11-16 against `main` `d3fd576dbd15f184520e954e23c8a0593934e3df` after [HYGIENE-01 PR #85](https://github.com/Rofikali/Messy-File-Cleaner/pull/85) merged. Its PR-head CMake, Docker and Makefile CI checks passed. GOV-01 is a documentation review candidate, not merged. Refresh current GitHub/CI state before selecting work.

## Product north star

A dependable **local Linux/POSIX file organizer** with clear dry-run, explicit classification, predictable behavior, conservative journal/recovery, low resource cost, and transparent error reporting. Keep user data preservation first. Current deliverable: C17 `file-cleaner` CLI; C11/C23 compatibility; Docker packaging. Rust exists only in isolated test support. A native SDK, API server, GUI, paid product or new journal deployment are **not** implemented or authorized.

Read [project identity and business strategy](docs/PRODUCT_AND_BUSINESS_STRATEGY.md), [engineering operating model](docs/ENGINEERING_OPERATING_MODEL.md), [CI/CD and Makefile guide](docs/CI_CD_AND_MAKEFILE_OPERATIONS.md), [handoff runbook](docs/AGENT_CONTINUATION_AND_HANDOFF.md), [AGENTS.md](AGENTS.md), [skills.md](skills.md), and [documentation map](docs/README.md).

## Status vocabulary and evidence

`PLANNED` / `READY` / `IN_PROGRESS` / `PR_OPEN` / `CI_PENDING` / `READY_FOR_REVIEW` / `BLOCKED` / `MERGED` / `RELEASE_CANDIDATE` / `RELEASED`. `DESIGN_ONLY` means no implementation. These states are **not interchangeable**. An unreviewed passing PR is not merged; merged code is not automatically released; a parser acceptance is not filesystem authenticity or recovery permission. Every task keeps a base SHA, reviewed PR SHA, merge SHA, exact workflow evidence, limitations and next action when available.

## Current milestone board — choose by dependency, not aspiration

| ID / priority | Status at baseline | Bounded next action | Completion gate |
| --- | --- | --- | --- |
| DOC-01 / P0 | **MERGED** — [PR #83](https://github.com/Rofikali/Messy-File-Cleaner/pull/83), merge `6529bd9` | IMGENGINE-style governance adapted to local C17 product; docs only. | Four PR-head CI workflows passed; no production release implied. |
| SAFE-01 / P0 | **MERGED** — [PR #84](https://github.com/Rofikali/Messy-File-Cleaner/pull/84), merge `0184cc1` | Default Compose source read-only, opt-in writable execution overlay and disposable Docker checks. | PR head `92c1d3d`: Docker, CMake and Makefile workflows passed; standalone Ubuntu job not triggered. |
| HYGIENE-01 / P1 | **MERGED** — [PR #85](https://github.com/Rofikali/Messy-File-Cleaner/pull/85), merge `d3fd576` | Document profile provenance/retention and ignore generated outputs; tracked artifacts preserved. | CMake, Docker and Makefile PR-head CI passed. Owner decision on historical artifact removal remains separate. |
| GOV-01 / P1 | **IN_PROGRESS — documentation review candidate; not merged** | Reconcile README, `AGENTS.md`, `skills.md`, uppercase `SKILLS.md` and playbook into one authority hierarchy; C17 production with Rust test-only exception and explicit merge approval. | Review policy diff and PR-head CI; no code, Makefile, CI or journal changes; owner approves merge separately. |
| RUST-01 / P1 | **READY only after preceding safety decisions** | Design test-only FCJ4 C shim and Rust FFI; no production link. | ABI/ownership/threat review, negative tests, C11/C17/C23 + Rust/CI, production isolation proof. |
| RUST-02 / P1 | **BLOCKED on RUST-01 merge** | Deterministic Rust property tests, then bounded fuzzing against existing C FCJ4 parser, with minimized C regressions. | Reproducible findings; no duplicate FCJ4 grammar or production C/Rust link. |
| FS-01 / P0 release gate | **OPEN / REVIEW** | Audit real move, journal, crash boundaries, TOCTOU and identity consistency on target filesystems before broader deployment. | Full adversarial fixtures, fault injection, recovery evidence; separately approved mutation/recovery changes. |
| PERF-01 / P2 | **PLANNED** | Expand representative bounded scan/move benchmarks incl. 5k/100k/1M *disposable fixtures where resources allow*, warm/cold cache and worker sweep. | Median/p95/p99 where statistically supported; CPU/RSS/syscalls/filesystem metadata; no nanosecond promise. |
| ABI-01 / P2 | **DESIGN_ONLY** | Identify actual external consumer; draft versioned opaque C API and possible static-library packaging, no public ABI yet. | Independent consumer/ownership/compatibility tests, license and distribution decision; new ADR. |
| REL-01 / P0 for release | **BLOCKED** | Define supported filesystems, backup/recovery procedures, release support matrix, install/license, versioning, release/canary/rollback verification. | Explicit owner go/no-go; exact candidate tests and operational evidence. |
| BIZ-01 / P2 | **PLANNED** | Interview real Linux users/teams to validate file-organization use case, safety differentiation, support expectations and distribution. | Record qualitative evidence; pricing/distribution decisions only after demand. |

**P0 is a priority label, not permission to implement destructive recovery.** FS-01 and REL-01 are release risks; no new production FCJ4 flow is authorized.

## Verified/implemented versus planned

### Implemented in repository (not blanket certification)

- C17 production CLI and modular scanner, classifier, bounded queue/worker, mover, FCJ2/legacy journal and conservative recovery; explicit CMake production source list.
- C11/C17/C23 build/test paths, Debug ASan/UBSan, Release, Ubuntu 24.04 CI, Docker non-root smoke, Makefile wrapper and separate TSan test job.
- FCJ3 read-only/reporting and staging experiments, FCJ4 parser, serializer/writer-state and Linux append adapter: **test-only, intentionally excluded from production**.
- Rust 1.99.1 pinned test-support crate under `tests/rust/` with byte-input tests, no FFI or product library. Accepted strategy PR #81 and merged scaffold PR #82.
- Historical 2026-11-04 benchmark evidence: see [CI baseline](docs/performance/CI-BASELINE-2026-11-04.md); results are workload-specific.

### Evidence checkpoint (do not automatically transplant to later SHAs)

- [PR #81](https://github.com/Rofikali/Messy-File-Cleaner/pull/81) merged at `57547d712f6eac919dbf50b8e44b6b00ca090de7`.
- [PR #82](https://github.com/Rofikali/Messy-File-Cleaner/pull/82) merged at `559ba1e4e747f7c8b1fed5741c95c5f429c50557`; its PR head `3e0f46ece485a71d0b907afd2c44db5fa4217d6e` had successful Rust, CMake, Makefile, Ubuntu 24.04 and Docker workflow runs. Check current PR/head separately.
- [PR #80](https://github.com/Rofikali/Messy-File-Cleaner/pull/80) merged earlier with recorded malformed/truncated restart coverage. Do not call this live production FCJ4 recovery.
- Main baseline above is a GitHub source snapshot, not a new local test run. Release readiness remains **unproven**.

## Execution order

1. **DOC-01 merged** (PR #83), **SAFE-01 merged** (PR #84), and **HYGIENE-01 merged** (PR #85); preserve their governance, Compose safety and retention policies.
2. Review GOV-01 policy-alignment PR and its CI; do not start a dependent milestone until it is merged.
3. Reassess FS-01 release-safety risks before authorizing implementation of a narrow Rust test-only shim (RUST-01); retain separation from production.
4. Reassess FS-01 safety/release risks; advance RUST-01 only after safety gates and accepted test-shim contract.
5. Start PERF-01, ABI-01 and BIZ-01 when dependencies, customer evidence, and resources justify them.
6. Never start a dependent branch on an unmerged PR; stop if an unverified security, journal, ABI, owner or release gate is unresolved.

## No automation illusion

A committed agent instruction is not an always-running agent. At a new session: read current GitHub/main, `next.md`, relevant contracts, select one READY item, prepare one bounded PR, inspect exact-SHA CI, and leave a checkpoint. Never merge, deploy, charge, delete branches or modify real user data without explicit approval. Use [continuation runbook](docs/AGENT_CONTINUATION_AND_HANDOFF.md).
