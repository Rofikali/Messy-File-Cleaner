# Messy-File-Cleaner — Advanced Feature Acceptance and Review Matrix

**Affinity acceptance:** [queue baseline](performance/QUEUE_BASELINE.md) requires two distinct allowed logical CPUs, verified per-thread masks, input rejection before measurement, unchanged default smoke and full payload verification; no timing threshold. Pinned positive run only on an eligible host.

**Cache-layout acceptance:** original and generated candidate compile the same concurrent benchmark and queue implementation; candidate verifies index alignment and separation, both verify full FIFO payload and bounded retry. No speed threshold or promotion based solely on smoke; [record evidence](performance/QUEUE_BASELINE.md).

**Concurrent queue baseline acceptance:** unique full payload per transfer, exactly one producer/consumer, three fixed samples, peer failure signal, monotonic retry deadline and external CTest timeout; no timing threshold, production change or weak-memory proof. See [queue baseline](performance/QUEUE_BASELINE.md).

**Queue evaluation acceptance:** [bounded baseline](performance/QUEUE_BASELINE.md) records three samples, each 65,536 verified event pairs, monotonic duration, no threshold and no production change. Smoke success is correctness evidence, not optimization evidence.

**Upstream audit evidence:** [Module assessment](upstream/UPSTREAM_MODULE_AUDIT.md) records inspected baseline, verified references, rejected replacements and remaining test/measurement gaps. Existing C queue is retained; full-ring recycle regression is distinct from concurrent FIFO stress. No upstream code or dependency adoption.

**Owner-approved upstream-first policy (2026-11-25):** Follow [reuse policy](upstream/UPSTREAM_REUSE_POLICY.md) and [master plan](upstream/UPSTREAM_MASTER_PLAN.md). Prefer supported Linux/POSIX APIs and mature compatible components; retain working code unless comparative evidence justifies change. External source imports and new third-party dependencies are BLOCKED pending explicit owner license decision and component acceptance. C17, C11/C23 CI, FS-01B/REL-01, no-spend and five-PR constraints remain binding.

**Status: proposed prospective acceptance rubric (2026-11-22); not a test report or blanket approval.** Companion to [advanced roadmap](ADVANCED_FEATURE_ROADMAP.md), [live selector](../next.md), [existing seven-level operating model](ENGINEERING_OPERATING_MODEL.md), and [five-PR feature policy](FEATURE_DELIVERY_AND_ACCEPTANCE_POLICY.md). Existing risk: FS-01B same-UID source-retirement check/use, legacy recovery and crash guarantees still BLOCK production release. Do not declare this resolved on CI alone.

## Feature intake contract (applies before work)

For each future feature candidate record: (1) stable Feature-ID and reason for independent scope; (2) verified current main SHA/open PRs; (3) observable customer or risk outcome; (4) current baseline and non-goals; (5) architecture and trust assumptions; (6) positive and negative acceptance tests; (7) cost/support implications; (8) decision/rollback; (9) exact authority and required approvals; (10) docs updated/N/A. Register as ACTIVE in `docs/feature-budget.json` only when selected in `next.md`; one feature is active, max five **opened PR attempts** per feature, and each actual PR number belongs in its own PR. Avoid gaming the budget with renamed test-only follow-ups.

## Seven engineering levels and specialist review

| Lens | Explicit question | Required artifact/evidence when applicable |
| --- | --- | --- |
| L1 implementation | What exact code path and negative case fail? | Reproducible issue or user request, narrow patch |
| L2 module integration | Are CLI, config, scanner, classifier, worker and journal behavior consistent? | End-to-end disposable integration test |
| L3 concurrency/reliability | What if allocation, cancellation, signal, syscall, watcher event or worker fails? | Error/cancel tests, lifecycle and bounded-resource checks |
| L4 staff architecture | Are module contracts/ownership/ports correct without speculative abstraction? | Focused HLD/LLD/ADR changes and stable boundaries |
| L5 cross-platform/build | Does it remain C17 and pass C11/C23 compatibility on the supported toolchain? | CMake/CTest, Ubuntu/Docker, sanitizer evidence by SHA |
| L6 principal alternatives | Is the design the simplest safe long-term choice? | Alternatives, portability, performance and failure tradeoffs |
| L7 distinguished principles | Which invariant and regression prevent recurrence across the codebase? | Enforced acceptance rule, review and test strategy |
| Security / CISSP | Can hostile paths, names, symlinks, same-UID actors, permissions, journals or races cause data loss? | Threat boundary, fail-closed behavior, no-clobber, preservation tests |
| Linux / POSIX | Are FD, inode, directory and filesystem assumptions supported on named systems? | Syscall semantics, mount/filesystem matrix, error handling |
| QA / SDET | Do tests positively prove source and unrelated-file preservation? | Disposable fixtures, fault injections, negative cases and test oracle |
| SRE / release | Can operators reproduce, inspect, back up and stop safely? | Provenance, packages, rollback, support matrix, incidents, manual test record |
| CCIE / network | Is a remote API actually needed? If none, what is network-exposure scope? | For current offline CLI: **N/A**; for future networking: authenticated threat model, transport and operational design |
| CA / finance | What are development, CI/storage, support, legal and data-loss exposure? | Assumption-based cost register with actual spend, no invented ROI |
| MBA / management | What is validated demand, who owns support, and what should be deferred? | Interviews, prioritized value, dependency and opportunity-cost decision |

