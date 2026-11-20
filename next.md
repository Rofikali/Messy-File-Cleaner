# Messy-File-Cleaner — Compact LIVE NOW / NEXT / LATER

> **Verified GitHub checkpoint, 2026-11-20:** default branch `main` at `24044b7cbc77e661e49d0b41ddfbe89f77b78372` when PERF-01K work began. PRs #114, #115 (private, exclusive/no-follow benchmark evidence and negative regressions) and #116 (dual-trigger governance) are **MERGED**; there were zero open work PRs at that checkpoint. This is a dated snapshot; fetch current GitHub main, open PRs, CI and review gates before work. **No production release is authorized.**
>
> **One authoritative current queue:** this compact file; older chronological logs are preserved in [next-history.md](next-history.md) and must not be used as the current task selector. [Documentation map](docs/README.md) and [milestone documentation transaction](docs/MILESTONE_DOCUMENTATION_WORKFLOW.md) explain what changes alongside each code PR.

## LIVE NOW / NEXT / LATER

| Lane | ID and current state | Next bounded engineering action | Required evidence / reason |
| --- | --- | --- | --- |
| **NOW** | **PERF-01K — MERGED (#118)** | Merged at `5b68979d2765a0008fc9824e446a48c77b1ce7d3`; exact PR head `b3e22029e269918135d8583e7b786ddf8c1be20e` had four successful workflows. | Scoped fixture preservation/no-clobber tests, actual current-head CI and honest benchmark environment; never run against user files or invent nanosecond claims |
| **NOW** | **PERF-01L — MERGED (#119)** | Exact PR head `7ea51c3f33f41b40d242de3499113d691bcfe146`; four workflows PASS; integrated main `87a51ee0f6c61f8461bc9e07d5dec5c0f9ea155a`. | Benchmark-only preflight, not atomic hostile-filesystem protection or P0 repair |
| **NOW** | **PERF-01M — MERGED (#120)** | Exact PR head `76ff887277304e9ee908c3b032c55d1b20324989`; four exact-head workflows PASS; integrated main `f70d825ec3e6892f1486247d9d6791c4c8e5687b`. | Negative disposable fixture regressions only; no production P0 repair |
| **NOW** | **PERF-01N — MERGED (#121)** | Reject hardlinked expected benchmark fixtures (link count must be one), add external hardlink preservation regression. Exact PR head `890bf4a84b0eb64bbcfe6d1671df1e10f102bbf7`; integrated main `f51d6db8fd376b8f4fb7e883d6fe3c3d551a6657`. Exact-head CI not reverified in this run. | Benchmark integrity guard, not a race-free same-UID filesystem guarantee; no customer-file operations |
| **NOW** | **PERF-01O — PR #122 OPEN / CI RECHECK REQUIRED** | Exclusive fixture creation and preservation tests; corrected multi-directory test to use a fresh disposable root after exact-head `9de6ca6e2d1dce3c67a73831ac24b99c05c18391` failed Makefile regression (3 PASS, 1 FAIL). Repair commit `d56eb4cbb89db6d691a85b110594b0920515f6f6`; local tests NOT RUN. Next READY: verify final exact-head four CI workflows, review threads and merge gates; merge only if eligible. | No production source-retirement change; FS-01B-FIX and REL-01 still BLOCKED |
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
