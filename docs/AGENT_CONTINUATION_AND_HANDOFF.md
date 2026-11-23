# Coding Agent Continuation, Task Selection and Milestone Handoff

**Standing process, not autonomous execution.** Applies to Codex, Astra, GitHub-connected agent or a human reviewer. Reading this runbook cannot wake a stopped chat, provision a coding agent, change branch protections or grant merge/deploy permission. The owner's **standing zero-human-approval, CI-gated PR review/merge authorization** for the existing hourly autopilot is documented in [GOV-02](AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md); only authorized agents with current GitHub access may exercise it.

Follow the [hourly execution continuation and stall-recovery contract](AUTOPILOT_EXECUTION_CONTINUATION.md) to record a compact last-action checkpoint and resume the precise unfinished step. Two same-blocker NO CODE CHANGED executions must trigger a distinct safe candidate or an explicit blocker escalation, never repeated ceremony. This procedure does not activate the external ChatGPT schedule.

## Five-PR handoff (current owner rule)

From 2026-11-21 follow [feature policy](FEATURE_DELIVERY_AND_ACCEPTANCE_POLICY.md) and `docs/feature-budget.json`. Verify live main and existing PRs before opening one. Stable feature ID, all opened PR attempts, ceiling five; end early on accepted outcome, otherwise close DONE/PARTIAL/BLOCKED at fifth PR and advance. No unlimited tiny tests under a new sub-ID. Historical PERF-01 was merged through #128 and is CLOSED_LEGACY; PR #142 merged at `69fdffe49e2bf502e178fe3a92044d04f4f2653c`. PR #143 AUTOPILOT-HANDOFF-01 merged at `80de73b465ede49eec0882846c50efc0cbdee254`; NOW selected is PLAN-REPORT-01 (0/5; read-only preview/reporting); the live board and live GitHub supersede previous task snapshots. Distinguish exact-head CI from local Ubuntu/Compose, filesystem safety, crash/power-loss and release signoff; preserve blocked FS-01B-FIX without destructive execution.

## Roadmap candidate promotion into one safe task

Read the [advanced feature roadmap](ADVANCED_FEATURE_ROADMAP.md) and [acceptance matrix](ADVANCED_FEATURE_ACCEPTANCE.md) **only after** reconciling current main/open PRs and the live [NOW board](../next.md). Proposals cannot become automatic parallel jobs. When current feature closes, choose a separately justified dependency-unblocked non-destructive candidate, define positive preservation/negative tests, register ONE active stable ID in [feature budget](feature-budget.json), and update the board and affected docs in the same substantive PR. Advanced mover/undo/recovery, ABI/FFI, desktop, licensing, publishing, network service and paid resources need distinct approved architecture/security/product decisions. Preserve review, same-UID P0 and release gates.

## Standing owner shorthand: go ahead / continue / next

This repository adopts the same **next-work selection** convention as IMGENGINE: when the owner says **"go ahead"** (including **"go ahead next"**, **"continue"** or **"next milestone"**), treat it as an instruction to **proceed now with the next dependency-unblocked work item in the top LIVE NOW / NEXT / LATER board of `next.md`**. Do not reflexively ask the owner which milestone to do or ask them to paste an agent prompt. Read actual GitHub state first: a task labelled open in an old paragraph may already have merged, and a green PR is not necessarily merged.

1. If the active task has an **open PR**, inspect current-head checks and comments, fix CI/test failures in the **same** PR when safe, or report READY_FOR_REVIEW. Never duplicate it. If this session is the owner-authorized autopilot, it may complete an eligible PR's exact-head review and **merge under GOV-02 gates**, not merely because checks are green.
2. If a PR was **externally or autonomously merged**, verify merge SHA and current `main`, then select the next independent eligible task. Record merge reconciliation in the next authorized PR. Do not rewrite historical evidence.
3. Choose the highest-priority **NOW** item that is READY and within ordinary non-destructive authorization. If blocked, disclose the gate and move to another independently READY NOW/NEXT item; do not bypass a release/security gate merely to keep moving.
4. Perform a **single bounded milestone**: inspect relevant source and tests, implement the authorized review/docs/disposable-fixture scope, run applicable focused gates, open a draft PR for review and checkpoint CI results and residual risks. No background work is promised after the response.
5. **Merge exception:** the owner's active [GOV-02 standing authorization](AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md) permits autonomous merges of **eligible, fully reviewed, exact-head green** PRs while respecting GitHub protections. The phrase **"go ahead" alone** is only task selection. Stop for explicit task-specific approval before blocked/high-risk production FS/journal/ABI changes, deployment, destructive real/user-file operations, branch/history deletion, secrets/security-rule changes, paid resources, or unapproved production FCJ3/FCJ4/FFI.

The shorthand is a **workflow selector**, not an autonomous service, a blanket authorization to modify anything in the backlog, or a requirement to run a blocked task. FS-01B known-risk test and FS-01B-GUARD partial mitigation are merged; **the same-UID source-retirement race and broader release gates remain blocked**. A P0 fix still requires an accepted trust/crash model and positive preservation tests.


