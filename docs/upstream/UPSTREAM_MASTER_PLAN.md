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


## UPSTREAM-MAINT-01 — capability reporter controls

#289 merged at `c8967aafc7025cba75e34d9607a9c68a9757e2c4`, tested head `75f567b5f4fcfa70f56456792bb00c236995d817`; 10/10 returned jobs PASS; GCC C17 Release 80/80 (job 140299285186), Debug 76/76 (job 140299285087), five individual capability CTests and combined correlation passed in both. Combined report field values were not shown by default CI output. Dedicated Ubuntu 24.04 and formal independent review remain unverified. #290 adds a mixed PASS/NOT_VERIFIED/NOT_BUILT fixture and five rejection controls (unexpected exit, missing executable, relative path, required target sentinel, actual timeout), requiring diagnostic, absence of completion marker, and no subsequent probe execution. Only optional IO/path targets may use the NOT_BUILT sentinel. GNU timeout adds two-second kill-after escalation; the timeout control uses a disposable sleep process. No new dependencies, production changes, upstream updates or imports. Local NOT RUN; exact-head CI pending. No universal capability, vulnerability or filesystem safety verdict; Rust FFI and FS-01B/REL-01 remain blocked. Rollback: revert PR.


## UPSTREAM-MAINT-01 — final maintenance evidence and Ubuntu workflow repair

#290 merged at `dca2d627d531406800bfb0826a3985436dc8515b`, tested head `9be8b323faddc24aba08c163ae2f0f82e0c431ee`; 10/10 returned jobs PASS; GCC C17 Release 81/81 (job 140300636443), Debug 77/77 (job 140300636604), capability controls PASS (10.19 seconds in both). Formal independent review not returned. Investigation compared the Ubuntu workflow at #277 (blob 1ac76082420e319b28ab52ca71a18af228bfa02d) and #278 (blob 7b318853337469203dcf9f8352317869a46c644c, still on main): malformed static sanitizer command and duplicate production tail with invalid YAML indentation. This is a concrete defect and likely explanation for the missing Ubuntu workflow, not proof of the platform's exact rejection reason. #291 restores the intended static sanitizer test and single production tail, retains dependency tests and full suites, and adds verbose capability correlation in the Ubuntu host matrix, isolated Clang Release and hosted Clang Debug. Both shared consumer sanitizer tests rerun verbosely in hosted Debug. Structural comparison performed; full local YAML/shell execution and exact-head CI NOT RUN at preparation. No new workflow, runner, packages or production changes. This is the fifth opened PR; reconcile scoped DONE/PARTIAL/BLOCKED with actual job logs, no sixth PR.

### Upstream change decision procedure

Record exact source SHA, configured tools and actual environment, compare raw six-field reports, and compare observed capability outcomes on matched source/configuration/workload. A version difference triggers investigation, not an automatic code update. If all relevant tests pass, retain implementation unless an authoritative advisory or maintenance requirement indicates impact. If previously passing probes skip or disappear, treat as unverified regression; inspect host kernel, container permissions, headers and distribution backports. If a probe fails, reproduce in disposable fixtures, isolate environment versus source changes, preserve safe fallback and open a bounded reviewed fix only when needed. For advisories use official distribution/kernel/toolchain notices and affected package versions including backports; do not infer CVE status from upstream version alone. For future approved vendored components review pinned provenance, license, integrity, changes and rollback before update; registry remains locked/empty. No automatic upgrades or upstream merges, spending, production FFI, imports, or FS-01B/REL-01 gate changes. Rollback: revert candidate and retain the documented workflow defect as a blocker if needed.


## Maintenance reconciliation and measured TCO baseline

#291 merged at `f0f95f66bc9800ba4e9fea87ef9e512f84446681`, tested head `105c13e1ff521b859a47a3c361edc1506fce24b5`. 13/13 returned latest jobs PASS after APT HTTP 403 retries (Makefile 140303420197; Ubuntu C23 140303430639). Ubuntu GCC C11/C17/C23 Release 81/81; hosted Clang Debug 77/77 and static/shared sanitizer consumers PASS (job 140303431369), isolated Clang Release 80 pass/1 io_uring skip, correlation 4 pass/1 unverified versus hosted 5/5. Historical static/shared acceptance stays PARTIAL; later evidence does not rewrite historical exact-head checks. Formal independent review and TSan workflow not returned. UPSTREAM-MAINT-01 PARTIAL 5/5, no sixth PR. #292 begins distinct [measured cost baseline](UPSTREAM_TCO_EVIDENCE.md) with actual selected CI log spans, comparable-horizon TCO and explicit unknown billing/owner time; no ROI claim or automatic spending. Design only, CI pending; license/import, FFI, FS-01B/REL-01 and production ABI gates unchanged.


