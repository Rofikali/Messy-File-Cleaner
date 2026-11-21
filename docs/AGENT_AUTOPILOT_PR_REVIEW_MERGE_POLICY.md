# GOV-02 — Owner-Authorized PR Review and Merge Autopilot

**Status: ACTIVE owner workflow authorization (2026-11-17), bounded to `Rofikali/Messy-File-Cleaner`.** The owner expressly asked the existing hourly autopilot to perform PR creation, review, correction and merging when appropriate. This is an **external owner instruction** recorded here for continuity; this Markdown file by itself does not confer GitHub permissions, bypass protections or authorize an unrelated agent. The recurring schedule belongs to ChatGPT Automations and can be paused by the owner.

## Owner amendment — five-PR feature progression (2026-11-21)

Latest instruction: **five opened PR attempts maximum per feature** (not five per tiny test/regression). The ledger and [policy](FEATURE_DELIVERY_AND_ACCEPTANCE_POLICY.md) count all post-policy attempted PR numbers and CI checks `Feature-ID` identity. PERF-01 through #128 is CLOSED_LEGACY, with the repetitive fixture loop finished; the independent NOW lane is CLI-DRYRUN-01. At fifth PR or sooner, state exactly what is IMPLEMENTED, CI_PASS, MANUAL_NOT_RUN, PARTIAL/BLOCKED and move to another dependency-cleared READY feature. No sixth PR by renaming work. A confirmed safety interruption remains separately scoped and subject to FS-01B trust/crash approval. Never infer actual Ubuntu/Compose, power-loss, backup or broad host validation from CI. Do not continue a stale PERF-01U PR_OPEN record once #128 is merged.

## Dual-trigger, unattended operation (owner reaffirmed 2026-11-20)

- **Manual trigger:** An owner message such as `go ahead` requests an immediate bounded engineering run; it does not disable, reset, postpone, or replace the scheduled hourly run.
- **Scheduled trigger:** The **existing** Messy-File-Cleaner ChatGPT automation runs hourly independently of chat activity. User inactivity is not a blocker. Never create duplicate hourly tasks or claim Markdown itself schedules execution. If the task is paused, only its actual automation setting can resume it.
- **Real delivery:** A completed scheduler invocation is not a completed milestone. Prefer an open PR's latest-head CI/review repair or safe eligible merge; otherwise implement one independent READY source/test/CI improvement and open a reviewable PR. Do not repeatedly create docs-only status PRs.
- **Fallback:** If checkout is unavailable, attempt bounded GitHub connector source/test edits; label local tests NOT RUN and rely on subsequent exact-head CI. After two consecutive no-change attempts on the same blocker, escalate the precise owner/upstream decision and choose another independent READY non-destructive task.
- **C-specific verification:** Apply current-head C11/C17/C23 Debug/Release and sanitizer/CTest gates, Ubuntu 24.04 Docker, Makefile refusal tests, path-filtered TSan and isolated Rust test-only checks where applicable. A conditional workflow not triggered is NOT RUN, not PASS. Never run real user-file moves as a test.
- **Safety priority:** FS-01B same-UID source-retirement and REL-01 remain blocked until an accepted trust/crash model and positive preservation tests exist. No automated source deletion, production recovery, public ABI/FFI, release, paid resources, or protection bypass. Green CI cannot substitute for those decisions.
- **Report each run:** Trigger type (manual or scheduled), CODE CHANGED or NO CODE CHANGED and reason, verified old/new main SHA, PR head/base/merge SHA, changed files, per-gate exact-head conclusions and links, review/protection evidence, blockers/remedies and one next executable job. Distinguish IMPLEMENTED, PR_OPEN, CI_PASS, MERGED and RELEASED.
- **Reliability limit:** ChatGPT scheduling and connector availability may fail; report missed or blocked execution truthfully rather than promising uninterrupted autonomous progress.

## Scope of owner standing authorization

