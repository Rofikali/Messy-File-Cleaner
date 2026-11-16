# Seven-Level Engineering, Security, CTO and Business Operating Model

**Status:** Review method; not a certificate, personnel hierarchy, auto-merge policy, or authority to deploy. Applies to Messy-File-Cleaner, a **local Linux/POSIX C17 production tool** with isolated Rust test support. It is not IMGENGINE's Rust API / Nuxt / SaaS architecture.

## Mission and decisions

**North star:** user file preservation and clear recoverability above throughput, feature count, language fashion and release speed. Risk priority: data integrity -> security -> operational correctness -> measurable throughput -> code clarity -> cost and delivery.

| Perspective | Primary question | Expected evidence |
| --- | --- | --- |
| L1 Junior | What is the exact behavior and affected code? | Small clean diff; focused regression; reproduction steps |
| L2 Mid-level | Does the bounded feature work end to end? | Happy/error paths, ownership, resource cleanup |
| L3 Senior | Is it reliable in failure and concurrency cases? | Negative cases, transaction sequencing, lifecycle proof |
| L4 Staff | Are subsystem boundaries and dependencies correct? | HLD/LLD changes, ports/adapters where needed, no leaked test modules |
| L5 Senior Staff | Does it cooperate with CI, packaging and adjacent modules? | Compatibility matrix, isolation/rollout/rollback, reproducible environments |
| L6 Principal | Is this the safest minimal long-term design? | Explicit tradeoffs, benchmark evidence, alternative evaluation |
| L7 Distinguished | What reusable engineering rule prevents a class of failures? | Documented invariant, regression strategy, accountable review gate |
| Security engineer | Can hostile path/journal/config/concurrency input lose files? | Threat model, symlink/TOCTOU/crash/fault tests, fail-closed result |
| SRE / release engineer | Can we reproduce, diagnose, restore and roll back? | Build provenance, resource behavior, non-root smoke, release runbook |
| CTO | Is this work necessary for user trust and strategic product direction? | Customer outcome, dependency order, cost, irreversible risk |
| CA / finance | What is total cost, liability and cash exposure? | Dev/CI/support/distribution cost assumptions, no invented sales figures |
| MBA / management | Who wants this and how will we measure adoption? | User interviews, activation/retention/support burden, opportunity cost |

Seven levels are **review lenses**, not seven independently acting AI agents or seven mandatory meetings. Scale scrutiny to blast radius. An innocuous doc fix is not a journal rewrite.

## Architectural rules

- C17 modular monolith and explicit production source list remain; C11/C23 tested compatibility; Linux/POSIX API boundaries.
- DDD only to express meaningful file-operation and journal domain concepts. Hexagonal ports for filesystem, journal, clock, I/O and test fault injection where testability benefits; no speculative framework.
- SOLID/DRY/KISS/YAGNI adapted to C: cohesive modules, narrow headers, fixed ownership, bounded allocations, single responsibility, clear status returns.
- FCJ2/legacy production; FCJ3 diagnostic/test-only; FCJ4 parser, writer and Linux test append adapter **test-only**. Never infer that valid replay authorizes source deletion or real recovery.
- Rust test tools only under accepted ADR. No production Rust, C API, static/shared library or cross-language ABI without reviewed consumer, acceptance tests and new authorization.
- No brokers, database, microservices, cloud, desktop shell or handwritten assembly without a demonstrated need. No remote network access requirement for current CLI.
- All filesystem mutation tests use disposable fixtures; never exercise customer data, host root or unreviewed production journal.

## Mandatory decision memo for nontrivial changes

Document: problem/customer outcome; exact baseline SHA; current behavior and violated invariant; options including doing nothing; scope/non-goals; C modules/ownership/lifetime/thread topology; syscall/security implications; tests (success/negative/crash); performance/resource budget; backward compatibility; support/CI cost; rollback; owner sign-off needed. Do not attach speculative nanosecond speedups or guarantees.

## Review gates by change

| Change class | Focused dev check | Mandatory review/CI gate |
| --- | --- | --- |
| Docs-only | Markdown links, truth of claims, changed-doc scope | Existing PR workflows; no false "new tests passed" |
| C scanner/config/classifier | Affected unit/integration | C11/C17/C23 + ASan/UBSan, security tests |
| Mover/journal/recovery | Focused fault-injection/crash fixtures | Full C matrix, non-root Docker, syscall/identity/adversarial review, owner approval |
| Queue/thread/watcher | Topology and lifecycle tests | Race/TSan as relevant, cancellation, C matrix, no SPSC to MPSC silent change |
| Rust test support | cargo fmt/clippy/test | Production-link isolation, existing C gates |
| Makefile/CI/Docker | Dry-run help/refusal, target-specific smoke | CI and non-root runtime checks; no skipped protections |
| External ABI / SDK | ABI review and independent consumers | Versioning, symbol/allocator/lifetime tests; separate decision |
| Public release | Exact candidate end-to-end + threat/resource evidence | Human go/no-go, rollback plan, operational restrictions |

## Management cadence and release decisions

- Weekly: reconcile `next.md` against merged PRs, failing CI and risk register. Select one eligible small milestone.
- Per PR: record exact changed SHA, tests/run URLs, known gaps, and rollback. Keep pending/failed/untested honest.
- Monthly: inspect defect classes, unresolved risk, CI minutes, engineering time and potential user/customer feedback. Do not fabricate revenue/COGS or ROI.
- At release: require a real user and distribution need, license decision, reproducible artifact, safety support matrix, backup guidance, safe default behavior, and explicit human approval.

Use [agent continuation](AGENT_CONTINUATION_AND_HANDOFF.md) for the operational sequence and [next.md](../next.md) for the current work, not this document as a duplicate backlog.