**No READY task is not a reason to idle:** After reconciling live merged PRs and current board, identify a distinct, evidenced, non-destructive C17 correctness or CI/automation-validation gap with bounded acceptance and tests. Register ONE stable feature ID and update the ledger + live board in the SAME implementation PR. Never manufacture cosmetic work or rename closed features to evade the five-PR cap. After two no-code attempts on the same blockage, try a different independent safe candidate or escalate a precise owner decision. Live GitHub overrides stale open/merged labels. A board/ledger consistency test cannot prove GitHub PR liveness.

## First command for a new session

> Read `AGENTS.md`, `next.md`, `docs/README.md`, `skills.md`, [GOV-02 autopilot merge policy](AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md), current `main`, relevant security/architecture docs and exact PR/CI state. Choose the next dependency-unblocked READY milestone, make one bounded change/PR with focused tests and required CI, checkpoint exact results. Under active standing owner authorization, **self-review truthfully and merge only eligible PRs with exact SHA and mandatory protections**. Never deploy, delete real files/history, spend money, bypass security or implement a blocked P0/destructive change without a separate authorized decision.

## Latest TOOL-01 rust-check merge checkpoint (2026-11-18)

- Verified GitHub `main` `e1ab35558240113934126196a57cc22f68ecd563`; [PR #103](https://github.com/Rofikali/Messy-File-Cleaner/pull/103) **MERGED** from reviewed head `1c07b11b09f7c4e5ca73835b0a38e98476888320`. No open PRs before this documentation follow-up.
- `make rust-check`: maturity **IMPLEMENTED**; delivery **MERGED / NOT RELEASED**. Runs at exact PR head: [Rust](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/44158446471) PASS, [C matrix](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/44158446455) 6 PASS, [Docker](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/44158446574) PASS, [Makefile](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/44158446593) PASS; **9 PASS / 0 FAIL**. Local tests **NOT RUN**.
- The scoped TOOL-01 developer targets `make status`, `make self-check` and `make rust-check` are merged; Rust remains test-only. No production FFI, FCJ3/FCJ4 linkage, source deletion or release certification. FS-01B-FIX and REL-01 remain **P0 BLOCKED** with unresolved same-UID race.
- Next: review this documentation PR's own diff and exact-head CI before merging. Then reassess independent READY work; RUST-01B requires explicit unsafe/FFI approval and cannot be started by inference.

## Historical TOOL-01 self-check merge checkpoint (2026-11-17)

- Verified `main` at `27a06701c8203a3279ee647543ce5b485bbdc405`, [PR #101](https://github.com/Rofikali/Messy-File-Cleaner/pull/101) **MERGED** from exact head `cd156241ebfd91ef5c0c89c881e1bb530f08043f`. No open PRs at checkpoint.
- Maturity: `make self-check` **IMPLEMENTED**, delivery **MERGED**; TOOL-01 overall **PARTIAL** (`make rust-check` **NOT_STARTED**, remains test-only). Ubuntu Makefile CI explicitly executed self-check successfully; C11/C17/C23 Debug/Release 6 PASS, Docker 1 PASS, Makefile 1 PASS (8 PASS, 0 FAIL). Runs: [CMake](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/44124849188), [Docker](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/44124849204), [Makefile](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/44124849390). Local full repository tests **NOT RUN**.
- The self-check inspects Makefile recipes, **not** adversarial filesystem behavior. FS-01B-FIX and REL-01 remain P0 **BLOCKED**; same-UID source-retirement vulnerability unresolved. No FCJ3/FCJ4 production link, Rust FFI, data mutation or release authorization.
- Next independent bounded milestone: evaluate isolated `make rust-check` only under existing Rust test-only rules; review this documentation PR's own exact-head CI before merging. Prior PR #101 CI is not this PR's CI.

## Latest verified RUST-01A checkpoint (2026-11-17)

- `main` at `18661c8291a8afd2129573dfb57ca88146665995` includes [PR #97](https://github.com/Rofikali/Messy-File-Cleaner/pull/97), a documentation-only reconciliation of RUST-01A's already merged test shim ([PR #96](https://github.com/Rofikali/Messy-File-Cleaner/pull/96)).
- Maturity: RUST-01A **IMPLEMENTED IN TESTS**; delivery: **MERGED**. RUST-01B Rust unsafe/FFI boundary: **NOT STARTED / BLOCKED** pending explicit approval. Production FFI and FCJ4 integration: **NOT AUTHORIZED**.
- FS-01B-FIX and REL-01 remain **P0 BLOCKED**. The same-UID source-retirement race is not fixed; a passing known-risk test is not positive preservation acceptance.
- This follow-up documentation PR requires fresh exact-head CI review. Local C11/C17/C23, Docker and Rust tests were **NOT RUN** for this documentation-only checkpoint; never copy previous PR-head results as current-PR results.
- Next: verify this follow-up PR's diff, comments, protections and CI before any routine merge. Then review the separate RUST-01B boundary decision without introducing Rust FFI prematurely.

## Start-of-session sequence

1. `git status --short`; `git branch --show-current`; `git rev-parse HEAD`; fetch latest `main`/PR state. Do not overwrite unknown dirty worktrees.
2. Read root rules, `next.md`, docs index and the specific module's header, callers, tests, threat model and ADR.
3. Reconcile stale statuses against actual merged PRs and SHA-specific CI. Read only the top LIVE NOW / NEXT / LATER section of `next.md` for task selection and use [component completion inventory](PROJECT_COMPLETION_AND_STATUS_INVENTORY.md) to distinguish **NOT_STARTED / DESIGN_ONLY / STARTED / PARTIAL / IMPLEMENTED** from **PR_OPEN / CI_PASS / MERGED / RELEASED**. The retained historical section is reference evidence, not the current queue.
4. Choose **one** eligible task, respecting dependency order, top-priority file safety and available evidence. Declare out-of-scope and a bounded resource/test budget.
5. Create branch from current main, implement smallest reviewable diff, focused tests, docs, and open draft PR only where publication is authorized.
6. Inspect full required GitHub check **conclusions** and only failed excerpts; do not retest huge workloads merely to fill model context. Higher-risk journal/FS work still requires full gate.
7. If checks pass, record `READY_FOR_REVIEW`; never infer `MERGED`. Under the active GOV-02 standing merge authorization, independently review and merge **only eligible** PRs with the expected head SHA; otherwise request owner decision. Start a dependent milestone only after GitHub confirms merge.
8. Leave one durable checkpoint in PR/next docs; when run ends, stop. An external configured runner is needed for scheduled or event-driven automatic continuation.

## Status/event decisions

| Event | Status | Next operation |
| --- | --- | --- |
| Backlog task chosen | IN_PROGRESS | Scope, branch, acceptance and negative cases |
| PR published | PR_OPEN / CI_PENDING | Observe relevant checks, avoid premature pass |
| Exact-SHA required CI succeeded | READY_FOR_REVIEW | Send human review summary and blocker risks |
| Failed required CI | IN_PROGRESS / BLOCKED | Diagnose within same task, preserve failure evidence |
| Owner or GOV-02 authorized autopilot merges eligible PR | MERGED | Verify exact main merge SHA; record proof, update next in next bounded PR and select eligible task only if authorized |
| Required design/FS/ABI authority missing | BLOCKED | Preserve evidence and request decision; don't work around invariant |
| Release candidate requested | RELEASE_CANDIDATE | Verify exact artifact/safety/rollback/support/license gates, explicit go/no-go |
| Agent session exits | CHECKPOINTED (session only) | Do not claim background work |

## Reusable bounded-task handoff template

```text
PROJECT: Rofikali/Messy-File-Cleaner
TASK_ID / PRIORITY / STATE:
CUSTOMER OUTCOME:
CURRENT MAIN SHA / FEATURE BRANCH / PR HEAD SHA:
CURRENT OWNER DECISION / ADR:
FILES AND CONTRACTS IN SCOPE:
OUT OF SCOPE:
SECURITY / DATA PRESERVATION / OWNERSHIP:
CONCURRENCY / CRASH / FAILURE CASES:
TEST LANE (L0/L1/L2/L3) AND EXACT COMMANDS:
CI RUN IDS, CONCLUSIONS, TESTED SHA:
RESOURCE / PERFORMANCE / SUPPORT / CA-MBA COST:
RISKS / UNSUPPORTED ASSUMPTIONS:
ROLLBACK:
HUMAN APPROVAL REQUIRED FOR:
NEXT SINGLE ACTION:
```

## Examples tailored to this repo

- **SAFE-01:** Compose dry-run readonly input; production Docker smoke already has separate readonly dry-run and writable real-move fixture. Do not globally make all source mounts readonly or invalidate real-move semantics.
- **RUST-01:** Rust tooling is merged but not FFI; future test-only C shim cannot import an FCJ4 production journal or expose destructive operations.
- **ABI-01:** A `.a` or `.so` file does not establish a versioned safe SDK; need consumer + ownership contract, ABI review, packaging/license.
- **FS-01:** Even valid journal replay is neither authenticity nor real filesystem truth. No unlink/retire based on parser validity alone.

## Approval boundaries

Never infer permission from **"go ahead" alone** to merge a PR, delete source/history, overwrite customer files, use paid infrastructure, deploy, change secrets or bypass unsafe recovery gates. The owner provided **separate standing approval for eligible autopilot merges on 2026-11-17**; apply [GOV-02](AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md) with exact-head review, current required CI, branch protections and explicit P0 safety exceptions. A task-specific authorization remains necessary for high-risk or irreversible operations. Do not self-approve as an independent reviewer. State what is implemented, tested, merged and *not done*.
