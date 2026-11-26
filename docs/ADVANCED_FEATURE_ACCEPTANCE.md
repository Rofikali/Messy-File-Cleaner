# Messy-File-Cleaner — Advanced Feature Acceptance and Review Matrix

**CPU guard-page acceptance:** [baseline](performance/CPU_OPT_REFERENCE.md) requires 82560 checks at both Linux guard-page edges with read-only input; non-Linux SKIP is not a pass; retain production classifier.

**CPU dispatch acceptance:** [baseline](performance/CPU_OPT_REFERENCE.md) requires 32 independent policy cases, all four allowed-mask selections on 165120 inputs each, forced portable identity and capability-tagged timing.

**CPU explicit vector acceptance:** [baseline](performance/CPU_OPT_REFERENCE.md) requires both wrappers equal libc across all 165120 cases each, capability-tagged execution, safe tails and 54–108 checked timing rows.

**CPU timing acceptance:** [baseline](performance/CPU_OPT_REFERENCE.md) requires 54 rows with every result checked, unchanged correctness suite and actual compiler/disassembly evidence before ISA claims.

**CPU optimization reference acceptance:** [baseline](performance/CPU_OPT_REFERENCE.md) requires 165120 exact scalar/libc comparisons and zero-length NULL checks, directly instrumented Debug, portable Release; no performance or explicit SIMD claim.

**Cache-layout final candidate:** [baseline](performance/CACHE_LAYOUT_REFERENCE.md) requires directly instrumented Debug reference execution with fatal ASan/UBSan and ordinary Release checks; no production adoption or packed TSan claim.

**Cache-layout capacity acceptance:** [baseline](performance/CACHE_LAYOUT_REFERENCE.md) requires oversized logical count/bytes rejection before access and four immutable readers with 64 exact digests each; no concurrent reclamation claim.

**Cache-layout ownership acceptance:** [baseline](performance/CACHE_LAYOUT_REFERENCE.md) requires malformed input rejection, two injected allocation failures, occupied-output rejection, source-free lifetime, seven corrupted representations rejected and repeatable cleanup.

**Cache-layout timing acceptance:** [baseline](performance/CACHE_LAYOUT_REFERENCE.md) requires six original footprint cases and 36 verified timing rows, independent ownership, bounded packed offsets and full-content equivalence; no latency threshold.

**Cache-layout acceptance:** [baseline](performance/CACHE_LAYOUT_REFERENCE.md) requires exact ordered path/type contents, bounded offsets, NUL terminators, matching full-byte digests and reported storage for all six cases.

**Batch I/O failure acceptance:** [baseline](performance/BATCH_IO_REFERENCE.md) requires interrupted/short-write completion, zero-progress and permanent-EIO termination, exact committed prefix and original source preservation. Injected returns are not kernel fault evidence.

**Batch I/O timing acceptance:** [baseline](performance/BATCH_IO_REFERENCE.md) requires all 21 original content/call-count comparisons and 18 additional verified transfer timing rows, no latency threshold or production speedup claim.

**Batch I/O acceptance:** [baseline](performance/BATCH_IO_REFERENCE.md) requires 21 complete independent transfers, exact bytes and offsets, bounded iovecs and partial write handling; actual call counts are observational.

**io_uring mixed completion acceptance:** [baseline](performance/LINUX_URING_REFERENCE.md) requires completed write batch before FSYNC, exact negative EBADF/zero EOF results, untouched sentinel buffers, and four valid recovery READV completions.

**io_uring vectored acceptance:** [baseline](performance/LINUX_URING_REFERENCE.md) requires NOP regression, four READV and four WRITEV successful exact-length completions, unchanged descriptor offsets and original source bytes, with exact destination contents.

**io_uring NOP acceptance:** [baseline](performance/LINUX_URING_REFERENCE.md) requires 16 unique successful completions, exact submission count, zero dropped/overflow and mapping cleanup; capability skip NOT VERIFIED.

**io_uring capability acceptance:** [baseline](performance/LINUX_URING_REFERENCE.md) requires actual setup and valid operation probe with NOP/READV/WRITEV/FSYNC support; unsupported build/runtime explicitly NOT VERIFIED, no security-policy bypass.

**Linux I/O range acceptance:** [baseline](performance/LINUX_IO_REFERENCE.md) requires three explicit offset cases, unchanged descriptor positions, exact source and destination bytes, and unchanged destination length. No production fallback or durability claim.

**Linux I/O timing acceptance:** [baseline](performance/LINUX_IO_REFERENCE.md) requires all six original boundary cases and six repeated comparison pairs, raw timings with exact contents/offset verification; skipped capability NOT VERIFIED.

**Linux I/O acceptance:** [baseline](performance/LINUX_IO_REFERENCE.md) requires complete independent byte verification of source and both destinations on six sizes; unsupported candidate NOT VERIFIED, no fallback or source retirement.

**Watcher lifecycle acceptance:** [baseline](performance/LINUX_WATCH_REFERENCE.md) requires four actual watcher cycles, two cooperative stops, two root deletions, four failed registrations and descriptor-count restoration with no additional file handoff.

