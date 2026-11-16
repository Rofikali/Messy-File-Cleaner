# Messy-File-Cleaner — Documentation Map and Authority

**Read this before starting a milestone.** This index borrows IMGENGINE's living documentation *structure*, not its stack, product or operational assumptions. Sources, actual behavior, versioned protocol contracts and exact-SHA CI evidence outrank historical notes.

| Question | Canonical source |
| --- | --- |
| What is the product, implemented scope and user-facing usage? | [README](../README.md) |
| What should be done NOW, NEXT and LATER? | [Live NOW / NEXT / LATER register](../next.md) (authoritative task selection; historical notes explicitly marked) |
| What is fully built, partially implemented, started, test-only, blocked or future? | [Component completion and status inventory](PROJECT_COMPLETION_AND_STATUS_INVENTORY.md) (evidence and remaining customer-journey gaps, not a second live backlog) |
| What may agents do and what requires approval? | [AGENTS](../AGENTS.md) |
| How to perform recurring engineering tasks? | [skills.md](../skills.md) is the operational recipe; [SKILLS.md](../SKILLS.md) is a non-binding learning catalogue and does **not** approve technologies, merges or runtime changes |
| Seven engineering levels, CTO/security, decision and review criteria? | [Engineering operating model](ENGINEERING_OPERATING_MODEL.md) |
| How to resume/handoff a bounded PR? | [Agent continuation](AGENT_CONTINUATION_AND_HANDOFF.md) |
| What runs in CI, what Make targets do, and what counts as proof? | [CI/CD and Makefile operations](CI_CD_AND_MAKEFILE_OPERATIONS.md) and [Makefile](../Makefile) |
| Which market, cost, free/paid or SDK opportunities are hypotheses? | [Product/business strategy](PRODUCT_AND_BUSINESS_STRATEGY.md) |
| What modules and dependencies exist? | [HLD](HLD.md), [LLD](LLD.md), [engineering playbook](architecture/ENGINEERING_PLAYBOOK.md) |
| What threatens user data, and what is proposed for the P0 retirement bug? | [Threat model](security/THREAT_MODEL.md), [FS-01A mover/journal audit](security/FS_01_PRODUCTION_FILESYSTEM_AUDIT_2026-11-16.md), [FS-01B merged known-risk characterization](quality/FS_01B_RETIREMENT_RACE_CHARACTERIZATION.md), [proposed source-retirement ADR](architecture/ADR_FS_01B_SOURCE_RETIREMENT_BOUNDARY.md), and [positive security acceptance plan](quality/FS_01B_FIX_SECURITY_ACCEPTANCE_PLAN.md) |
| How does journal parsing/recovery work? | [FCJ4 schema](quality/FCJ4_SCHEMA_AND_REPLAY_DESIGN.md), [FCJ4 execution safety](quality/FCJ4_RECOVERY_EXECUTION_SAFETY_DESIGN.md), [other quality docs](quality/) |
| What is Rust allowed to do now? | [Accepted Rust ADR](architecture/ADR_RUST_ADOPTION_STRATEGY.md), [test-support boundary](architecture/RUST_TEST_SUPPORT_BOUNDARY.md) |
| How are C API/static/shared libraries proposed? | [C API/ABI/FFI roadmap](architecture/C-API-ABI-FFI-ROADMAP.md) |
| Where are container instructions? | [Docker architecture](docker/DOCKER-ARCHITECTURE.md), [Docker runbook](docker/DOCKER-RUNBOOK.md) |
| Where are benchmark baselines? | [Performance baseline](performance/CI-BASELINE-2026-11-04.md), [performance guidance](performance/PERFORMANCE-ENGINEERING.md), [profile artifact retention and provenance](performance/PROFILE_ARTIFACT_RETENTION_AND_PROVENANCE.md) |

## Precedence

1. Explicit current owner decisions and accepted ADRs, within mandatory filesystem and data-preservation safety constraints; no automatic approval of risky actions.
2. Data preservation, security and recovery invariants in AGENTS/threat model.
3. Existing on-disk journal compatibility and current source/API contracts.
4. Architecture playbook and module-specific design docs.
5. Milestone instructions, operational recipes and historical reference documents.

If documents conflict, preserve user data, record the conflict, and seek owner resolution before behavior-changing implementation. In particular, **C17 production + isolated test-only Rust** is the accepted decision; phrases like "pure C" describe the production core, not every file in this repository.

## Status truth rules

- `DESIGN_ONLY` ≠ `IMPLEMENTED` ≠ `CI_PASS` ≠ `MERGED` ≠ `RELEASED`.
- Date every audit snapshot with the exact Git SHA; always consult current GitHub state.
- Keep **current** task selection/status in the top LIVE BOARD of `next.md`; preserve its historical checkpoint section without treating stale labels as current. Detailed per-component maturity and blockers belong in [completion inventory](PROJECT_COMPLETION_AND_STATUS_INVENTORY.md), technical contracts in their canonical documents, exact evidence in PR/CI links. Do not duplicate giant logs.
- In this repo, **"go ahead" / "continue" / "next"** tells a running agent to reconcile GitHub state and begin the next READY bounded job from the live `next.md` board; it is not authority to merge, deploy, spend, delete real files, or bypass gates. See [agent continuation](AGENT_CONTINUATION_AND_HANDOFF.md).
- Docs/Makefile cannot autonomously start an agent, release software, or authorize destructive file handling.
- No deployment, production FCJ4 recovery, external static/shared library or paid feature is authorized by this index.
