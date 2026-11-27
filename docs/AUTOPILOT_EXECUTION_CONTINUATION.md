# Hourly Engineering Autopilot — Continuation and Recovery Contract

**Owner-directed release and distribution phase (2026-11-27):** [Release, packaging and distribution roadmap](RELEASE_PACKAGING_DISTRIBUTION_ROADMAP.md). `go ahead` / `continue` / `next` / `next milestone` resumes the next eligible bounded job in the live board after GitHub reconciliation; same rules for hourly agents and engineers. CI-RECONCILIATION-01 is selected for the release-baseline incident; FS-01B-FIX remains the critical blocked safety prerequisite. LICENSE-01, PACKAGING-01, RELEASE-PIPELINE-01, GHCR-01, REL-01, PRODUCTION-SDK-01 and PRODUCT-ADOPTION-01 are gated future work, not implemented or release authorization. Preserve five-PR ceilings, no-spend, no external import, no automatic tag/publish, and the existing Rust FFI gate. Apply L1–L7 engineering, CISSP/security, CCIE/networking, Linux/POSIX, SRE/DevOps, CA/CFO, MBA/product and management review lenses; synchronize core and affected docs with evidence, not claims.

**Owner-approved upstream-first policy (2026-11-25):** Follow [reuse policy](upstream/UPSTREAM_REUSE_POLICY.md) and [master plan](upstream/UPSTREAM_MASTER_PLAN.md). Prefer supported Linux/POSIX APIs and mature compatible components; retain working code unless comparative evidence justifies change. External source imports and new third-party dependencies are BLOCKED pending explicit owner license decision and component acceptance. C17, C11/C23 CI, FS-01B/REL-01, no-spend and five-PR constraints remain binding.

## Cross-contributor execution contract (24 November 2026)

Every authorized human or agent must reconcile live GitHub main, all open and recently merged/closed PRs regardless of author, existing branches, exact-head/base CI and the feature ledger before selecting work. Resume existing eligible work instead of duplicating it. Manual `go ahead` and hourly execution coexist; changes by other contributors are discovered on the next hourly poll, not an instant webhook. Five opened PRs per stable feature ID across all authors; same-PR fixes do not consume a slot. Update AGENTS, skills/SKILLS, README, next and affected architecture/security/CI/SRE docs in the same PR; mark unchanged families N/A. Review honestly and merge only with real gates satisfied. If checkout fails, attempt concrete GitHub connector operations; report the actual failed operation/error if blocked. A completed scheduled invocation is not evidence of engineering progress. Preserve filesystem safety and release blocks.

**Status:** repository operating procedure, 23 November 2026. **Scope:** `Rofikali/Messy-File-Cleaner` only. This document does not schedule an agent: the separately enabled ChatGPT task owns its hourly recurrence, execution environment and run history. A run can fail or be delayed; neither a Markdown edit nor a successful GitHub Actions workflow proves that a scheduled coding invocation occurred. Manual **go ahead** is an additional immediate request, never a replacement for the hourly task. Follow [binding AGENTS rules](../AGENTS.md), [GOV-02](AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md), [no-human-approval governance](AUTOPILOT_ZERO_REVIEW_AND_DOC_SYNC.md), [live board](../next.md), and [feature ledger](feature-budget.json).

## Outcome invariant

Each *executed* run should attempt **one bounded, genuinely useful engineering advance**, not a routine re-audit: fix an existing PR and its tests/CI; complete a permitted exact-head review/merge; or implement one source/test/CI change in a reviewable feature PR. A run that cannot safely implement must report **NO CODE CHANGED**, the verified evidence and the specific gate. A platform invocation marked completed is **not** automatically a completed milestone. Never fabricate progress, CI, a PR, scheduling reliability or test evidence.

## Deterministic resume order

1. **Trust live GitHub before dated prose.** Read current `main`, *all* open PRs, newest merged PR, exact PR-head/base SHAs, relevant checks and review threads. Read only the affected source/tests and the top live selector. Reconcile any former OPEN PR that is actually MERGED. Do not repeatedly audit historical PRs or open cosmetic status-only PRs.
2. **Resume an existing PR before opening another.** If it covers the active feature, finish the smallest missing step: reproduce/fix a failing check in the *same PR*, resolve factual review blockers, run required exact-head jobs, or merge when GitHub rules actually permit it. An agent cannot claim independent approval by commenting on its own change.
3. **If no PR is open, select executable safe work.** Treat a stale `ACTIVE/OPEN` ledger entry as an inconsistency to carry into the **next substantive implementation PR**, not as authority to reimplement merged code. Inspect source, tests and issue evidence for an independent non-destructive C17 scanner/configuration/CI defect; require reproduction, stable feature ID, bounded acceptance and a rollback. If evidence is insufficient, select another independent safe candidate rather than making up a cosmetic task. Do not re-label a closed feature to escape the five-opened-PR limit.
4. **Implement before broad reporting.** Change the smallest coherent source/tests or scoped CI, add negative/disposable-fixture tests, then update `next.md`, `docs/feature-budget.json` and impacted AGENTS/README/skills/SKILLS/HLD/LLD/security/operations docs in that same feature PR. Note UPDATED vs N/A for every relevant document family.
5. **Gate integration on evidence.** Review diff, current head/base, required C11/C17/C23 and applicable Docker/Ubuntu/Makefile/sanitizer/security checks, blocking discussions and active branch rules. Missing, skipped or conditional unexecuted jobs are NOT RUN, not PASS. Only a permitted, safe merge may use `expected_head_sha`; verify merged `main` and integrated CI separately. No bypass.
6. **Hand off unfinished work.** Record the exact unfinished action in the automation's run result and, when an existing PR exists, in its factual PR conversation. On the next execution, read that checkpoint *and revalidate GitHub live state*; do not assume a previous head or test result remains current. Do not create a Git commit solely to store a heartbeat.