**Watcher batch acceptance:** [baseline](performance/LINUX_WATCH_REFERENCE.md) requires 64 distinct real file events, one ignored directory, multi-record read, exact raw membership, matching app counter deltas and unchanged file contents.

**Linux watcher real-event acceptance:** [baseline](performance/LINUX_WATCH_REFERENCE.md) requires actual production ESTALE after root rename, no new app events, and raw watch identity evidence after former path replacement. No production recovery or real queue-overflow proof.

**Path error acceptance:** [baseline](performance/LINUX_PATH_REFERENCE.md) requires nine exact error comparisons and a subsequent valid identity comparison, with no unrestricted fallback. Unsupported capability stays NOT VERIFIED.

**Path identity acceptance:** [baseline](performance/LINUX_PATH_REFERENCE.md) requires held original directory and child identities survive a rename, replaced lexical name resolves distinct objects, and relocated child matches original identity. No source-retirement claim.

**Path resolution acceptance:** [baseline](performance/LINUX_PATH_REFERENCE.md) requires successful capability probe and 11 stable private-fixture comparisons; missing headers/syscall or policy-denied probe NOT VERIFIED, not a capability pass. Production trust and FS-01B remain unchanged.

**Scanner reporting acceptance:** [reference baseline](performance/SCANNER_REFERENCE_BASELINE.md) requires successful existing C17 Release smoke, verbose sample evidence and environment identification; keep production scanner until trust and resource equivalence are established.

**Scanner stop acceptance:** [reference baseline](performance/SCANNER_REFERENCE_BASELINE.md) requires 18 deterministic stop/rescan pairs with exact unique partial callbacks, native errno, nftw callback return, descriptor restoration and complete recovery set.

**Scanner descriptor acceptance:** [reference baseline](performance/SCANNER_REFERENCE_BASELINE.md) requires all eight untimed traversals, exact callback sets, scanner callback-visible deltas 2 wide/13 deep and baseline restoration. nftw budget is observational, not a process-wide cap.

**Scanner evaluation acceptance:** [baseline](performance/SCANNER_REFERENCE_BASELINE.md) requires exact regular-file callbacks with no duplicates or symlink traversal in private wide/deep fixtures; nftw is test-only and does not establish equivalent trust/cancellation contracts.

**String measured evidence:** [extension baseline](performance/STRING_EXTENSION_BASELINE.md) records exact-blob standalone correctness and sanitizer pass, and all 30 C17 timing samples. Production adoption remains unapproved pending representative workload/target-host evidence.

**String boundary acceptance:** [extension baseline](performance/STRING_EXTENSION_BASELINE.md) requires 299,615 untimed cases, independent forward-reference routing, unchanged map size and identical borrowed result pointers for current/candidate implementations.

**String acceptance:** [extension baseline](performance/STRING_EXTENSION_BASELINE.md) requires null/empty, hidden, trailing-dot, dotted-directory, unknown, uppercase and multi-extension semantics unchanged, exact results and unchanged map size. No new production algorithm.

**Memory unwind acceptance:** [path allocation](performance/PATH_ALLOCATION_BASELINE.md) requires all 66 modeled cases, rejection without arena byte/offset mutation, intact ancestors, release balance and reuse. Actual scanner lifetime and system allocator failure remain separate.

**Nested arena acceptance:** [path allocation](performance/PATH_ALLOCATION_BASELINE.md) requires verified path contents, preserved ancestors after sibling release and frame unwind, balanced heap requests, zero live state at round end, and bounded smoke. No production ownership change.

**Arena acceptance:** [path allocation](performance/PATH_ALLOCATION_BASELINE.md) requires exact-fit and exhausted/short-buffer rejection without mutation, output and live-copy independence, balanced successful allocations/frees, cleared borrowed pointers before reset and bounded smoke. No production ownership change.

**Memory baseline acceptance:** [path allocation](performance/PATH_ALLOCATION_BASELINE.md) verifies every owned path, independent live copies, balanced successful joins/frees and bounded fixtures. No real filesystem access, allocator replacement or RSS claim.

**Collision acceptance:** [baseline](performance/HASHMAP_BASELINE.md) requires bounded fixture discovery, exact wraparound placement at 64 slots, full-table rejection and update preservation, verified values/misses and bounded smoke. No production or ABI changes.

**Classifier capacity acceptance:** [baseline](performance/HASHMAP_BASELINE.md) requires every mixed-path classification and case count verified, unchanged map size, same fixture and insertion order across capacities, bounded smoke and no production change.

**Hash-map capacity acceptance:** [baseline](performance/HASHMAP_BASELINE.md) preserves identical insertion/key sets across capacities, verifies every hit/miss and reports circular occupied cluster length and logical storage. No timing threshold or production change.

**Hash-map acceptance:** [baseline](performance/HASHMAP_BASELINE.md) runs three verified hit and miss samples at each of four occupancies, exact values and absent-key results, no latency threshold. No production replacement or proof of hostile-key resistance.

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
