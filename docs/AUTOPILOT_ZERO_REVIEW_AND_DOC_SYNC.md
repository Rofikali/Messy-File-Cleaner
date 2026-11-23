# Autopilot Governance — Zero Required Human Reviews and Main Protection

**Owner decision, 23 November 2026.** Eligible scoped engineering and governance pull requests can be created, modified, inspected, tested, and merged by the authorized autopilot without mandatory independent human approval. GitHub ruleset must set `required_approving_review_count: 0`. An agent's review of its own change is a **self-review**, not a second-person APPROVED review. This supersedes the earlier mandatory-independent-review language for *ordinary safe PRs only*.

## Install protection (not automatically active)

Import [main protection JSON](../.github/rulesets/autopilot-main-no-human-review.json) via GitHub **Settings > Rules > Rulesets > Import**. Protect `main` from deletion and non-fast-forward pushes, require PRs (zero approving reviews), resolved review threads, strict up-to-date required CI, and no bypass actors. Current required contexts are feature-budget validation, C11/C17/C23 matrix checks and Docker smoke. Verify actual context names against Actions before enabling. Path-filtered Ubuntu/TSan/specialty jobs remain *conditional* and are not mandatory for every docs PR. A checked-in JSON file does NOT install the ruleset; only GitHub admin settings can.

**For the runnable continuation checklist and cross-discipline acceptance:** [Hourly Autopilot — Continuation and Recovery Contract](AUTOPILOT_EXECUTION_CONTINUATION.md). It defines how to resume unfinished work, distinguish scheduled execution from delivered code, and avoid repeated no-change audits.

## Complete bounded engineering lifecycle

1. **Observe:** query live GitHub main SHA, current open/merged PRs, exact head/base, CI jobs/checks, review comments/threads, applicable rulesets, open security issues and the current milestone. Never follow a stale OPEN label as live fact.
2. **Implement:** prefer an existing safe PR (including #148 when still open); otherwise one bounded feature with stable Feature-ID and no more than five opened PR attempts. Apply C17 ownership, C11/C23 compatibility, isolated Rust-test-only scope, Linux/Docker/CI, security and measurable performance. Use disposable fixtures, negative tests and no real user data.
3. **Review:** autopilot must inspect the full diff, test plan and evidence, data-loss and threat-model implications, rollbacks, docs and review threads; leave an honest factual self-review comment, never impersonate an independent reviewer or approve its own PR.
4. **Verify and merge:** only when every configured required check is successful for the current PR head and latest base, any applicable security jobs are verified, branch rules and blocking review threads are satisfied, and no unresolved high-impact safety problem is introduced. Missing/skipped tests are NOT RUN, not PASS. Use `expected_head_sha` and verify the resulting main SHA; never bypass checks, force push or quietly downgrade protection.
5. **Reconcile docs:** after merge, check integrated-main CI (a PR can pass while main fails); record merge SHA, feature DONE/PARTIAL/BLOCKED, next READY candidate and outstanding manual acceptance. Update `docs/feature-budget.json`, `next.md` and impacted `AGENTS.md`, `README.md`, `skills.md`, `SKILLS.md`, documentation map, HLD/LLD, FS-01/FCJ, CI/Docker, security and business documents in the next substantive PR, **or use a bounded factual reconciliation PR when the live selector is materially wrong**. Avoid cosmetic date-only PRs and redundant status text.

## Safety and business boundaries

**FS-01B source-retirement same-UID check/use race is P0; REL-01 remains blocked.** No autonomous real-customer source deletion/moves, production recovery, unapproved ABI/FFI changes, release/deployment, secrets/visibility changes or spending. Current C17 production and test-only Rust boundaries remain. Security/SRE, L1–L7, CA/MBA and management are review lenses, not claimed certifications. A zero-approval ruleset deliberately sacrifices independent separation of duties; it is appropriate only with conservative scope, real CI and recorded evidence. Stop for a specific irreversible or owner-only decision.

## Dated findings (23 November 2026)

[PR #147](https://github.com/Rofikali/Messy-File-Cleaner/pull/147) merged at `d839ad608c554d0c7715dc6264be6952ef07eccf`; four main CI workflow families passed. The previously inspected ledger and live board still called it OPEN; [PR #148](https://github.com/Rofikali/Messy-File-Cleaner/pull/148) was subsequently open. Treat both as dated facts, not a permanently valid NOW directive. Check current PR #148 and its branch before reconciling to avoid clobbering its updated ledger.

**Benefits:** no human-approval bottleneck, checked PR history, repeatable delivery and documentation trace. **Costs:** no independent reviewer, CI cannot prove every safety invariant, strict checks can delay merges, and an extra reconciliation PR may sometimes be necessary.