Only apply proportionate lenses. Review perspective ≠ professional certification, a mandated seven-agent topology, or authorization to release.

## Acceptance gates by advanced feature family

| Family | Minimal measurable acceptance | Release / approval blocker |
| --- | --- | --- |
| Read-only planner and JSON report | Tests prove no source/destination/journal mutation; stable schema/version, escaping, deterministic ordering, partial error and privacy policy; retain existing `--dry-run` semantics | No implicit execution; stale plan cannot grant deletion approval |
| Rule engine, metadata and duplicates | Deterministic precedence, invalid-rule refusal, bounded parsers/hashes, known-file edge cases, no private-path telemetry | No arbitrary script execution or automatic duplicate deletion |
| Filesystem mutation, move, undo and recovery | Approved same-UID threat/crash design, positive unrelated-file preservation, no-clobber, interruption/failure-state inventory and host-specific evidence | **FS-01B-FIX P0 and REL-01 BLOCKED** until accepted design and owner signoff |
| Watcher, rescan and cancellation | Induced event overflow and idempotent bounded rescan, no missed-error success, cancellation leaves data untouched or conservative state | Watcher input may not authorize blind destructive action |
| Native scanner evidence acceptance | Bounded native C harness; exact binary fingerprint, count and sample validation, recomputed summaries, compiler/version and workflow provenance; manual workload not dispatched | CI smoke and synthetic invalid-evidence tests are not target-host measurements; final BENCH-SCALE-01 slot, measured acceptance pending |
| Large-scale performance | Median/p95/p99, CPU, RSS, FDs, per-file throughput, cache/size/distribution, compiler/kernel/storage, repetitions; no hidden 1m CI cost | No unmeasured nanosecond claims; preserving safety is mandatory |
| Native C API / `.a` / `.so` | Real consumer, opaque handle, ABI version, symbol visibility, fixed-width contracts, memory ownership, C tests, upgrade compatibility, license | Distinct owner-approved ADR; not implied by test-only Rust |
| Rust FFI | Proved consumer need, isolated unsafe boundary, panic containment, ownership and cancellation contract, cross-language ABI tests | Separate owner review; no production Rust/FFI automatically permitted |
| TUI/desktop | Same backend policy, preview-first review, accessibility, packaging, storage/permission audit, privacy and update model | Not a shortcut around FS-01B, release/license or supported OS gates |
| Product distribution / commercial | Supported filesystem/OS matrix, safe install/rollback, signed/reproducible artifact where approved, licensing, support/incident and backup guidance | Explicit REL-01 owner approval; no promised SLA/enterprise claims or paid resource activation |

## Mandatory status and evidence record

- **Implementation:** NOT_STARTED / DESIGN_ONLY / STARTED / IMPLEMENTED / PARTIAL (or a justified equivalent).
- **Delivery:** PR_OPEN / CI_PENDING / CI_PASS_AT_SHA / MERGED_AT_SHA / RELEASE_ACCEPTED / RELEASED. Never substitute one for another.
- **Verification:** each test/job must include SHA, environment, command/workflow, result PASS/FAIL/SKIPPED/NOT RUN. Tests in a private temporary fixture are not real-filesystem operator or power-loss acceptance.
- **Security:** explicit unresolved risks, threat actors and data-preservation outcomes; passing a test that reproduces unsafe unlink is NOT a security fix.
- **Business:** actual engineering/CI/support spend, assumptions vs realized demand, license/reliance risks, owner decision and changes to ongoing obligations.
- **Documentation impact:** `next.md` living milestone, `docs/feature-budget.json` one ACTIVE ID, impacted README/AGENTS/skills/docs map/HLD/LLD/security/CI/perf/strategy, with UPDATED/N/A rationale; historical `next-history.md` left intact.

## One safe first milestone: PLAN-REPORT-01

The current C17 CLI already supports `--dry-run` and basic metrics. Review what exists before extending it. A first bounded PR may define and test **opt-in machine-readable read-only preview** without changing mover, journal, source retirement or default CLI behavior. Tests should include invalid roots/config, hostile strings, output framing/escaping, partial failures, limits, and **positive verification that input files, their bytes, destination trees and journal remain unchanged**. Where output/privacy semantics are undecided, produce a reviewable design and non-destructive tests instead of guessing. This is not production release or permission to auto-execute a plan.

## References

- [Repository security rules](../AGENTS.md), [contributor workflow](../skills.md), [threat model](security/THREAT_MODEL.md), [FS-01B acceptance](quality/FS_01B_FIX_SECURITY_ACCEPTANCE_PLAN.md)
- [Architecture HLD](HLD.md), [LLD](LLD.md), [C API/ABI/FFI plan](architecture/C-API-ABI-FFI-ROADMAP.md)
- [CI/Makefile operations](CI_CD_AND_MAKEFILE_OPERATIONS.md), [performance engineering](performance/PERFORMANCE-ENGINEERING.md), [release/business strategy](PRODUCT_AND_BUSINESS_STRATEGY.md)