## Minimum continuation checkpoint (every executed run)

| Field | Required evidence |
| --- | --- |
| Invocation | Scheduled / manual; scheduled-for, actual-start and actual-end when the scheduler exposes them; task enabled state only if checked from the actual task |
| Progress | **CODE CHANGED** / **NO CODE CHANGED**; why; exact bounded outcome; changed paths |
| Source | Live `main` SHA, PR number and URL or NONE, feature ID, previous/current head, base, merge SHA if merged |
| Tests and governance | Exact-head required/applicable check conclusions and URLs; local tests PASS/FAIL/NOT RUN; branch protection and blocking threads verified or UNKNOWN |
| Documentation | Ledger/board and impacted docs UPDATED / DEFERRED TO SAME SUBSTANTIVE PR / N/A; never pretend stale generated or historical snapshots are live |
| Continuation | Single next executable file/function/test, prerequisite, owner-only decision if applicable, and retry count for the *same* blocker |
| Risk and cost | File-preservation/rollback evidence, residual risks, zero unapproved spending/deployment and manual acceptance status |

The latest run report is a **handoff hint**, not another authoritative task queue. GitHub is the source of truth for merges and checks; code/tests for behavior; the feature ledger/board for planned work only after reconciliation. No redundant per-run status commits.

## Stalls, scheduler health and recovery

* If a run fails to occur, inspect **ChatGPT Tasks/Automations** for actual enablement, schedule, last recorded run, failure/notification and connection/environment state. GitHub Actions activity cannot diagnose missing task invocations. Do not create a duplicate task.
* If checkout, local compilation or CI interrogation is unavailable, attempt a safe **GitHub connector** edit/review where authorized; explicitly mark local compilation/tests NOT RUN. Do not fabricate Docker, Ubuntu, sanitizer or filesystem results.
* After **two consecutive NO CODE CHANGED executions for the same blocker**, state the blocker and failed step; either switch to a distinct independently eligible safe task or escalate the exact owner/administrative decision. Never bypass P0 safety, spend, release restrictions or protected branch checks to generate activity.
* For main-versus-ledger drift (for example merged #150 still listed OPEN on 23 November), retain the actual merge SHA and repair the record during the next *material* PR; a narrowly scoped factual governance PR is justified only if the stale selector would materially misroute subsequent executions. Do not create timestamp-only churn.
* At each invocation report whether the scheduler **fired**, whether code actually changed, and whether anything **merged**. These are three distinct events; no execution cadence or merge is guaranteed by documentation.

## Proportional L1–L7 and cross-discipline review

| Lens | Concrete pre-merge question |
| --- | --- |
| L1–L2 implementer | Is there a reproduced bug, bounded fix, negative regression, readable modular C and deterministic error handling? |
| L3–L4 senior/integration | Are contracts, CLI/config behavior, C11/C17/C23 portability, ownership and rollback unchanged or intentionally versioned? |
| L5 staff/system | Are concurrency, races, idempotence, directory identity, TOCTOU and unrelated-file preservation reviewed? |
| L6 principal/platform | Are CI provenance, branch rules, observability, Linux/POSIX/Docker/Compose behavior and recovery evidence proportionate? |
| L7 architecture/governance | Does it advance one approved product outcome with a feature budget, clear risk owner, measurable acceptance and truthful release status? |
| CISSP/security | Threat model, least privilege, supply chain, input validation, secret exposure, audit and no destructive operations on untrusted files |
| CCIE/networking | Normally N/A for offline local-file work; revisit only on new network/API/remote service boundaries, and record threat/latency assumptions |
| CA/finance, MBA/management | No unapproved spend or recurring paid dependency; weigh implementation/maintenance cost, customer value, liability and opportunity cost |

**Hard stop:** FS-01B same-UID source-retirement TOCTOU P0 and REL-01 remain BLOCKED; no real customer-file moves/deletion, unapproved production FCJ3/FCJ4/Rust FFI or ABI changes, deployment, paid infrastructure, release or branch-protection bypass. Manual native Ubuntu/Docker Desktop/Compose, backups, power-loss and real-filesystem acceptance are NOT RUN until actually executed. Zero required human approvals is not zero technical verification.
