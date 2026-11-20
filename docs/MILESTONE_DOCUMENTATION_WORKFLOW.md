# Messy-File-Cleaner — Milestone Documentation Transaction

**Status:** Binding repository execution and PR-review procedure adapted from [IMGENGINE's milestone continuation runbook](https://github.com/Rofikali/imgengine/blob/main/docs/AGENT_MILESTONE_CONTINUATION_RUNBOOK.md) and compact [live queue](https://github.com/Rofikali/imgengine/blob/main/next.md). This is a C17/Linux/POSIX file-preservation project, **not** IMGENGINE's C ABI/SaaS/desktop platform and **not** Design Platform's Nuxt/Rust/WASM editor.

## Why documentation became stale

Historically `next.md` collected dated PR_OPEN and CI_PENDING paragraphs even after those PRs merged. Updating code on GitHub does **not** automatically update unrelated Markdown; a successful scheduled ChatGPT task is neither a fresh GitHub merge nor a documentation transaction. A previous successful test run cannot establish current main or PR-head status.

The **single live task selector** is now compact [next.md](../next.md); the original chronology is preserved unchanged inside [next-history.md](../next-history.md). Other documents are scoped contracts or historical evidence, **not competing live status dashboards**. Do not generate duplicate status paragraphs across dozens of docs.

## Transaction for every bounded engineering milestone

1. **Reconcile first:** query live `main`, all open and merged PRs, head/base/merge SHAs, relevant issues, review threads, protections, current PR-head check jobs. Existing PR before new work. Never use a historical PR_OPEN line as current proof.
2. **Implement and verify:** change C17 source/tests, isolated Rust test support, Docker/CI or other approved work inside **one bounded PR**. Preserve user-file safety and non-destructive fixtures. Run available Makefile/CTest/sanitizer and C11/C17/C23, Docker and conditional Rust/TSan gates; report PASS, FAIL, PENDING, SKIPPED, NOT RUN at exact SHA.
3. **Update documentation in the *same implementation PR*:** update compact `next.md` with selected milestone ID/state, tested head SHA, scoped delivery, actual PR/CI links, blockers and one next READY job. At PR creation avoid inventing a PR URL. For materially changed facts update README, AGENTS, `skills.md` and docs index, then the **affected** HLD, LLD, ADR, security, journal, API/ABI, benchmark, Docker, finance/CA/MBA/product documents only when their contracts or evidence change.
4. **Review the impact matrix:** write `UPDATED` or `N/A` for each affected domain in the PR template. `SKILLS.md` is a skill catalogue and not a second implementation policy; `skills.md` remains the contributor recipe. Keep the production C17 vs test-only Rust boundary explicit.
5. **PR integration:** inspect the exact full diff, negative tests, comments, current required checks, branch protection and independent review rules. Owner standing automation authorization permits safe low-risk merges only after gates; never count self-review as independent approval or skip required checks.
6. **After *actual* merge:** record the verified merge SHA and new main SHA in the **next authorized implementation PR**, not in a timestamp-only post-merge status PR. Distinguish IMPLEMENTED, PR_OPEN, CI_PASS, MERGED, RELEASED; preserve history in `next-history.md`. No automatic production publish.
7. **Blocker recovery:** if no checkout, use permitted GitHub connector for bounded source/test work, say local tests NOT RUN, then rely on exact-head CI. If two runs cannot deliver due to the same gate, escalate it and switch to independent READY safety work. No-op runs are not engineering delivery.

## Per-PR documentation impact matrix

| Area | Required review | UPDATE only when |
| --- | --- | --- |
| Current milestone and owner handoff | `next.md`, PR head/merge link, next READY task | Every material milestone; preserve historical record separately |
| User entry points and instructions | `README.md`, `AGENTS.md`, `skills.md`, `SKILLS.md`, `docs/README.md` | The scope, authority, command or public status changes |
| Native C17 contract and portability | HLD/LLD, public headers, CMake/Makefile, C11/C23 test matrix | APIs, ownership, cross-standard behavior or build guarantees change |
| Filesystem threat and recovery | FS-01/FCJ3/FCJ4 threat model, journal protocol, positive preservation tests | Path trust, source retirement, durability, data-loss assumption or recovery changes |
| Performance and resources | Disposable benchmark harness, fixture provenance, measured environment | Benchmark result, instrumentation, safety or measurable performance changes |
| Deployment/security | Ubuntu 24.04 Docker, workflow security, support/backup/release contracts | Image/CI/runtime/security or release evidence changes |
| Business/finance/management | Unit economics, licensing, support, SDK strategy | Actual financial or product decisions, not mechanical C tests |

## Evidence, limitations and change discipline

- **CI cannot prove live status unless it queries and verifies GitHub.** A markdown validator can prove syntax or links are valid, not that an old PR_OPEN is still open. Exact-head CI belongs to the linked workflow run.
- **Avoid a new docs generator/runtime solely for this repository.** CMake/CTest and existing repo documentation are authoritative; the compact one-board-and-history pattern is simpler, DRY and appropriate to the pure-C product. A future generator/CI gate requires a separate reviewed milestone and tests.
- **No data-destructive operations:** the known same-UID source-retirement check/use race remains unresolved. Positive preservation and accepted trust/crash designs are prerequisites for any production fix/release. Passing negative tests, auto-merge authorization and CMake checks do not override safety approval.
- **No spending or deployment:** Markdown is not an unattended worker, GitHub Actions is not a coding agent, and a ChatGPT scheduled automation can miss a run or have unavailable connectors. Report the exact blocker without claiming work occurred.
