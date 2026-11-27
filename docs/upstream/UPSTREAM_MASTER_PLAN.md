# Linux upstream integration master plan

**Owner-approved Linux-inspired library architecture (2026-11-27):** [Engineering requirements](../architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). Stage 6 uses a Linux-inspired hybrid, own C user-space code and compatible individually cleared upstream components only. Reproducibility, dependency direction, symbol visibility and ownership are explicit gates, not authorization to import.

**Initial evidence:** [Module audit](UPSTREAM_MODULE_AUDIT.md) compares baseline implementation with Linux 6.12 circular-buffer documentation and Linux man-pages; no external source imported. Other upstream projects remain research candidates until individually examined. Queue full-ring regression strengthens acceptance without modifying production.

Owner-approved direction; proposed candidates, not implemented features or automatic queue. [next.md](../../next.md) is sole live selector. [Policy](UPSTREAM_REUSE_POLICY.md) governs imports.

## Official upstream research
| Source | Research scope |
|---|---|
| [Linux](https://github.com/torvalds/linux) | fs/, lib/, include/linux/, include/uapi/, tools/lib/, tools/include/, tools/testing/selftests/, Documentation/ |
| [glibc](https://sourceware.org/git/?p=glibc.git), [musl](https://git.musl-libc.org/cgit/musl/) | libc, allocator, strings; no duplicate libc |
| [Coreutils](https://git.savannah.gnu.org/cgit/coreutils.git/), [Findutils](https://git.savannah.gnu.org/cgit/findutils.git/) | copy/move/traversal |
| [util-linux](https://github.com/util-linux/util-linux) | CLI and user-space helpers |
| [liburing](https://github.com/axboe/liburing), [Userspace RCU](https://liburcu.org/) | async I/O and concurrency |
| [jemalloc](https://github.com/jemalloc/jemalloc), [LLVM](https://github.com/llvm/llvm-project) | allocation, compiler/sanitizer research |
| [SQLite](https://github.com/sqlite/sqlite) | transaction and durability concepts; no database dependency |

Kernel mm/, arch/, scheduler and workqueues are study-only. No device/filesystem drivers or kernel memory manager imports. Internal kernel code is not user-space ABI; prefer exported APIs and per-file license review.

## Module comparison and acceptance
| Module | Candidates | Required evidence |
|---|---|---|
| Scanner | Findutils, VFS, openat2 | descriptors, cancellation, first error, symlinks, rescan |
| Watcher | inotify/fanotify | event loss/invalidation, privileges, no unsafe handoff |
| Queue | kernel circular-buffer concepts | C atomics, concurrent FIFO, wraparound, cache locality |
| Workers | pthread/eventfd/workqueue concepts | lifecycle, error propagation, wakeups |
| Hash map | mature C algorithms | collision/capacity, ownership, small-map benchmarks |
| Classifier/config/CLI | libc, util-linux | unchanged routing, validation, safe diagnostics |
| Mover | Coreutils, copy_file_range | read-only research, no-clobber, preservation, fallback and crash gates |
| Journal | transactional systems/checksums | test-only malformed records, durability, recovery gates |
| Memory | malloc/arena/allocators | RSS, allocation count, failure cleanup |
| Tests | kselftest, sanitizers | deterministic negative tests, disposable fixtures |

## Staged roadmap
Provisional IDs must reconcile existing ledger before activation; no budget reset. One active feature, max five opened PRs each. Reference-only audit may proceed before license decision; imports may not.
| Stage | Candidates |
|---|---|
| 1 Governance | UPSTREAM-GOVERNANCE-01; UPSTREAM-LICENSE-01; UPSTREAM-AUDIT-01; UPSTREAM-PROVENANCE-01 |
| 2 Core | QUEUE-UPSTREAM-EVAL-01; HASHMAP-EVAL-01; MEMORY-POOL-EVAL-01; STRING-EVAL-01 |
| 3 Filesystem | LINUX-SCANNER-EVAL-01; LINUX-PATH-EVAL-01; LINUX-WATCH-EVAL-01; LINUX-IO-EVAL-01 |
| 4 Performance | LIBURING-EVAL-01; BATCH-IO-EVAL-01; CACHE-LAYOUT-EVAL-01; CPU-OPT-EVAL-01 |
| 5 Security/reliability | LINUX-SELFTEST-01; UPSTREAM-FAULT-01; SUPPLY-CHAIN-01; UPSTREAM-COMPAT-01 |
| 6 Libraries | PUBLIC-API-01; STATIC-LIB-01; SHARED-LIB-01; RUST-FFI-EVAL-01 (reconcile existing ABI roadmap; Rust test-only until approved) |
| 7 Maintenance | UPSTREAM-MAINT-01; UPSTREAM-COST-01; UPSTREAM-CONTRIBUTION-01 (external contribution needs authorization) |

## Architecture
Proposed CLI/SDK/desktop/API -> versioned C API/ABI (.a/.so candidates) -> independent domain engine -> internal adapters -> POSIX/Linux or approved libraries with fallback. No blanket restructure. DRY/SOLID/KISS, explicit ownership, design patterns, HLD/LLD and hexagonal/event-driven architecture only if justified. Existing C17 and Rust/FCJ test-only constraints preserved.

## Evidence, finance and management
Same compiler/build flags, kernel, CPU, filesystem, cache state, dataset and worker count; repeat benchmarks, report variation, throughput, latency/tails, CPU/cycles/cache misses where permitted, RSS, allocation/fd counts and correctness. 1k/10k/100k/1m/5m/10m are proposed fixture sizes, not achieved results or authorization for costly jobs. BENCH-SCALE-01 PARTIAL 5/5. TCO = integration + testing + maintenance + compliance + operations + opportunity cost; compare same horizon, assumptions and rollback. Free acquisition is not zero TCO. L1–L7, CISSP, Linux/SRE, CCIE, CA, MBA, management and legal review per existing rubric, not certification. Record exact source/release/SHA/license/notices, integrity, local changes, platform evidence, owner, update cadence and rollback for each approved component. Pin and review changes; no automatic upstream merges. No spending, release or destructive operations. Extend existing canonical scoped docs rather than duplicating them.


### Upstream maintenance boundary (design note)

Use supported Linux userspace interfaces, not internal kernel APIs. Kernel releases do not rewrite this repository; ordinary compatible syscall improvements can benefit execution after the host boots a patched kernel, while independent reference algorithms remain unchanged. The registry is empty/locked, no approved vendored component. If a component is later approved and imported, its pinned source, notices and local patch set require deliberate advisory/release review and targeted tests before updates; no automatic upstream merge. Track distribution security fixes/backports and actual runtime capability (not version alone); use [Linux regression policy](https://docs.kernel.org/process/handling-regressions.html) and [ABI stability documentation](https://docs.kernel.org/admin-guide/abi.html). Container uses host kernel, image updates alone do not replace it. Review relevant regressions and security advisories, rerun tests on supported environments, retain safe fallbacks, and open bounded PRs only for evidenced impact. UPSTREAM-MAINT-01 remains a future stage; this note does not authorize new automation or imports.


## UPSTREAM-MAINT-01 — observed environment baseline

#286 merged at `15e720b63f897709b31623c9cbea2cc081602c52`, tested head `1eee3583dd10fb42b7d021dc27825324a044238a`; 9/9 returned jobs passed, formal independent review not returned. FFI implementation remains BLOCKED at 1/5; preserve the existing approved FCJ4 Rust boundary and budget. #287 selects independent stage-7 reference-only maintenance. [Environment report](../../scripts/tests/report_upstream_environment.sh) is registered as a read-only test in the existing CTest matrix and records running Linux kernel release, machine architecture, glibc identity, CMake version and configured C compiler/linker version first lines. Configured linker identity is not proof of the executable actually invoked by every compiler driver. Report from the actual environment (container reports host kernel); no hostname, network, automatic upgrade, upstream merge or version-based capability/vulnerability assertion. Current scope Linux/glibc, not musl/other OS. CTest verbose output or LastTest.log holds the report; passing default CI output may omit values. Full local execution NOT RUN; exact-head CI pending. Follow-up: compare matched environments and actual capability outcomes, identify advisory impact only with authoritative distribution evidence. No dependencies, imports, spending, production behavior or release changes. FS-01B/REL-01 and license/production ABI gates remain blocked. Rollback: revert PR.


## UPSTREAM-MAINT-01 — strict report drift comparison

#287 merged at `e7a2fe10f569f30f8abbda1d8cac5de3993dc66c`, tested head `2ae345d63ff21b65607a35c2a33c75a264380db4`; 10/10 returned jobs PASS; GCC C17 Release 78/78 (job 140296816555), Debug 74/74 (job 140296816605), environment CTest passed. Standard matrix runner logs identify Ubuntu 26.04 and C17 Release compiler GCC 15.2.0. These are observed runner/compile identities, not complete six-field report values; dedicated Ubuntu 24.04 workflow and formal review not returned. #288 introduces [comparison script](../../scripts/tests/compare_upstream_environment.sh) for two complete raw six-field reports. It normalizes field order, validates unique known nonempty fields and a final completion marker, rejects control characters, and prints changed field names only; zero exit means comparison succeeded, not that environments match or are safe. CTest exercises actual report self-comparison, synthetic kernel/libc drift and reordering, plus six malformed candidate cases. Synthetic versions are fixtures, not observed host values. Local NOT RUN; exact-head CI pending. Usage: collect raw reports with report_upstream_environment.sh using actual configured tools, then run `sh scripts/tests/compare_upstream_environment.sh BASELINE_REPORT CANDIDATE_REPORT`. Do not supply timestamped CI logs as raw reports; compare matched source, flags, workloads and actual capability/regression tests before acting. No network, automatic update, import, dependency, production change, version-based security verdict or spend. Rust FFI, license/import, FS-01B/REL-01 and production ABI gates remain blocked. Rollback: revert PR.


## UPSTREAM-MAINT-01 — environment and runtime capability correlation

#288 merged at `c8ce7dd68fc0f20d092ac7151dd83f79584c0269`, tested head `c0017770fafc98a056d32e2a314c4afd7c602be6`; 10/10 returned jobs PASS, GCC C17 Release 79/79 (job 140297982361), Debug 75/75 (job 140297982363), strict environment comparison test passed. Dedicated Ubuntu 24.04 workflow and formal independent review not returned. #289 adds [correlated reporting](../../scripts/tests/report_upstream_capabilities.sh) in existing Linux CTest: collect the six-field environment identity, then directly execute existing disposable io_uring, copy_file_range, openat2, FCJ3 writer, and O_TMPFILE reference probes with bounded timeouts. Exit 0 = PASS; 77 = NOT_VERIFIED; absent optional compile-time targets = NOT_BUILT; missing configured executable, timeout, or other exit = FAIL and no completion marker. No nested CTest or replacement of existing tests. CTest success means the report completed without a failing probe, not that all five capabilities passed. Verbose output or LastTest.log is required for actual field values and individual probe results. No cross-host causal comparison, production filesystem guarantee, security verdict, network, automatic upstream update, imports, new external dependencies or spend. Full local execution NOT RUN; exact-head CI pending. Rust FFI, licensing, FS-01B/REL-01 and production ABI gates remain blocked. Rollback: revert PR.