- Inspect current GitHub `main`, the LIVE NOW/NEXT/LATER board of [next.md](../next.md), open/merged PRs, related issue discussions, review threads, branch protections and CI.
- Start the single highest-priority dependency-unblocked bounded milestone; branch from **verified latest `main`**, change only necessary repo code, docs, fixtures, tests, CI or maintenance, and create/update a focused PR.
- Independently review the exact diff and threat/operational/business impact. Leave an honest review/comment (do **not** manufacture an independent second-person/code-owner approval). Repair CI failures in the same PR and rerun required checks.
- **Merge eligible low-risk, well-scoped PRs without a fresh per-PR owner prompt** only after all review and safety gates below are satisfied. Verify the exact PR head SHA at merge, use the connector's `expected_head_sha` safeguard, honor branch protections, then verify `main` and the merge SHA. Update the live register in the next authorized bounded work item. One automation invocation is one bounded milestone, not an endless loop.

## Mandatory gates before EVERY automated merge

1. Confirm this owner authorization is still active; reconcile the exact repository, base SHA, head SHA, mergeability, open PRs and changed files. Do not work on a different repository by accident.
2. Review the **entire scoped diff**, its functional and security consequences, test/CI coverage, negative cases, rollback and user-facing behavior. Check current PR conversations, reviews and **all unresolved review threads**. Never present your own review as independent approval.
3. All configured **required** checks must succeed for the current exact head SHA (C11/C17/C23 where applicable, sanitizers, relevant Docker and Ubuntu smoke, security and test-only Rust bounds). Determine which conditional workflows did *not* trigger; neither skipped nor unrun is a PASS. Respect actual GitHub branch protection and code-owner review restrictions.
4. Do not merge if the PR is a **known-risk characterization mislabelled as a fix**, adds untested destructive filesystem/recovery behavior, silently changes journal format or public ABI, activates production FCJ3/FCJ4/FFI outside an accepted ADR, weakens security or CI, leaves unresolved P0 data-safety assumptions, or misrepresents production/release readiness.
5. No automatic overwrite/deletion of real or customer files, cleanup by prefix, force push/history rewrite, secrets or credentials access/exposure, weakening branch protections, new paid infrastructure, deployment, licensing/price commitments, or release publication. These are separate task-specific decisions and may require human intervention regardless of PR CI status.
6. If an independent required reviewer or accepted design decision is missing, **stop with a transparent BLOCKED state** and request the precise decision; do not self-approve, force merge, claim universal same-UID safety or bypass checks simply to keep the schedule moving.
7. If all gates pass and scope is non-destructive and sufficiently verified, write a factual PR review/comment, merge with **expected exact head SHA**, confirm GitHub's merged result and main SHA, then report what changed, test evidence, residual risk, and next eligible task.

## Examples and boundaries

- A documentation-only status reconciliation with correct evidence, zero unresolved comments and all required PR-head CI green is generally an eligible autonomous merge.
- A contained test-only regression with explicit scope and completed relevant tests may be eligible for autonomous merge if it does not enable risky production features.
- The **FS-01B-FIX** source-retirement race is **not** resolved by group/other-writable permission checks. A high-risk production unlink/quarantine change stays blocked until the adversary model, authority, crash/orphan recovery and **positive preservation regression** are accepted and verified. A green test that reproduces unsafe behavior is not proof of safety.

## Status, auditability and cancellation

- Record exact links, branch/base/head/merge SHAs, changed paths, reviewer findings, CI/job conclusions, negative tests actually run or NOT RUN, how risks are mitigated, and rollback in the PR and [next.md](../next.md). Do not erase historical evidence.
- An autonomous merge is **MERGED**, not RELEASED. Starting future scheduled work requires the configured external scheduler; repository Markdown alone does not run anything.
- This permission remains until the owner changes or revokes it (for example by pausing or editing the existing ChatGPT automation). If GitHub access or authorization expires, stop and report the blocker. Recheck instruction priority and current owner decisions on every run.

**Authorities:** [AGENTS.md](../AGENTS.md) sets the primary repository safety contract; [skills.md](../skills.md) is procedural, the [handoff runbook](AGENT_CONTINUATION_AND_HANDOFF.md) specifies run boundaries, and the [LIVE work register](../next.md) selects tasks. This page explains how the owner's later standing merge authorization is applied without overriding security invariants.
