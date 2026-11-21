# Messy-File-Cleaner — Compact LIVE NOW / NEXT / LATER

> **Verified GitHub checkpoint, 2026-11-20:** default branch `main` at `24044b7cbc77e661e49d0b41ddfbe89f77b78372` when PERF-01K work began. PRs #114, #115 (private, exclusive/no-follow benchmark evidence and negative regressions) and #116 (dual-trigger governance) are **MERGED**; there were zero open work PRs at that checkpoint. This is a dated snapshot; fetch current GitHub main, open PRs, CI and review gates before work. **No production release is authorized.**
>
> **One authoritative current queue:** this compact file; older chronological logs are preserved in [next-history.md](next-history.md) and must not be used as the current task selector. [Documentation map](docs/README.md) and [milestone documentation transaction](docs/MILESTONE_DOCUMENTATION_WORKFLOW.md) explain what changes alongside each code PR.

## LIVE NOW / NEXT / LATER

| Lane | ID and current state | Next bounded engineering action | Required evidence / reason |
| --- | --- | --- | --- |
| **NOW — PR OPEN** | **PERF-01T (#127) — RECONCILED / CI PENDING** | FIFO preservation regression tests retained after #126 merge; PR original tested head `0ec3430e92539fd1e2282709acfcbecb42f1fad5` had CMake, Docker, Ubuntu 24.04, Makefile PASS, but reconciled head requires new exact-head CI. Local tests NOT RUN. Next READY: verify exact-head CI, reviews and protections; merge only if eligible. | Test-only, disposable fixtures; no P0 repair or release. |
| **NEXT — completed** | **PERF-01S (#126) — MERGED** | Verified actual merge/main SHA `e4056759602aefb3f8e3814fd81f2643c23742c6`; prior exact-head four CI workflows PASS. | Nonempty-directory benchmark preflight only; non-atomic. |
| **NEXT** | **DOC-GOV-01 — MERGED** | PR #117 integrated the compact live board, history archive and documentation impact checklist; continue updating affected docs in each implementation PR | Merge `24044b7cbc77e661e49d0b41ddfbe89f77b78372`; this does not prove filesystem P0 fixed |
| **BLOCKED P0** | **FS-01B-FIX — BLOCKED** | Obtain approved same-UID source-retirement authority/trust/crash model, design safe positive preservation tests | [Threat model](docs/security/THREAT_MODEL.md), [FS-01B safety plan](docs/quality/FS_01B_FIX_SECURITY_ACCEPTANCE_PLAN.md); merged guard is only partial |
| **BLOCKED P0** | **REL-01 — BLOCKED** | After FS design/safety evidence, establish candidate, backups, supported filesystems, recovery and rollback signoff | C11/C17/C23 + Docker CI passing alone is not release certification |
| **NEXT gated** | **RUST-01 / ABI-01 — DEFERRED** | Test-only Rust verification work only with approved test boundary; production public .a/.so, FFI or FCJ4 recovery require separate ADR/owner decision | C17 production core and Ubuntu 24.04 Docker remain unchanged |
| **LATER** | **BIZ-01 — PLANNED** | Validate users, support and cost with real evidence before commercial SDK/API | No paid infra, SaaS, telemetry or billing on this workstream |

## Recent verified merges (not a live CI claim)

| PR | Actual integrated commit | Delivered bounded scope |
| --- | --- | --- |
| [#113](https://github.com/Rofikali/Messy-File-Cleaner/pull/113) | `58a451ab640672950f16cb6b87348737da9c9c82` | Existing benchmark CSV/JSON evidence preserved |
| [#114](https://github.com/Rofikali/Messy-File-Cleaner/pull/114) | `0ae0b63277ba7a04ec566cf0c92b66eb02088e40` | Private no-clobber evidence creation |
| [#115](https://github.com/Rofikali/Messy-File-Cleaner/pull/115) | `4fd2d8c031e9da5a20f34ff0fcd2477b04eff662` | Evidence permissions and symlink-race regression |
| [#117](https://github.com/Rofikali/Messy-File-Cleaner/pull/117) | `24044b7cbc77e661e49d0b41ddfbe89f77b78372` | Compact board, dated history and docs impact PR checklist |
| [#116](https://github.com/Rofikali/Messy-File-Cleaner/pull/116) | `8144736aeebcb961cad5d7f32e5f0e4468bb73c5` | Manual plus unattended hourly governance |

## Non-negotiable continuation and status truth

- **First:** current GitHub main/PR head, code diff, reviews, comments, branch protections, current exact-head CI and this live board. Continue an open PR first; if blocked, advance independent READY non-destructive work.
- **Every material PR:** update this single live work board *with the code/tests*, record implemented versus PR_OPEN versus CI_PASS versus MERGED versus RELEASED, explain updated docs and N/A, check applicable security/architecture/runtime/CMake/Makefile/finance docs. Do not create repetitive status-only PRs.
- **Verification:** C17 production; independent C11/C23 compatibility, CMake/CTest/sanitizer, Ubuntu Docker, safe Makefile refusal tests and conditional TSan/Rust test-only gates when relevant. SKIPPED / not triggered / API-only edits are NOT RUN.
- **Safety:** never automatically execute a real move on user/customer files, retire sources, adopt production Rust/FFI/FCJ4, weaken CI/branch policies, publish or spend. Unresolved same-UID check/unlink risk remains a P0 release blocker, not a solved regression.
- **Scheduler:** the external ChatGPT automation is independently enabled hourly; manual `go ahead` does not reset it. A repo Markdown change never constitutes an actual scheduled run. Log verified execution and the one next executable job.
- **History:** old milestone states, dated PR updates and full development chronology remain intact in [next-history.md](next-history.md); consult only when relevant, and never elevate an old PR_OPEN label above current GitHub state.
