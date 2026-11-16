# Messy-File-Cleaner — Master Work Register

## LIVE BOARD — NOW / NEXT / LATER (authoritative task selection)

**Reconciled 2026-11-16** against GitHub `main` `5e41a6ba1ab2a6839c1156cb7959fcaa20eccd6d`. [FS-01A PR #88](https://github.com/Rofikali/Messy-File-Cleaner/pull/88) is **MERGED** at this SHA; its PR head `715dc7d73d8af92093d06c3b96f729fa92a6fd14` passed CMake, Docker, Makefile and Ubuntu 24.04 CI. No open PRs were found when starting FS-01B. **FS-01B is a test-only known-risk characterization in review**, not a fix. Current P0 production source-retirement and release safety remains **BLOCKED** pending the separately reviewed solution. Refresh GitHub/main/PR/check state before each task.

**Product mission:** A safe, explainable Linux/POSIX file organizer. The current product is a C17 CLI, with C11/C23 compatibility tests, Docker dry-run / explicitly writable execute, and legacy journal-backed moves. FCJ3/FCJ4 are **test-only**. Rust is **test-only**, not connected via FFI. Broader release, public C SDK, GUI/API and monetization are not demonstrated. User data protection outranks speed and features.

**Navigation:** [FS-01B known-risk characterization](docs/quality/FS_01B_RETIREMENT_RACE_CHARACTERIZATION.md) · [FS-01A production filesystem safety review](docs/security/FS_01_PRODUCTION_FILESYSTEM_AUDIT_2026-11-16.md) · [Component completion inventory](docs/PROJECT_COMPLETION_AND_STATUS_INVENTORY.md) · [AGENTS](AGENTS.md) · [skills](skills.md) · [docs index](docs/README.md) · [CI/Makefile operations](docs/CI_CD_AND_MAKEFILE_OPERATIONS.md) · [handoff](docs/AGENT_CONTINUATION_AND_HANDOFF.md) · [product/business](docs/PRODUCT_AND_BUSINESS_STRATEGY.md).

### Live work lanes

| Lane | Milestone / accurate current status | Next bounded action | Acceptance and gating |
| --- | --- | --- | --- |
| **FINISHED — documentation board** | **ROADMAP-02 — MERGED PR #87, merge `9dfb2ac`** | IMGENGINE-style live NOW/NEXT/LATER plus completed/partial/test-only inventory now exists. | Four PR-head workflows passed at `df48832`; no runtime release implied. |
| **FINISHED — FS-01A source review** | **FS-01A P0 — MERGED PR #88, merge `5e41a6`** | Read-only mover/journal audit and standing `go ahead` governance. | Four PR-head CI workflows passed at `715dc7d`; FS-01 P0 release gate remains OPEN. |
| **NOW — controlled source-retirement evidence** | **FS-01B P0 — IN_PROGRESS, test-only known-risk PR candidate** | Reproduce post-`fstatat`/pre-`unlinkat` quarantine-name substitution using only private fixtures and retained backup link. | [Test interpretation](docs/quality/FS_01B_RETIREMENT_RACE_CHARACTERIZATION.md), exact-head C11/C17/C23 and CI; **PASS proves the risk is reproduced, not fixed.** |
| **NOW — production source-retirement safety blocker** | **FS-01B-FIX P0 — BLOCKED, requires separate safety design/review** | Design a race-resilient source retirement/private quarantine with object ownership, failure/crash behavior and regression criteria. No implementation in FS-01B. | Owner-approved bounded design, positive preservation tests, required CI and supported filesystem evidence before any release claim. |
| **NOW — separate release planning** | **REL-01 P0 release gate — BLOCKED** | Inventory supported Linux/fs targets, backup/restore, distribution license, build provenance, operational rollback and release evidence. | Owner decisions, complete exact-candidate safety validation; CI pass alone is not release certification. |
| **NEXT — isolated Rust test seam** | **RUST-01 P1 — PLANNED / needs explicit test-shim design review** | Propose smallest C parser-only test shim and Rust `extern "C"` verification, with ownership, bounds, error and link-isolation contracts. | Separate bounded PR, C11/C17/C23, sanitizer + Rust fmt/clippy/test, negative inputs, production binary isolation, owner approval. |
| **NEXT — Rust property/fuzz** | **RUST-02 P1 — BLOCKED on RUST-01** | Extend test-only Rust harness with deterministic valid/invalid byte generation and bounded parser fuzz/property tests. | Minimal reproducible counterexamples, C regression fixtures and CI, no second FCJ4 grammar or unsafe prod link. |
| **NEXT — project tooling** | **TOOL-01 P2 — PROPOSED** | Consider focused safe `make status`, `make self-check`, `make rust-check`; preserve existing `make` Debug default unless separately decided. | Refusal and no-user-mutation tests, existing full required CI, scoped cost review. |
| **NEXT — reproducible benchmarks** | **PERF-01 P2 — PLANNED** | Representative 5k/100k/optional manual 1m disposable scan and real-move benchmarks with memory and filesystem context. | Exact SHA/runner, median/tails where supported, RSS/disk/inodes/CPU, no nanosecond claims. |
| **LATER — artifact cleanup choice** | **HYG-02 P2 — OWNER DECISION OPEN** | Privately inspect tracked binary `perf.data` and legacy `gprof.txt`; decide archival versus removal from HEAD. | Specific owner approval before `git rm` or history modification; ignored paths alone do not untrack. |
| **LATER — reusable engine** | **ABI-01 P2 — DESIGN_ONLY / DEFERRED** | Find independent C consumer; design opaque engine lifecycle and stable API then `.a` and optionally `.so`. | License, external consumer, exact ABI/ownership/fault/link and compatibility review. |
| **LATER — product expansion** | **BIZ-01 P2 — HYPOTHESIS**, GUI/API/SaaS **NOT_STARTED** | Interview Linux users, validate file safety advantage, activation/support costs; decide whether any UI, desktop, paid or hosted channel has a real need. | Observed customer evidence and owner decision; no assumed pricing or infrastructure. |
| **LATER — FCJ4 prod adoption** | **FCJ4-PROD — BLOCKED, NOT AUTHORIZED** | Only if FS-01, durable object evidence and compatibility design justify another separate decision. | Approved ADR, independent threat/recovery and actual crash evidence; never just link test-only objects into production. |

**Owner shorthand:** When the owner says **"go ahead"**, **"go ahead next"**, **"continue"** or **"next milestone"**, select the highest-priority dependency-unblocked READY item from this **live** board and begin its bounded, non-destructive review/implementation PR without asking for a new copy-paste prompt. First verify current GitHub PRs/merges/CI; fix the existing PR instead of duplicating it. If top item is blocked, explain the gate and choose another independent READY item. **Never infer authorization to merge, deploy, delete branches or real files, spend money, change secrets, weaken checks, or activate production FCJ4.** Docs cannot restart a stopped agent. See [AGENTS](AGENTS.md) and [continuation runbook](docs/AGENT_CONTINUATION_AND_HANDOFF.md).

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
| FCJ4 restart tests | **MERGED** PR #80, merge `4362c41` | Malformed/truncated test scope; **not** production FCJ4 recovery |
| Rust test scaffold | **MERGED** PRs #81 / #82 | Toolchain/Cargo crate; parser FFI **not implemented** |
| Existing C17 CLI | **IMPLEMENTED, merged** | Scanner/classifier/worker/mover/legacy journal and 28 declared CTest cases; no broad release certification |
| FCJ3/FCJ4 components | **IMPLEMENTED IN TESTS, merged** | Parser/writer/staging/append and fault tests; **not linked into product** |
| Docker/CI/baseline | **IMPLEMENTED, recorded** | Eight workflows with distinct triggers; local Makefile, 2026-11-04 performance evidence; no deployment pipeline |

### Started / half complete / unfinished customer journeys

| Workflow | Current maturity | Missing for full customer outcome |
| --- | --- | --- |
| Journal recovery under hostile/crash conditions | **PARTIAL evidence** | Verify preserved identity/durability and supported FS, no unsupported automatic deletion |
| Source retirement under concurrent writable-directory substitution | **KNOWN P0 CHECK/USE RISK; FS-01B test characterization candidate** | Data-preserving test fixture can reproduce deletion of a swapped entry; production fix/positive regression is not implemented |
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
