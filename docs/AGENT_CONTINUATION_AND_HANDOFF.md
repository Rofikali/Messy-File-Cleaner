# Coding Agent Continuation, Task Selection and Milestone Handoff

**Standing process, not autonomous execution.** Applies to Codex, Astra, GitHub-connected agent or a human reviewer. Reading this runbook cannot wake a stopped chat, provision a coding agent, change branch protections or grant merge/deploy permission.

## Standing owner shorthand: go ahead / continue / next

This repository adopts the same **next-work selection** convention as IMGENGINE: when the owner says **"go ahead"** (including **"go ahead next"**, **"continue"** or **"next milestone"**), treat it as an instruction to **proceed now with the next dependency-unblocked work item in the top LIVE NOW / NEXT / LATER board of `next.md`**. Do not reflexively ask the owner which milestone to do or ask them to paste an agent prompt. Read actual GitHub state first: a task labelled open in an old paragraph may already have merged, and a green PR is not necessarily merged.

1. If the active task has an **open PR**, inspect current-head checks and comments, fix CI/test failures in the **same** PR when safe, or report READY_FOR_REVIEW. Never duplicate or automatically merge it.
2. If a PR was **externally merged**, verify merge SHA and current `main`, then select the next independent eligible task. Record merge reconciliation in the next authorized PR. Do not rewrite historical evidence.
3. Choose the highest-priority **NOW** item that is READY and within ordinary non-destructive authorization. If blocked, disclose the gate and move to another independently READY NOW/NEXT item; do not bypass a release/security gate merely to keep moving.
4. Perform a **single bounded milestone**: inspect relevant source and tests, implement the authorized review/docs/disposable-fixture scope, run applicable focused gates, open a draft PR for review and checkpoint CI results and residual risks. No background work is promised after the response.
5. **Stop for explicit task-specific approval** before any merge, deployment, destructive real/user file operation, branch or historical evidence deletion, secrets/security-rule change, paid resources, or unapproved production journal/FFI change. An ambiguous **"go ahead" alone is not that approval**.

The shorthand is a **workflow selector**, not an autonomous service, a blanket authorization to modify anything in the backlog, or a requirement to run a blocked task. For the present sequence, FS-01A is a **read-only source audit**; FS-01B requires a **separate test-only** evidence PR before any production filesystem mutation is proposed.

## First command for a new session

> Read `AGENTS.md`, `next.md`, `docs/README.md`, `skills.md`, current `main`, relevant security/architecture docs and exact PR/CI state. Choose the next dependency-unblocked READY milestone, make one bounded change/PR with focused tests and applicable required CI, checkpoint exact results. Do not merge, release, delete branches, run on real user data, spend money or make a destructive decision without specific owner approval.

## Start-of-session sequence

1. `git status --short`; `git branch --show-current`; `git rev-parse HEAD`; fetch latest `main`/PR state. Do not overwrite unknown dirty worktrees.
2. Read root rules, `next.md`, docs index and the specific module's header, callers, tests, threat model and ADR.
3. Reconcile stale statuses against actual merged PRs and SHA-specific CI. Read only the top LIVE NOW / NEXT / LATER section of `next.md` for task selection and use [component completion inventory](PROJECT_COMPLETION_AND_STATUS_INVENTORY.md) to distinguish **NOT_STARTED / DESIGN_ONLY / STARTED / PARTIAL / IMPLEMENTED** from **PR_OPEN / CI_PASS / MERGED / RELEASED**. The retained historical section is reference evidence, not the current queue.
4. Choose **one** eligible task, respecting dependency order, top-priority file safety and available evidence. Declare out-of-scope and a bounded resource/test budget.
5. Create branch from current main, implement smallest reviewable diff, focused tests, docs, and open draft PR only where publication is authorized.
6. Inspect full required GitHub check **conclusions** and only failed excerpts; do not retest huge workloads merely to fill model context. Higher-risk journal/FS work still requires full gate.
7. If checks pass, record `READY_FOR_REVIEW`; never infer `MERGED`. Only after explicit owner approval and verified merge should a dependent milestone start.
8. Leave one durable checkpoint in PR/next docs; when run ends, stop. An external configured runner is needed for scheduled or event-driven automatic continuation.

## Status/event decisions

| Event | Status | Next operation |
| --- | --- | --- |
| Backlog task chosen | IN_PROGRESS | Scope, branch, acceptance and negative cases |
| PR published | PR_OPEN / CI_PENDING | Observe relevant checks, avoid premature pass |
| Exact-SHA required CI succeeded | READY_FOR_REVIEW | Send human review summary and blocker risks |
| Failed required CI | IN_PROGRESS / BLOCKED | Diagnose within same task, preserve failure evidence |
| Owner merges PR | MERGED | Verify main merge SHA; update next; select next eligible task only if authorized |
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

Never infer permission from "go ahead" for merging a PR, deleting source, overwriting customer files, paid infrastructure, production release, secrets changes or an unsafe recovery bypass. A task-specific approval permits only that named bounded action. Code-review feedback and artifacts alone cannot constitute a sign-off. State what is implemented, tested, merged and *not done*.
