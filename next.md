# Messy-File-Cleaner — Living Work Register

> **Source-of-truth dashboard.** Reconciled 2026-11-16 against `main` `6529bd9bb9ce5374ce2c065d61bc11b665eafa40` after [PR #83](https://github.com/Rofikali/Messy-File-Cleaner/pull/83) merged (its CMake, Docker, Makefile and Ubuntu PR-head workflows completed successfully). This file tracks decisions and delivery; SAFE-01 is a separate review candidate, **not** merged, and no new tests have been run merely by editing this file. Reconcile current GitHub/CI state before taking the next task.

## Product north star

A dependable **local Linux/POSIX file organizer** with clear dry-run, explicit classification, predictable behavior, conservative journal/recovery, low resource cost, and transparent error reporting. Keep user data preservation first. Current deliverable: C17 `file-cleaner` CLI; C11/C23 compatibility; Docker packaging. Rust exists only in isolated test support. A native SDK, API server, GUI, paid product or new journal deployment are **not** implemented or authorized.

Read [project identity and business strategy](docs/PRODUCT_AND_BUSINESS_STRATEGY.md), [engineering operating model](docs/ENGINEERING_OPERATING_MODEL.md), [CI/CD and Makefile guide](docs/CI_CD_AND_MAKEFILE_OPERATIONS.md), [handoff runbook](docs/AGENT_CONTINUATION_AND_HANDOFF.md), [AGENTS.md](AGENTS.md), [skills.md](skills.md), and [documentation map](docs/README.md).

## Status vocabulary and evidence

`PLANNED` / `READY` / `IN_PROGRESS` / `PR_OPEN` / `CI_PENDING` / `READY_FOR_REVIEW` / `BLOCKED` / `MERGED` / `RELEASE_CANDIDATE` / `RELEASED`. `DESIGN_ONLY` means no implementation. These states are **not interchangeable**. An unreviewed passing PR is not merged; merged code is not automatically released; a parser acceptance is not filesystem authenticity or recovery permission. Every task keeps a base SHA, reviewed PR SHA, merge SHA, exact workflow evidence, limitations and next action when available.

## Current milestone board — choose by dependency, not aspiration

| ID / priority | Status at baseline | Bounded next action | Completion gate |
| --- | --- | --- | --- |
| DOC-01 / P0 | **MERGED** — [PR #83](https://github.com/Rofikali/Messy-File-Cleaner/pull/83), merge `6529bd9` | IMGENGINE-style governance adapted to local C17 product; docs only. | Four PR-head CI workflows passed; no production release implied. |
| SAFE-01 / P0 | **IN_PROGRESS — review candidate; not merged** | Make default Compose source mount readonly, add explicitly named writable-source execution overlay and disposable Compose-mode CI tests; document safe usage. | PR-head Docker default/override smoke, CMake/Makefile/Ubuntu CI, exact source/target preservation, owner review; not yet verified. |
| HYGIENE-01 / P1 | **READY after DOC-01** | Review tracked `perf.data` and `gprof.txt` for provenance/sensitive content, retention and safe cleanup; clarify .gitignore/benchmark artifacts. | Owner review before deleting tracked files; CI; no destruction of user data. |
| GOV-01 / P1 | **READY after DOC-01** | Reconcile README, `AGENTS.md`, `skills.md` and uppercase `SKILLS.md` as a single clear authority, accurately permitting **test-only Rust**, not production Rust. | Docs-only or scoped PR; no policy loosening. |
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

1. **DOC-01 merged** (PR #83); preserve its governance baseline and reconcile current main SHA.
2. Review SAFE-01 separately, verifying actual Compose config and disposable real/dry-run smoke before any merge; safety before convenience.
3. Address HYGIENE-01 and GOV-01 as separate low-risk changes (order by review readiness).
4. Reassess FS-01 safety/release risks; advance RUST-01 only after safety gates and accepted test-shim contract.
5. Start PERF-01, ABI-01 and BIZ-01 when dependencies, customer evidence, and resources justify them.
6. Never start a dependent branch on an unmerged PR; stop if an unverified security, journal, ABI, owner or release gate is unresolved.

## No automation illusion

A committed agent instruction is not an always-running agent. At a new session: read current GitHub/main, `next.md`, relevant contracts, select one READY item, prepare one bounded PR, inspect exact-SHA CI, and leave a checkpoint. Never merge, deploy, charge, delete branches or modify real user data without explicit approval. Use [continuation runbook](docs/AGENT_CONTINUATION_AND_HANDOFF.md).
