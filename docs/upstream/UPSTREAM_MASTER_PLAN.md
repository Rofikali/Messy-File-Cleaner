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