## UPSTREAM-COST-01 — checked integer comparison

#292 merged at `cbf8b068564cb493213cfa55fef8ac9a262b07c5`, tested head `29ac7703fb87cc720fd99e11d59beecc710074be`; 13/13 returned exact-head jobs PASS (CMake 45937918563, Ubuntu 45937918525, Docker 45937918686, Makefile 45937918541); formal independent review and TSan workflow not returned. #293 adds a [BUILD_TESTING-only reference](UPSTREAM_TCO_EVIDENCE.md), using unsigned 64-bit minor-unit amounts, three one-time and four constant monthly cost categories, matching three-uppercase-letter currency identifiers and horizons of 1–120 months, explicit known/unknown flags, checked addition and horizon multiplication, and unsigned difference plus direction. Reject unknown amounts, invalid metadata, null arguments and arithmetic overflow; do not publish a result on failure. The currency identifier is syntactic, not an ISO currency registry lookup; amounts must use the same minor-unit scale. Seventeen synthetic checks include equal/higher/lower options and error controls. Categorical values and horizons are illustrative, not project financial measurements; no labor-rate conversion, tax, discounting, inflation, varying monthly costs, billing ingestion, revenue or ROI model. Local NOT RUN; exact-head CI pending. No dependency, production linkage, import, spend or release; Rust FFI and FS-01B/REL-01 remain blocked. Rollback: revert PR.


## UPSTREAM-COST-01 — bounded horizon sensitivity

#293 merged at `f7ae637aea60eb831dede7fd89b0a809bc551d1e`, tested head `c22f4457091c655006d277814cf6b1490bb35220`; 14/14 returned jobs PASS (CMake 45938409448, Ubuntu 45938409320, Docker 45938409369, Makefile 45938409298, FCJ4 TSan 45938409307); GCC C17 Release 82/82 (job 140306836990), Debug 78/78 (job 140306836973), TCO reference CTest passed. Formal independent review not returned. #294 extends the [private reference](UPSTREAM_TCO_EVIDENCE.md) with a bounded 1–120 month sweep using the checked comparison per horizon, lower/equal/higher counts and first strictly lower horizon (zero when none). Output remains unchanged on failed evaluation, including later overflow. Twelve additional synthetic checks cover equal alternatives, crossover at month 12 with first strictly lower month 13, reversed comparison, truncated horizon, nonintegral crossover, immediate savings, late overflow, unknown and invalid arguments. Fixed monthly costs, no extrapolation, discounting, inflation, changing costs or financial recommendation. Existing 17 checks retained; local NOT RUN, exact-head CI pending. No actual charges, ROI, dependencies, imports, production changes or spend. Licensing, Rust FFI and FS-01B/REL-01 remain blocked. Rollback: revert PR.


## UPSTREAM-COST-01 — cash and noncash economic cost separation

#294 merged at `f77a47765a2907757aab74d209a5cb0a374e860f`, tested head `71000f1d27d8b7f62354ed62b5d112534ce3b260`; 13/13 returned jobs PASS (CMake 45939596896, Ubuntu 45939596771, Docker 45939596841, Makefile 45939596844); GCC C17 Release 82/82 (job 140310606069), Debug 78/78 (job 140310606118), existing TCO CTest with 29 synthetic checks passed. Formal independent review and TSan workflow not returned. #295 extends the [private reference](UPSTREAM_TCO_EVIDENCE.md) with separate cash and noncash owner-time option records, same currency and horizon, checked combined total and atomic failure. Cash can be evaluated independently even when noncash inputs remain unknown. Caller must allocate each economic amount once; the reference does not detect overlapping categories or convert hours to rates. Ten additional synthetic checks cover zero cash/nonzero noncash, independently known cash, unknown amounts, metadata mismatch, combined overflow and null arguments. Fixed monthly assumptions; not a financial accounting, billing or profitability system. Local NOT RUN; exact-head CI pending. No actual charges, ROI, dependencies, imports, production changes or spending. Licensing, Rust FFI and FS-01B/REL-01 remain blocked. Rollback: revert PR.
