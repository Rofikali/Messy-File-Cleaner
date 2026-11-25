# Five-PR Feature Delivery and Acceptance — Messy-File-Cleaner

**Upstream-first owner amendment (2026-11-25):** [Reuse policy](upstream/UPSTREAM_REUSE_POLICY.md) and [master plan](upstream/UPSTREAM_MASTER_PLAN.md) govern upstream candidate intake. License and component review precede imports; proposed stages do not reset consumed PR budgets. Existing selector and gates unchanged.

## Cross-contributor execution contract (24 November 2026)

Every authorized human or agent must reconcile live GitHub main, all open and recently merged/closed PRs regardless of author, existing branches, exact-head/base CI and the feature ledger before selecting work. Resume existing eligible work instead of duplicating it. Manual `go ahead` and hourly execution coexist; changes by other contributors are discovered on the next hourly poll, not an instant webhook. Five opened PRs per stable feature ID across all authors; same-PR fixes do not consume a slot. Update AGENTS, skills/SKILLS, README, next and affected architecture/security/CI/SRE docs in the same PR; mark unchanged families N/A. Review honestly and merge only with real gates satisfied. If checkout fails, attempt concrete GitHub connector operations; report the actual failed operation/error if blocked. A completed scheduled invocation is not evidence of engineering progress. Preserve filesystem safety and release blocks.

**Owner direction effective 2026-11-21.** This policy is adapted to the C17/POSIX safety and lifecycle model. It does **not** authorize destructive moves, production recovery, release, spending, production Rust/FFI, or weaker security gates.

## 1. One feature = maximum five opened PR attempts

A *feature* is a stable user/security/engineering outcome with one ID, not an arbitrary submilestone label. All feature fixes, tests and refactors belong to the same count. Every **opened feature PR** consumes one slot whether merged, closed, failing or abandoned. Additional commits to the **same PR** do not consume more slots. Five is a **ceiling**, not a target; finish earlier whenever scoped acceptance is met. Never rename `PERF-01V` as an allegedly new feature just to obtain more PRs.

Maintain actual opened numbers under `docs/feature-budget.json` `features[].prs`; the Python-standard-library `scripts/feature_budget_check.py` validates the five-PR limit and, in PR CI, that the actual PR number appears under its `Feature-ID`. A new PR initially lacks its number: once GitHub assigns the number, commit that number to the **same PR** before it can pass the policy check. Do not invent PR numbers. Every PR body must carry exactly one `Feature-ID: <ID>` line. `Feature-ID: GOVERNANCE-EXEMPT` is narrowly for a genuinely docs/governance-only PR with a `docs(...)` title and independent reviewer scrutiny, not for production changes. The checker cannot prove that the ledger contains *every historic GitHub PR*; agents must reconcile live GitHub.

After at most five PRs, close the feature **DONE / PARTIAL / BLOCKED** in the handoff and move to the next independent READY feature. Unverified manual, reliability or performance questions become precise accepted risks/issue follow-ups, not a sixth feature PR. Urgent confirmed file-loss/security issues use a separate traceable `SAFETY_INTERRUPT` issue/feature and an independently approved trust/crash design; not a way to disguise ordinary polishing. Safety gates never expire when the five slots are spent.

## 2. Independent status dimensions

| Dimension | Status and meaning |
| --- | --- |
| Code | `IMPLEMENTED` means scoped source behavior exists. `MERGED` means a verified main commit contains it |
| CI | `AUTOMATED_PASS` requires exact-head/merge run IDs for relevant gates. Skipped is NOT RUN |
| Manual environment | Ubuntu/Docker Desktop/Compose operator test, different filesystems/permissions, hardware runs: `NOT RUN` until observed |
| Filesystem safety | Data loss, same-UID source-retirement, crash/power-loss and journal identity: `BLOCKED` where guarantees remain unproven |
| Product acceptance | `DONE` scoped technical journey; `PARTIAL` with exact gaps; `BLOCKED` on a required prerequisite |
| Product release | `RELEASED` only after independent authorization, signed-off FS/recovery/security and observed distribution |

**Passing a test intentionally reproducing an unsafe unlink is not a safety pass.** No broad production claim or live customer-file exercise is permitted merely because C11/C17/C23 and Docker CI are green.

## 3. Historical 2026-11-21 feature snapshot (superseded by live board)

The section below records the policy's initial 21 November selection, not today's READY queue. Current `next.md` and live GitHub supersede it. On 22 November, PR #142 merged and a separate non-destructive `AUTOPILOT-HANDOFF-01` CI consistency milestone became active. Do not treat a previously completed CLI-DRYRUN-01 as READY again.

- **PERF-01 — CLOSED_LEGACY:** Earlier benchmark instrumentation and fixture-preservation work reached merged PR #128 on main `3d7e14ed16040d6918f5d88330c510db77be0b96`, well above five PRs before this policy. CSV/JSON provenance, no-clobber tests, FIFO/socket/symlink/hardlink negative regressions are implemented; latest main CMake/Ubuntu/Makefile/Docker/performance workflows passed. No further PERF-01-only PRs this cycle. Neither a million-file production benchmark nor actual filesystem power-loss certification has been observed.
- **NOW CLI-DRYRUN-01 — 0/5:** Enhance the *existing* `--dry-run --once` flow for understandable, bounded status/error diagnostics and safe read-only user expectations. First PR should inventory current CLI text/exit codes and add disposable-only negative tests; implement code only when an actual missing user outcome is demonstrated. Keep no real moves and no private path leakage in new evidence artifacts.
- **NEXT SCAN-ROBUST-01:** Independently review non-destructive scanner failure/limits and resource behavior with temporary fixture tests.
- **LATER BENCH-SCALE-01:** New empirical, reproducible *scale* evidence rather than endless PERF-01 fixture polishing; use realistic bounded workloads and report sizes, CPU/RSS, runner and variance. A million files is not a mandatory CI gate.

## 4. End-of-feature documentation transaction

With the final **substantive implementation PR**, reconcile real PR numbers and exact main/head/merge SHAs; record what is coded, merged, what precise CI ran, what manual OS/Compose/permission/filesystem/crash tests were NOT RUN, defects, user impact, rollback and NEXT. Update `next.md`, `docs/PROJECT_COMPLETION_AND_STATUS_INVENTORY.md`, relevant README, docs map, AGENTS/skills, HLD/LLD, ABI, security/FCJ, Docker, CI, performance and business/CA/MBA *only where facts change*. Mark others N/A in the PR. Preserve historical chronology in `next-history.md`. Do not create repetitive timestamp-only docs PRs.

## 5. Engineering and business lenses

Seven engineering levels plus QA/SDET, security/CISSP, Linux/POSIX, DevOps/SRE, networking/CCIE, architecture, finance/CA, MBA and management should review proportional impacts. For a local CLI there is no need to invent a cloud, distributed networking or production Rust service. Prioritize file-preservation and recovery threat boundaries, no-clobber and TOCTOU, ownership, portability, test-fixture disposal, exact CI, performance provenance and user clarity. Record opportunity cost and support liability from each extra PR; no unmeasured nanosecond guarantees or assumed revenue.
