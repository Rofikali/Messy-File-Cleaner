# Repository Engineering Rules

**Next selected research:** SPARSE-EXTENT-EVAL-01 (0/5), distinct test-only sparse extent evaluation using SEEK_DATA/SEEK_HOLE on disposable Linux files. Compare logical bytes, allocated blocks, extent boundaries, unsupported capability and error cases against buffered baseline; reflink is a conditional later experiment only if scoped evidence justifies it. No production copy/mover change. Governance PR #315 is not a feature attempt. Exact-head CI pending; FS-01B-FIX and REL-01 remain blocked.

**Owner direction (2026-11-28):** Continually identify Linux kernel and mature userspace techniques even when absent from the roadmap. Record real unmet requirement, falsifiable hypothesis, baseline, negative controls, capability/portability limits, licensing/provenance, maintenance/TCO, rejection and rollback criteria before selecting one bounded feature. Prefer native interfaces and independent designs; no blanket source-copy approval, no production safety gate bypass, no spend or release. See the Linux hypotheses and upstream reuse policy.

**Live reconciliation (2026-11-28):** PR #314 merged at `22e11eabf60a5abfbd8073acb25ba685b83fd4ac`; exact head `242a2d5dc3a712e7317fddea7e42df88395c4451`, five returned PR workflow families successful (CMake, Ubuntu, Docker, Makefile, FCJ4 TSan). Individual jobs, integrated-main and independent review not verified. WATCH-OVERFLOW-EVAL-01 PARTIAL 5/5; retain fail-closed watcher, no sixth PR. FS-01B-FIX and REL-01 remain blocked.

**Linux-native next research (2026-11-28):** #313 merged at `c3d0d9513a3dff49ecb03d26e6c14e44f3d81d8e`; exact-head 14/14 checks PASS, GCC C17 Release and Ubuntu C17 96 passed/1 skipped (97 registered), all four Inotify references PASS (not skipped); integrated-main 12/14 checks PASS, 2 pending at inspection; independent review unverified. WATCH-OVERFLOW-EVAL-01 (5/5; PR #314 open) adds deterministic close/register gap creation after actual overflow and requires rejection of old snapshot despite an empty new queue. Adoption decision: retain production fail-closed watcher; defer automatic recovery. No sixth PR, production recovery, dispatch, source import, spend, release or FS-01B closure. CI pending. [Hypotheses and evidence](docs/upstream/LINUX_NEXT_HYPOTHESES.md).

**Owner-directed release and distribution phase (2026-11-27):** [Release, packaging and distribution roadmap](docs/RELEASE_PACKAGING_DISTRIBUTION_ROADMAP.md). `go ahead` / `continue` / `next` / `next milestone` resumes the next eligible bounded job in the live board after GitHub reconciliation; same rules for hourly agents and engineers. CI-RECONCILIATION-01 is selected for the release-baseline incident; FS-01B-FIX remains the critical blocked safety prerequisite. LICENSE-01, PACKAGING-01, RELEASE-PIPELINE-01, GHCR-01, REL-01, PRODUCTION-SDK-01 and PRODUCT-ADOPTION-01 are gated future work, not implemented or release authorization. Preserve five-PR ceilings, no-spend, no external import, no automatic tag/publish, and the existing Rust FFI gate. Apply L1–L7 engineering, CISSP/security, CCIE/networking, Linux/POSIX, SRE/DevOps, CA/CFO, MBA/product and management review lenses; synchronize core and affected docs with evidence, not claims.

**Final upstream adoption decision:** [Evidence-first gate](docs/upstream/UPSTREAM_ADOPTION_DECISION.md). #295 merged; 13/13 returned jobs PASS including Ubuntu. #296 finalizes evidence-first upstream adoption and instruments private Debug TCO tests. CI pending; actual billing and ROI unknown; no sixth cost PR, imports, production changes or spend.

**Cash versus noncash TCO:** [Evidence and decision contract](docs/upstream/UPSTREAM_TCO_EVIDENCE.md). #294 merged; 13/13 returned jobs PASS including Ubuntu. #295 separates cash expenditure and noncash economic estimates with checked arithmetic, CI pending. No actual billing, production change, imports or spend.

**Bounded upstream TCO sensitivity:** [Evidence and decision contract](docs/upstream/UPSTREAM_TCO_EVIDENCE.md). #293 merged; 14/14 returned jobs PASS including Ubuntu and TSan. #294 adds bounded 1–120 month cost sensitivity with atomic error handling, CI pending. No actual billing, production change, import or spend.

**Checked upstream TCO reference:** [Evidence and decision contract](docs/upstream/UPSTREAM_TCO_EVIDENCE.md). #292 merged; 13/13 returned jobs PASS. #293 adds a test-only integer TCO reference with explicit unknown and overflow handling; CI pending. No production changes, spend, actual billing or ROI claim.

**Upstream maintenance cost evidence:** [TCO baseline](docs/upstream/UPSTREAM_TCO_EVIDENCE.md). #291 merged; 13/13 returned latest jobs PASS after retry. UPSTREAM-MAINT-01 PARTIAL 5/5; no sixth PR. #292 begins measured cost baseline and TCO contract; billing unknown, no imports, production FFI or spend.

**Final upstream maintenance evidence:** [Master plan](docs/upstream/UPSTREAM_MASTER_PLAN.md). #290 merged, 10/10 returned jobs and controls PASS. #291 repairs the Ubuntu workflow defect introduced in #278 and adds verbose capability evidence. CI pending; no sixth maintenance PR, production change, automatic update or new FFI.

**Upstream capability reporter controls:** [Master plan](docs/upstream/UPSTREAM_MASTER_PLAN.md). #289 merged; 10/10 returned jobs and capability correlation PASS. #290 adds mixed outcome and five rejection controls, required probe enforcement and bounded timeout escalation. CI pending; no production changes or new FFI.

**Upstream capability correlation:** [Master plan](docs/upstream/UPSTREAM_MASTER_PLAN.md). #288 merged; 10/10 returned jobs and comparison PASS. #289 adds environment identity plus actual runtime capability outcomes; CI pending. No automatic update, production change or new FFI.

**Upstream environment drift comparison:** [Master plan](docs/upstream/UPSTREAM_MASTER_PLAN.md). #287 merged; 10/10 returned jobs and environment report PASS. #288 adds strict environment drift comparison and malformed controls; CI pending. No automatic upstream update, production change or new FFI.

**Upstream maintenance environment evidence:** [Master plan](docs/upstream/UPSTREAM_MASTER_PLAN.md). #286 design merged, 9/9 returned jobs; FFI implementation remains BLOCKED at 1/5 until explicit owner scope approval and staged review. #287 begins independent upstream maintenance reference work, read-only environment report; CI pending. No production dependency or import.

**Private Rust FFI feasibility proposal:** [Design and approval gate](docs/architecture/RUST_CLASSIFIER_FFI_FEASIBILITY.md). #285 merged, 10/10 returned jobs and artifact isolation PASS; Ubuntu/Clang final evidence and independent review unverified. SHARED-LIB-01 PARTIAL 5/5. #286 proposes a private classifier FFI consumer; implementation requires explicit scope approval and staged ADR review. Existing FCJ4 crate unchanged; no new FFI or production SDK.

**Final private shared-library acceptance:** [Linux-inspired architecture](docs/architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #284 merged, 10/10 returned jobs, instrumented consumers PASS; Ubuntu workflow, Clang sanitizer execution and formal review unverified. #285 final private shared artifact candidate, CI pending; no sixth PR or production SDK/ABI/FFI approval.

**Private shared sanitizer and ownership:** [Linux-inspired architecture](docs/architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #283 merged, 10/10 returned jobs and runtime/ELF hardening PASS. #284 adds Debug DSO and consumer sanitizers, mutable caller-owned input; CI pending, Clang Debug sanitizer-specific linker exception only. No production SDK or ABI/FFI approval.

**Private shared runtime and hardening:** [Linux-inspired architecture](docs/architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #282 merged, 10/10 returned jobs and ELF metadata controls PASS; #283 adds explicit loader cycles, hidden lookup rejection and private DSO hardening. CI pending; no production SDK or ABI/FFI approval.

**Private shared ELF boundary:** [Linux-inspired architecture](docs/architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #281 merged, 10/10 returned jobs and eight consumer scenarios PASS; #282 adds private SONAME, relative RUNPATH, dynamic export and dependency metadata gates. CI pending; no production SDK or ABI/FFI approval.

**Private shared library reference:** [Linux-inspired architecture](docs/architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). STATIC-LIB-01 PARTIAL 5/5, no sixth PR; returned final artifact tests PASS, Ubuntu workflow and formal review unverified. #281 starts private PIC shared reference with hidden default visibility and eight consumer scenarios; CI pending, no production SDK or ABI/FFI approval.

**Final private static archive acceptance:** [Linux-inspired architecture](docs/architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #279 merged, 10/10 returned jobs, C17 Release 72/72 and Debug 71/71; Ubuntu workflow unverified. #280 is the final private archive artifact acceptance candidate; no sixth PR or production library/ABI/FFI approval. CI pending.

**Static archive matrix and ownership:** [Linux-inspired architecture](docs/architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #278 merged; 10/10 returned jobs and targeted exact-blob local dependency/sanitizer checks PASS; Ubuntu-specific CI unverified. #279 extends Release CTest coverage and caller-owned storage checks; CI pending, no production SDK or ABI/FFI approval.

**Static archive dependency and sanitizer boundary:** [Linux-inspired architecture](docs/architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #277 merged, 13/13 returned jobs and five archive definitions plus two negative controls PASS. #278 tests per-member dependency direction and Debug sanitizer-instrumented consumer; CI pending. No production ABI/FFI or library approval.

**Static archive symbol boundary:** [Linux-inspired architecture](docs/architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). #276 merged, 14/14 returned jobs and same-environment archive repeatability PASS. #277 enforces five global definitions with extra/missing controls; CI pending, no production ABI/FFI or library approval.

**Owner-approved Linux-inspired library architecture (2026-11-27):** [Engineering requirements](docs/architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). All agents must enforce logical subsystem boundaries, strict warnings, archive reproducibility evidence, namespace/visibility, ownership, portable fallbacks, license and measured performance gates. Do not create unnecessary physical archives or import kernel internals.

**Private static archive reference:** [API roadmap](docs/architecture/C-API-ABI-FFI-ROADMAP.md). PUBLIC-API-01 scoped DONE 5/5; #275 13/13 returned exact-head jobs and three artifact isolation checks plus six negative controls PASS. #276 starts STATIC-LIB-01 with a BUILD_TESTING-only classifier/hashmap archive and separate internal consumer, not a production SDK; CI pending. No sixth API PR or production ABI/FFI approval.

**Public API final isolation candidate:** [API roadmap](docs/architecture/C-API-ABI-FFI-ROADMAP.md). #274 merged, 13/13 returned jobs, 140 synthetic calls, 112 matched allocations/releases and hosted ASan/UBSan leak checks PASS. #275 final scoped candidate checks production symbol/install isolation; CI pending, no sixth PR or production ABI/FFI approval.

**Public API synthetic ownership:** [API roadmap](docs/architecture/C-API-ABI-FFI-ROADMAP.md). #273 merged, 13/13 returned PR-head jobs and 60 synthetic scenario executions PASS. #274 models heap ownership, matching release and positive retries in test-local mock only, plus hosted ASan/UBSan leak checks; CI pending. No production ABI/FFI approval.

**Public API mock caller:** [API roadmap](docs/architecture/C-API-ABI-FFI-ROADMAP.md). #272 merged, 13/13 returned PR-head jobs PASS. #273 tests ten caller scenarios against a test-local synthetic stub only, not the real engine. Six compiler-mode executions pending; no production ABI/FFI approval.

**Public API consumer boundary:** [API roadmap](docs/architecture/C-API-ABI-FFI-ROADMAP.md). #271 merged, 13/13 returned PR-head jobs PASS. #272 models the existing batch-preview CLI as a proposed internal consumer: only a complete compatible result is published, acquired handle cleanup after preview, no actual runtime or production ABI/FFI approval.

**Public C API design candidate:** [API/ABI roadmap](docs/architecture/C-API-ABI-FFI-ROADMAP.md) starts PUBLIC-API-01 as design/compile-only preview consumer evidence, not production API, library or FFI approval. UPSTREAM-COMPAT-01 scoped DONE 5/5; #270 reports container 4 passed/1 skipped and host 5 passed, 324 header probes. Existing global lifecycle and approval gates remain.

**Scoped compiler/platform acceptance:** [compatibility evidence](docs/upstream/UPSTREAM_COMPAT_REFERENCE.md) records #269 merged and 324/324 internal-header checks PASS. Final candidate explicitly reports five existing capability-sensitive tests separately in container Release and hosted Debug, distinguishing PASS from NOT_VERIFIED on skip; no API/ABI claim or new dependencies.

**Internal header compatibility:** [compatibility evidence](docs/upstream/UPSTREAM_COMPAT_REFERENCE.md) records #268 merged, hosted Clang sanitizer preflight and Debug 70/70 PASS; fourth candidate checks each internal header independently and twice under GCC/Clang C11/C17/C2x, normal and test macros. No public ABI promise or new dependency.

**Same-PR Clang Debug correction:** Ubuntu run 45824599054/job 139925552427 failed at link because the development image lacks Clang 18 ASan runtime archives. [Compatibility evidence](docs/upstream/UPSTREAM_COMPAT_REFERENCE.md) moves only Debug to the existing non-root hosted Ubuntu runner with preinstalled clang-18, explicit ASan/UBSan link/run probe, disposable tracked source archive and build directory; no package added, no sanitizer disabled, container Release isolation unchanged. Host runtime availability and exact-head CI pending; host step is not container-isolated.

**Compiler compatibility:** [evidence](docs/upstream/UPSTREAM_COMPAT_REFERENCE.md) records #267 merged, full Clang C17 Release CTest 69 passed, one io_uring capability skip, zero failures; third candidate runs full Clang Debug suite with existing ASan/UBSan target instrumentation and halt-on-error, same isolation. No new packages or runtime/API changes.

**Upstream compatibility:** [compiler evidence](docs/upstream/UPSTREAM_COMPAT_REFERENCE.md) records merged #266: Clang production C11/C17/C23 and CLI controls PASS; second candidate runs the full existing C17 Release suite with Clang in the same isolated development image. No new dependencies or runtime/API change.

**Upstream compatibility reference:** [compiler compatibility](docs/upstream/UPSTREAM_COMPAT_REFERENCE.md) starts UPSTREAM-COMPAT-01 with Clang production C11/C17/C23 compile/link and existing CLI help acceptance inside the already built Ubuntu development image. SUPPLY-CHAIN-01 scoped DONE 5/5; no new packages, runner, production source or public ABI changes.

**Final supply-chain acceptance candidate:** [supply-chain reference](docs/security/SUPPLY_CHAIN_REFERENCE.md) requires literal direct action owner/repo@40 lowercase hex SHA independently of inventory equality, with 11 isolated fixture cases. #264 merged, 15/15 returned jobs, 70/70 C17 Debug/Release, 35 references and seven APT cases PASS; final candidate CI pending. No new dependencies or production source changes.

**Immutable action references:** [supply-chain reference](docs/security/SUPPLY_CHAIN_REFERENCE.md) pins all 18 direct action declarations across nine workflows to the full commits resolved from the existing official upstream major tags. #263 merged, 13/13 returned exact-head jobs, 70/70 C17 Debug/Release, 35 inventory references and seven APT cases PASS. Existing runtime, runner and package set unchanged; pin candidate CI pending.

**Build-input inventory:** [supply-chain reference](docs/security/SUPPLY_CHAIN_REFERENCE.md) documents mutable direct action, runner, Python and Docker base references; CI checks the tracked textual inventory. #262 merged, 13/13 returned jobs and seven APT retry cases PASS. New inventory gate CI pending; no version pinning or runtime change.

**APT retry acceptance:** [supply-chain reference](docs/security/SUPPLY_CHAIN_REFERENCE.md) adds a deterministic test of the exact Dockerfile RUN body with isolated command stubs, seven cases including exhausted retries and failed installation/cleanup. #261 merged, 13/13 returned exact-head jobs and 70/70 C17 Debug/Release PASS; new CI pending.

**Supply-chain reference:** [build integrity](docs/security/SUPPLY_CHAIN_REFERENCE.md) begins SUPPLY-CHAIN-01 with a bounded fail-closed authenticated APT refresh in the existing production-image build after #260's observed mirror inconsistency. UPSTREAM-FAULT-01 scoped DONE 5/5; no runtime binary or dependency-set change.

**Final configuration fault acceptance:** [fault reference](docs/security/UPSTREAM_FAULT_REFERENCE.md) adds 12 simultaneous read/close fault combinations with all acquisition/allocation adapters armed, full rollback, descriptor closure, matching staged free and clean retries. #259 merged, 13 completed jobs PASS, Makefile pending at inspection; #258 Ubuntu image still pending; 70/70 C17 Debug/Release and 24 fault cases PASS. Final candidate CI pending; no production change.

**Configuration acquisition fault reference:** [fault acceptance](docs/security/UPSTREAM_FAULT_REFERENCE.md) adds isolated open and fdopen failure controls, exact staged allocation/free, raw descriptor closure and retry. #258 merged, 13 completed jobs PASS, Ubuntu image pending at inspection; 70/70 C17 Debug/Release and three allocation faults PASS. Candidate CI pending; no production change.

**Configuration allocation fault reference:** [fault acceptance](docs/security/UPSTREAM_FAULT_REFERENCE.md) adds isolated staged malloc ENOMEM at capacities 2/8/64, no stream acquisition, complete rollback and successful retry with one matching staged free. #257 13/13 returned jobs, 70/70 C17 Debug/Release and 3 close faults PASS; candidate CI pending. No production change.

**Configuration close fault reference:** [fault acceptance](docs/security/UPSTREAM_FAULT_REFERENCE.md) adds EIO/ENOSPC/EINTR stream-close reports after successful EOF, full rollback and same-adapter positive commit controls. #256 14/14 jobs, C17 Debug/Release 70/70 and 12 read fault cases PASS; new candidate CI pending. No production change.

**Upstream fault reference:** [fault acceptance](docs/security/UPSTREAM_FAULT_REFERENCE.md) begins UPSTREAM-FAULT-01 with a test-only GNU stdio adapter for deterministic post-open config stream faults, full caller-state rollback, descriptor close and recovery. Linux selftest scoped DONE 5/5. No production change.

**Linux selftest final leak oracle:** [scanner security](docs/security/FILESYSTEM-SCANNER-SECURITY.md) adds intentional deepest-directory inheritance solely in forked test children and exact positive/negative oracle controls. #254 13/13 returned jobs, 69/69 C17 Debug/Release and deep identity checks PASS; final candidate CI pending. No production change.

**Linux deep descriptor reference:** [scanner security](docs/security/FILESYSTEM-SCANNER-SECURITY.md) adds a private 20-directory chain across the initial 16-frame capacity, exact live inode CLOEXEC/exec checks, cancellation and successful rescan. #253 merged, 12 completed jobs PASS, Ubuntu image pending at inspection; candidate CI pending. No production change.

**Linux scanner abort lifecycle reference:** [scanner security](docs/security/FILESYSTEM-SCANNER-SECURITY.md) adds callback EACCES/EIO and cooperative cancellation after actual child exec probes, inode-specific descriptor restoration and immediate successful rescans. #252 13/13 returned jobs and 69/69 C17 Debug/Release PASS; candidate CI pending. No production change.

**Linux scanner exec inheritance reference:** [scanner security](docs/security/FILESYSTEM-SCANNER-SECURITY.md) adds real fork/exec of the same test binary during each private scan callback, with negative and intentional positive inheritance controls, parent restoration and preserved bytes. #251 14/14 jobs and C17 Debug/Release 69/69 passed; new candidate CI pending. No production change.

**Linux selftest reference:** [scanner descriptor security](docs/security/FILESYSTEM-SCANNER-SECURITY.md) begins LINUX-SELFTEST-01 with actual scanner FD_CLOEXEC checks on private directory inodes during callbacks, descriptor restoration and preserved file bytes. CPU research scoped complete 5/5; retain production classifier. No production change.

**CPU guard-page final reference:** [baseline](docs/performance/CPU_OPT_REFERENCE.md) adds Linux inaccessible pages around read-only input, both edges, all eight entry modes and 82560 exact comparisons. #249 dispatch checks passed; final candidate CI pending. Retain production classifier; no universal speedup or production ISA change.

**CPU dispatch policy reference:** [baseline](docs/performance/CPU_OPT_REFERENCE.md) adds one-time capability-intersected selection, forced portable execution, four allowed masks and an independent policy truth table. Native selection remains test-only; no production change.

**CPU explicit vector reference:** [baseline](docs/performance/CPU_OPT_REFERENCE.md) adds guarded test-only SSE2/AVX2 byte counts, bounded vector loads and tails, generic x86-64 baseline and capability-tagged comparisons. No production change.

**CPU timing CI correction:** PR #247's earlier workflow had two split CTest selectors; restore exact single-line anchored selectors without weakening the matrix. Ubuntu C17 on the earlier head passed 68/68 and GCC reported 16-byte vectorization of the portable loop. Corrected-head CI pending.

**CPU timing reference:** [baseline](docs/performance/CPU_OPT_REFERENCE.md) adds 54 bounded timing rows across three lengths and match densities, retains 165120 correctness comparisons, and requests GNU vectorization diagnostics plus C17 Release disassembly. No production change.

**CPU optimization reference:** [baseline](docs/performance/CPU_OPT_REFERENCE.md) begins CPU-OPT-EVAL-01 with bounded scalar/libc byte-count equivalence and direct Debug sanitizers; cache-layout scoped evaluation closed 5/5, retain production layout. No production change.

**Cache-layout final evaluation:** [evidence](docs/performance/CACHE_LAYOUT_REFERENCE.md) records #244 exact-head 14/14 jobs and C17 Release 67/67 tests, including 256 immutable reader digests and oversized extent rejection. Final candidate instruments the reference target itself with fatal ASan/UBSan in Debug; CI pending. Retain production layout; no production speedup claim.

**Cache-layout capacity and readers reference:** [evidence](docs/performance/CACHE_LAYOUT_REFERENCE.md) compares logical metadata against trusted owner allocation extents and tests four immutable readers (64 digests each), joined before cleanup. Experimental only; no production change.

**Cache-layout ownership reference:** [evidence](docs/performance/CACHE_LAYOUT_REFERENCE.md) adds bounded packed snapshot construction, allocation-failure injection, independent lifetime and metadata corruption rejection. No production change.

**Cache-layout timing reference:** [evidence](docs/performance/CACHE_LAYOUT_REFERENCE.md) adds 36 bounded construction/traversal measurements with independently owned snapshots and verified full-path digests. No production change.

**Cache-layout evaluation:** [reference](docs/performance/CACHE_LAYOUT_REFERENCE.md) compares fixed FileEvent arrays with an independent packed path/index model; prior queue padding remains inconclusive, batch I/O closed 3/5, no production change.

**Batch I/O failure reference:** [evidence](docs/performance/BATCH_IO_REFERENCE.md) adds deterministic bounded short writes, injected EINTR/zero/EIO and exact prefix preservation; timing advantage versus scalar128k is small, retain production mover.

**Batch I/O timing reference:** [evidence](docs/performance/BATCH_IO_REFERENCE.md) adds 18 bounded wall/CPU timing samples with rotated method order; prior syscall counts show no advantage over scalar 128 KiB. No production change.

**Batch I/O evaluation:** [reference](docs/performance/BATCH_IO_REFERENCE.md) compares scalar 32/128 KiB and four-segment vectored transfers on private files. Scoped raw io_uring evaluation closed at 4/5; no liburing dependency or production change.

**Linux async I/O synchronization/error reference:** [io_uring evidence](docs/performance/LINUX_URING_REFERENCE.md) adds actual FSYNC, invalid descriptor, EOF and recovery completions after successful writes. Test-only private files; no production change.

**Linux async I/O vectored reference:** [io_uring evidence](docs/performance/LINUX_URING_REFERENCE.md) adds four READV and four WRITEV operations on anonymous private fixture files, with exact bytes and completion validation. Kernel version evidence recorded; no production changes.

**Linux async I/O NOP reference:** [io_uring reference](docs/performance/LINUX_URING_REFERENCE.md) adds bounded actual submission/completion validation with shared ring mappings after successful capability discovery. No file I/O, liburing dependency or production change.

**Linux async I/O capability:** [io_uring reference](docs/performance/LINUX_URING_REFERENCE.md) starts the LIBURING-EVAL-01 reference-only kernel probe; Linux I/O scoped evaluation closed at 3/5, retain buffered mover. No liburing dependency or production changes.

**Linux I/O offset reference:** [baseline](docs/performance/LINUX_IO_REFERENCE.md) adds explicit offset, short EOF transfer and untouched destination byte comparisons. Buffered transfer retained; no production change.

**Linux I/O timing reference:** [baseline](docs/performance/LINUX_IO_REFERENCE.md) adds six bounded transfer pairs with alternating order and verbose C17 Release evidence. Independent content-copy models only; production unchanged.

**Linux I/O evaluation:** [copy reference](docs/performance/LINUX_IO_REFERENCE.md) compares independent buffered content transfer with copy_file_range on private fixture files. Watcher evaluation closed at 3/5; no production mover change.

**Watcher lifecycle reference:** [kernel reference](docs/performance/LINUX_WATCH_REFERENCE.md) adds repeated actual watcher cancellation, root deletion, failed registration and descriptor-count restoration checks. No production change.

**Watcher batch reference:** [kernel reference](docs/performance/LINUX_WATCH_REFERENCE.md) adds real buffered inotify create/move-in events, exact raw record membership, production parser handoff and dry-run worker counters with unchanged file bytes. No production change.

**Linux watcher evaluation:** [kernel reference](docs/performance/LINUX_WATCH_REFERENCE.md) compares production terminal invalidation with a separate raw inotify watch on a private rename/replacement fixture. Path evaluation closed at 3/5; production unchanged.

**Linux path error evaluation:** [path reference](docs/performance/LINUX_PATH_REFERENCE.md) adds real invalid descriptor, path, request-size and resolution-option checks. No unrestricted fallback; no production adapter change.

**Linux path identity evaluation:** [path reference](docs/performance/LINUX_PATH_REFERENCE.md) adds a deterministic private-fixture rename and replacement comparison of held directory descriptors versus reopened names. No production trust change.

**Linux path evaluation:** [path resolution baseline](docs/performance/LINUX_PATH_REFERENCE.md) characterizes openat2 confinement on disposable directories only; conditional Linux UAPI test, no production fallback or trust change. Scanner scoped evaluation closed at 4/5; retain existing scanner.

**Scanner CI evidence:** [reference baseline](docs/performance/SCANNER_REFERENCE_BASELINE.md) adds verbose reporting of the existing bounded scanner reference smoke in the C17 Release CI leg only, with compiler/kernel/libc context. No production or scale-benchmark change.

**Scanner early-stop evidence:** [reference baseline](docs/performance/SCANNER_REFERENCE_BASELINE.md) adds callback error, nftw callback stop and native cooperative cancellation at first/middle/final regular-file callbacks on private fixtures, verifies exact partial counts, descriptor restoration and complete rescans. No production change.

**Scanner descriptor evidence:** [reference baseline](docs/performance/SCANNER_REFERENCE_BASELINE.md) adds untimed Linux procfs callback-visible descriptor observations for native scanner and nftw budgets 1/4/16, exact-set and baseline restoration checks. No production backend change.

**Linux scanner evaluation:** [scanner reference baseline](docs/performance/SCANNER_REFERENCE_BASELINE.md) compares actual read-only scanner with POSIX nftw FTW_PHYS on small private wide/deep fixtures; exact file set and no symlink callbacks required. String evaluation closed at 3/5; retain production classifier/scanner.

**String measurement evidence:** [extension baseline](docs/performance/STRING_EXTENSION_BASELINE.md) records standalone exact-blob C11/C17/C23 and ASan/UBSan execution; reverse candidate improved long synthetic prefixes but representative workload and target-host evidence remain absent. Retain production libc classifier.

**String boundary evaluation:** [extension baseline](docs/performance/STRING_EXTENSION_BASELINE.md) adds exhaustive short byte strings, explicit extension-length boundaries and an independent forward routing reference. Verify both classifier implementations and borrowed result identity; no production change.

**String-processing evaluation:** [extension baseline](docs/performance/STRING_EXTENSION_BASELINE.md) compares actual libc-based classifier with test-only reverse basename/extension scan. Preserve exact classification semantics and production code. Memory evaluation closed at 4/5; retain existing allocator.

**Memory unwind evaluation:** [path allocation baseline](docs/performance/PATH_ALLOCATION_BASELINE.md) adds deterministic interruption at each nested frame depth, real bounded arena exhaustion and modeled owned-join failure. Verify ancestor preservation, balanced releases and reuse; not actual scanner cancellation or system malloc-failure acceptance.

**Nested path lifetime evaluation:** [path allocation baseline](docs/performance/PATH_ALLOCATION_BASELINE.md) adds variable-length synthetic directory frames and transient siblings to the existing owned/arena comparison. Verify ancestor preservation and strict LIFO rewind; no production lifetime or allocator change.

**Bounded arena comparison:** [path allocation baseline](docs/performance/PATH_ALLOCATION_BASELINE.md) compares the actual owned helper with an isolated test-only batch-reset byte arena. Check exact output, bounds, independent live copies, allocation/free counts and reset lifetime. Retain production allocator and ownership; no speedup claim.

**Memory allocation evaluation:** [path allocation baseline](docs/performance/PATH_ALLOCATION_BASELINE.md) measures the actual owned lexical path helper with bounded simultaneous copies and exact output/independence checks. Hash-map scoped evaluation closed at 4/5; retain production map and allocator. No arena, new dependency, ABI or filesystem change.

**Hash-map collision evaluation:** [baseline](docs/performance/HASHMAP_BASELINE.md) discovers fixed colliding keys through the actual map, checks slot-63 wraparound, full-table rejection, existing-key updates and exact hit/miss results across 64/128/256 slots. Test-only; retain production capacity, no speedup or hostile-key resistance claim.

**Classifier workload evaluation:** [hash-map baseline](docs/performance/HASHMAP_BASELINE.md) compares actual classifier plus map across 64/128/256 logical slots using fixed synthetic mixed POSIX paths. Verify mapped, absent, extensionless, hidden, trailing-dot and multi-extension cases; no production capacity or ABI change, no speedup claim.

**Hash-map capacity comparison:** [lookup evidence](docs/performance/HASHMAP_BASELINE.md) compares 64/128/256 slots with identical fixed key sets: four shipped default mappings and a 64-key synthetic stress set. Record longest occupied cluster, logical backing bytes and verified lookup timings. No production capacity, hash, ownership or ABI change.

**Hash-map evaluation:** [bounded lookup baseline](docs/performance/HASHMAP_BASELINE.md) measures verified hits and misses at 25/50/75/100% synthetic occupancy. Preserve caller-owned storage, bounded probing, full-map updates and production behavior. Queue evaluation closed at four PRs; no external imports or performance claims without evidence.

**Queue affinity evaluation:** [queue baseline](docs/performance/QUEUE_BASELINE.md) adds optional test-process-only pinning of producer and consumer to two distinct allowed Linux logical CPUs, verifies resulting masks, rejects invalid requests, and leaves normal CI unpinned. Same feature budget; no production layout or memory-order change.

**Queue cache-layout experiment:** [queue evaluation](docs/performance/QUEUE_BASELINE.md) compares the existing ring with a test-only 64-byte-aligned index candidate compiled from the same source and workload. Report layout, memory cost, verified transfers and variability; retain production C atomics/layout until representative comparative evidence justifies a change.

**Concurrent queue evidence:** [queue baseline](docs/performance/QUEUE_BASELINE.md) adds a distinct two-thread, fixed-workload measurement with unique sequence and full payload verification, cooperative retry deadline and CTest timeout. Same QUEUE-UPSTREAM-EVAL-01 budget; no production layout, synchronization or performance claim change.

**Queue performance evidence:** [bounded in-memory baseline](docs/performance/QUEUE_BASELINE.md) measures the existing C atomic SPSC queue in three fixed-size batched samples, verifying payloads and FIFO ordering. Test-only C target, no production code/layout change, no time threshold or speedup claim. Linux circular-buffer reference informs evaluation; retain modulo and C atomics pending comparative evidence.

**Scanner special-file regression:** Linux inode-type reference confirms regular files, FIFOs and sockets are distinct. Existing descriptor-relative regular-file-only selection is retained; disposable FIFO and pathname Unix-socket fixtures exercise no callback handoff, repeated scans, file preservation and descriptor balance. No imported implementation, production change or performance claim. See [scanner security](docs/security/FILESYSTEM-SCANNER-SECURITY.md).

**Upstream audit:** [Reference-only module assessment](docs/upstream/UPSTREAM_MODULE_AUDIT.md) records existing native interfaces, candidate tradeoffs and deferred work. Retain current C atomic SPSC queue; full-ring recycling regression verifies rejection at capacity, FIFO ordering and complete event payload across wraparound. No imported implementation, performance claim or license clearance.

**Owner-approved upstream-first policy (2026-11-25):** Follow [reuse policy](docs/upstream/UPSTREAM_REUSE_POLICY.md) and [master plan](docs/upstream/UPSTREAM_MASTER_PLAN.md). Prefer supported Linux/POSIX APIs and mature compatible components; retain working code unless comparative evidence justifies change. External source imports and new third-party dependencies are BLOCKED pending explicit owner license decision and component acceptance. C17, C11/C23 CI, FS-01B/REL-01, no-spend and five-PR constraints remain binding.

> **Configuration input contract:** Physical config lines must reject embedded NUL bytes before string parsing; failed loads preserve caller state; loader requires a nonempty path and initialized power-of-two map capacity with size not exceeding capacity; extension names reject control bytes; rejected extension keys and destinations must not be echoed into diagnostics. Hash-map initialization and access reject impossible byte extent or size metadata; callers still own backing storage. Classification uses the last POSIX path component; leading-dot-only and trailing-dot names use the fallback. SPSC queue initialization is exclusive; its unit suite checks concurrent FIFO transfer across repeated wraparound. Worker move diagnostics escape nonprintable/non-ASCII bytes and backslashes under a stream lock; original filesystem paths are unchanged. Root/output validation reports fixed rejection reasons without echoing untrusted paths. Linux watch registration requires a directory and rejects final-component symlinks; ancestor path identity is not established. Shared dispatch rejects empty paths and worker counts outside 1..MAX_WORKERS. Watcher event headers are copied before decoding; actionable names must be nonempty basename components. Worker notification and shutdown reject out-of-range pool metadata before indexing fixed arrays. The process-wide worker storage has one owner; duplicate starts and wrong-context lifecycle operations are rejected under caller serialization. Watcher idle-poll metrics count actual poll timeouts; integration waits for observed readiness instead of a fixed startup sleep. Scanner cancellation is checked before root access and immediately before callback handoff; it remains cooperative. Scanner errors retain the first observed failure even when later cancellation or callback failure stops traversal. Linux scanner strips trailing root separators before no-follow opening, preserving the root slash; ancestor identity is not established. Linux watcher also trims trailing root separators before no-follow registration and builds event paths from the trimmed root. Scanner and watcher share the internal root-path copy helper; it trims only trailing slashes and performs no filesystem resolution. CLI root preflight uses the same lexical root-copy helper before its existing lstat/realpath checks. CLI output preflight, scanner and watcher use a shared checked lexical path-join helper; callers retain component validation and filesystem policy. Scanner path construction and frame growth failures stop discovery and unwind owned frames; deep-tree allocation fault regressions check descriptor balance. Scanner directory acquisition stops on EMFILE, ENFILE or ENOMEM and unwinds owned frames; deterministic child-open fault tests check attempts and descriptor balance. Scanner resource regressions cover directory-stream acquisition faults and a real child-process RLIMIT_NOFILE exhaustion test without changing the parent or system limit. BENCH-SCALE-01 adds a bounded native C scanner benchmark with callback counts, monotonic timing, CPU and process peak RSS; smoke checks do not establish large-scale capacity. Native scanner evidence has a bounded manual-only C17 Release workflow with source/binary and runner provenance; no automatic scale run or spending authorization. BENCH-SCALE-01 final budget slot adds semantic evidence validation before upload; no performance result or release acceptance is inferred. Malformed bare recognized configuration settings use the same C whitespace classification as key normalization, including vertical tab and form feed; rejection preserves caller configuration. Hash-map existing-key updates accept overlapping borrowed value strings, including a suffix, without undefined memcpy overlap. Classifier initialization ignores a NULL context; classification rejects NULL contexts and NULL/empty filenames without changing valid-path routing. SPSC queue APIs reject NULL queue/event arguments without transferring events; NULL initialization is a no-op and NULL emptiness queries return true. Valid initialized SPSC ordering and capacity are unchanged. Workers reject empty or unterminated event paths and NULL/empty classification results before diagnostics or moving; invalid work increments failures and stops intake with worker_error while existing queued work drains. Scanner entry and opened-directory metadata resource exhaustion terminates discovery and unwinds owned frames; entry and opened-directory lookup fault regressions verify traversal termination and descriptor balance, including nested failure and successful rescan. Hash-map insertion refuses an unused slot when the declared size is already at capacity; existing-key updates remain allowed at full capacity. This guard does not prove arbitrary metadata consistency. Configuration initialization treats NULL as a no-op and resets scalar defaults without changing caller-owned extension mappings; regression covers populated-map preservation. Watcher cancellation is checked between buffered records and before actionable path construction; a pending stop prevents new record handoff at those boundaries but cannot revoke in-flight work. Watcher terminal-record regression covers overflow and all four invalidation flags followed by an actionable record; the parser returns the terminal error without handing off subsequent records. CLI --workers is a unique value-bearing override, like --root and --config; repeating it (including identical counts) is rejected before configuration or filesystem access. Scanner directory read resource exhaustion terminates discovery and unwinds all owned frames; depth-targeted test-only faults verify termination, descriptor balance and successful rescan. Ordinary EOF and non-resource errors retain existing behavior. Worker pool lifecycle independently tracks the actual started count; in-range caller count mismatches reject notify/join before mutation under the existing serialized ownership contract. Worker shutdown regression injects a notification failure inside join, checks error propagation and complete metadata cleanup, then verifies restart. Configuration loading opens with nonblocking and close-on-exec flags, validates the opened object is a regular file before parsing, and preserves symlink-to-regular-file compatibility. FIFO and directory inputs fail; arbitrary devices or remote filesystem opens are not proven nonblocking. See [live board](next.md) for implementation and verification status. FS-01B/REL-01 remain blocked.

> **Current owner contract (24 Nov 2026):** Every authorized contributor (human, ChatGPT, Codex, Claude Code or other agent) uses the same resume-first [handoff contract](docs/AUTOPILOT_EXECUTION_CONTINUATION.md), [live board](next.md) and [feature ledger](docs/feature-budget.json). Hourly and manual `go ahead` are additive. Reconcile all authors' PRs, finish existing eligible work before starting another, count all opened PRs toward one stable five-PR budget, and update affected docs in the same PR. Actual GitHub state supersedes historical milestones below. No fake approval, security bypass or release authority.

> **Execution recovery (23 Nov 2026):** use the [hourly continuation and stall recovery](docs/AUTOPILOT_EXECUTION_CONTINUATION.md) for checkpoints, resume-first task selection, two-no-change escalation, and proportional L1–L7/CISSP/CCIE/Linux/SRE/CA-MBA review. This is a repository procedure, **not a running scheduler**; current GitHub and actual ChatGPT task history supersede dated snapshots.

> Owner directive (23 Nov 2026): **no required human approving review** for routine eligible autopilot PRs. Conduct documented agent self-review; enforce current-head CI, thread resolution, main rules and FS-01B P0 boundaries. [Authoritative policy](docs/AUTOPILOT_ZERO_REVIEW_AND_DOC_SYNC.md) replaces contrary older independent-approval requirements; [ruleset JSON](.github/rulesets/autopilot-main-no-human-review.json) must be imported separately.

This file is the working contract for every human and automated contributor.

## Owner feature-budget directive (2026-11-21)

Follow [five-PR feature policy](docs/FEATURE_DELIVERY_AND_ACCEPTANCE_POLICY.md), [machine-checked ledger](docs/feature-budget.json), and the compact [NOW board](next.md). **Maximum five OPENED feature PR attempts per stable feature ID**, counting rejected/closed/merged attempts; revisions to a single PR do not consume another slot. Exit earlier when verified, or after five document DONE/PARTIAL/BLOCKED and outstanding safety/manual gaps and advance to a different independent feature. **PERF-01 is CLOSED_LEGACY through merged #128**; no sixth/unlimited PERF-01 fixture-hardening spin-offs. **PLAN-REPORT-01 PARTIAL:** PR #145 merged at `ac9fc87a91a665e2d0f09a3af79ee7f7ca310139` with aggregate-only JSON, not a per-file plan/conflict inventory. **CONFIG-MODE-01 DONE:** PR #146 merged at `d7adc17afa4c015f95ad016bb3817c133a682435` with four exact-head workflow families PASS. **CONFIG-NUMERIC-01 DONE:** PR #147 merged at `d839ad608c554d0c7715dc6264be6952ef07eccf` (four exact-head workflow families PASS). **CONFIG-LINE-01 DONE:** PR #148 merged at `3b32efbe8fad2047f3588fa51c907525a4e4dca4` (four PR workflow families PASS). **CONFIG-EXT-TRAIL-01 DONE:** #150 merged at `74c0639e9656eb37339e96cdc9ed7d312682b3f0` (exact-head CI not independently verified this run). CONFIG-MODE-MISSING-01 is historical; use the live board. PR #143's offline task-selector CI guard is MERGED at `80de73b465ede49eec0882846c50efc0cbdee254`; FS-01B P0 remains blocked. `Feature-ID: ...` metadata and actual PR number registration are required by CI. Emergency file-preservation work must remain a separately justified safety interruption, not bypass the budget. No real source retirement without accepted FS-01B design and positive preservation. CI success is not owner-run Ubuntu/Compose or production release acceptance.

## Milestone documentation with implementation (owner-directed, 2026-11-20)

Borrow IMGENGINE's **compact living work board plus historical archive** and per-PR documentation-impact transaction, adapted to this native **C17/POSIX** project: [current NOW/NEXT board](next.md), [dated archive](next-history.md), [documentation transaction](docs/MILESTONE_DOCUMENTATION_WORKFLOW.md), and [PR checklist](.github/pull_request_template.md). Each meaningful code/test/CI milestone PR must update current `next.md` scope/status/evidence and the affected README, scoped AGENTS/skills, HLD/LLD, security, journal, Docker, perf or business docs **only when facts change**, explicitly recording UPDATED or N/A in the PR. Do not maintain parallel live queues or open repetitive docs-only PRs after every merge; after actual merge, reconcile main/merge SHA in the next authorized implementation PR. Historical labels do not outweigh live GitHub. C17 production, Rust test-only, same-UID source-retirement P0 and release limits remain unchanged. A Markdown checklist is not an autonomous scheduler or GitHub CI check.

## Advanced feature and product roadmap authority (2026-11-22)

The owner's future-feature candidates, staged dependencies and scope are in [advanced roadmap](docs/ADVANCED_FEATURE_ROADMAP.md), with [acceptance matrix](docs/ADVANCED_FEATURE_ACCEPTANCE.md) applying L1–L7 engineering, security/CISSP, Linux, proportionate networking/CCIE, SRE, CA, MBA and management lenses. Use [product strategy](docs/PRODUCT_AND_BUSINESS_STRATEGY.md), [FS-01B ADR](docs/architecture/ADR_FS_01B_SOURCE_RETIREMENT_BOUNDARY.md) and [API/ABI/FFI roadmap](docs/architecture/C-API-ABI-FFI-ROADMAP.md) for their authoritative domains. These docs are **DESIGN_ONLY**; `next.md` and live GitHub select actual authorized work, and `docs/feature-budget.json` owns active feature ID/budget. New feature selection requires explicit dependencies, acceptance, a reviewable bounded scope and a unique stable ID; never convert every roadmap row into a parallel open PR. No automatic duplicate deletion, production source retirement, remote upload, production Rust FFI, SDK release, paid resource or desktop deploy is authorized by a roadmap. Existing P0 FS-01B and REL-01 remain blocked until their approvals and positive safety evidence.

## Project operating dashboard and agent continuation

- Start every new task by reading the [living work register](next.md), [documentation authority map](docs/README.md), [seven-level engineering and business operating model](docs/ENGINEERING_OPERATING_MODEL.md), [CI/CD and Makefile operations](docs/CI_CD_AND_MAKEFILE_OPERATIONS.md), and [agent continuation/handoff](docs/AGENT_CONTINUATION_AND_HANDOFF.md), then the affected code and security contracts. These documents supplement, never override, the safety invariants here.
- The **top LIVE NOW / NEXT / LATER board** of `next.md` is the canonical cross-milestone status/dependency selector. The historical section is audit evidence, **not** a current queue; [component completion inventory](docs/PROJECT_COMPLETION_AND_STATUS_INVENTORY.md) separates built vs partial vs test-only vs production/release. Confirm current main/PR/check SHAs before treating a dated entry as current. A design, implementation, checked PR, merged PR and released binary are distinct.
- After a milestone, checkpoint exact SHA, reviewed diff, actual test/CI outcomes, blockers, risk, next READY item and owner decision. Use one bounded PR per milestone; never start its dependent implementation before the earlier PR is merged and verified.
- A running or scheduled coding agent may follow the documented next-task-selection procedure in its authorized session. Markdown alone does **not** start an automation, grant permissions, or authorize release, destructive user-file operations or spending. The owner's **separate, explicit, standing PR review/merge authorization** is recorded in [GOV-02](docs/AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md); it never bypasses branch protections or safety checks.
- Apply the seven engineering levels as proportional **review perspectives**, not seven automatic code-changing agents. Security, SRE, CTO and CA/MBA review should scale with risk and data-loss consequences.
- Keep heavy passing test logs in CI/artifacts; report exact SHA + job conclusions, retrieve only bounded failures. Change-scoped local tests never waive existing required PR/CI gates.

## Standing owner shorthand: go ahead = next eligible next.md work

When the owner says **"go ahead", "go ahead next", "continue" or "next milestone"** in the context of this repository, **do not ask them to restate the task list**. In the currently authorized agent session, first fetch actual GitHub `main`, open PRs, merged SHAs and required checks; read the **LIVE NOW / NEXT / LATER board** at the top of `next.md`; reconcile any stale labels; and select the **highest-priority dependency-unblocked bounded work item**. If the highest priority is blocked, state the blocker and choose a separate independent READY item. If an existing PR already covers the item, check its latest head CI and diagnose failures rather than repeating code. Create only the smallest justified reviewable change/PR, checkpoint the exact evidence and next action, and report what is implemented versus still open.

**Authority limits:** The bare phrase **"go ahead"** selects the next task; it is not by itself a blanket permission for dangerous operations. **Separately, on 2026-11-17, the owner explicitly authorized the existing Messy-File-Cleaner hourly autopilot to review and merge eligible PRs** with exact-head CI, review and protection gates. See [GOV-02](docs/AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md) for scoped merge authority and its limits. This does **not** authorize deleting customer files, destructive recovery, production releases, paid infrastructure, secrets/protection changes, history rewriting, or new production FCJ3/FCJ4/FFI without task-specific decisions. Markdown cannot run an agent on its own.

## Dual-trigger hourly autopilot (external scheduler)

The existing **Messy-File-Cleaner Autopilot** runs on an independent hourly ChatGPT schedule while enabled. The owner's manual `go ahead` is an immediate trigger **in addition** to that schedule, never a reason to pause or postpone it. User inactivity does not block authorized work. Each run must attempt one bounded material engineering advancement: resume an open PR and repair CI/reviews, merge a verified eligible PR, or implement source/tests/CI in a new reviewable PR. A successful scheduled invocation alone is not engineering progress. If checkout fails, try the GitHub connector; do not claim local tests ran. Escalate a repeated no-change blocker after two attempts and choose an independent READY safe task. Preserve C17 production, C11/C23 compatibility, test-only Rust isolation and all P0 filesystem safety restrictions. Report exact SHAs, checks, files, blockers, and next executable job. [GOV-02](docs/AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md) is the detailed dual-trigger and merge-gate authority. Repository text does not guarantee platform execution or authorize releases.

## Approved narrow Rust test-tooling exception

- The owner merged [Rust adoption ADR](docs/architecture/ADR_RUST_ADOPTION_STRATEGY.md) in PR #81 and the isolated test-support scaffold in PR #82. The **production engine remains pure C17**. The repository now contains Rust **test support only** under `tests/rust/`, with no production FFI, library or runtime dependency.
- Where this file or `skills.md` says “pure C,” interpret it as the production core/production native targets. It does not prohibit the **approved, isolated Rust test-support pilot**. No extra language may enter the production core without a separate approved architecture/security decision.
- No `.a`/`.so` product library, public ABI or production FCJ4 recovery is authorized by test-tooling acceptance. [Artifact policy](docs/architecture/RUST_TEST_SUPPORT_BOUNDARY.md) remains binding.
- `skills.md` is the operational recipe; uppercase `SKILLS.md` is a skill catalogue, not a competing agent authority.

## Mission and priorities
Build a dependable file-management engine that is safe with user data. Priorities: correctness and preservation of files; security and least privilege; recoverability and observability; measured performance; maintainability and delivery speed. Never trade data safety for a benchmark result. No performance claim without a reproducible benchmark and recorded environment.

## Supported development environment
- Ubuntu 24.04 LTS is the canonical Codespaces and container-development baseline.
- Use the repository Dev Container definition in `.devcontainer/`; do not install toolchains ad hoc into an unrelated base image.
- The development image must provide GCC, Clang, CMake, Ninja, Python 3, Git, and static-analysis/debugging tools.
- The production image is a separate multi-stage image based on Ubuntu 24.04 LTS. Keep build dependencies out of its runtime stage.
- Keep compiler, C standard, and build type explicit in validation commands. Record OS/image, compiler version, CMake version, flags, and container limits with benchmark results.
- Do not assume that Arch Linux, Debian, or host-installed tools are equivalent to the canonical development image.

## Makefile developer workflow and command safety
- Treat the repository `Makefile` as the primary documented local developer interface. Inspect `make help` and use its targets before inventing ad-hoc build/test commands; use direct CMake commands only when a needed target is not provided or when diagnosing the build system itself.
- Use `make` for the default Debug + ASan/UBSan build, `make test` for the Debug CTest suite, and `make validate` for isolated C11/C17/C23 Debug build-and-test runs.
- Keep build parallelism bounded. The Makefile defaults to `JOBS=2`; raise it only after checking available CPU and memory.
- `make run` must remain a dry-run. Never invoke `make run-execute` unless the owner explicitly requests real file operations and the input root/configuration have been reviewed; the target requires `CONFIRM_MOVE=YES` as an additional guard.
- Treat `make benchmark-moves` as a destructive test of its isolated temporary fixtures. Inspect the harness before changing its fixture/output behavior; never direct it at user data.
- `make clean` is limited to build/profiling artifacts. Do not broaden its deletion scope without explicit review and isolated tests.
- The Makefile is an interface, not a security boundary. Continue validating paths and options in the C application itself; quoting in shell recipes does not replace input validation.

## Architecture
- Keep the first product as a modular C application. Do not introduce microservices, brokers, databases, or a web server without a demonstrated requirement.
- Keep CLI, configuration, scanning, classification, queueing, moving, journaling, and platform filesystem operations behind clear module boundaries.
- Use Hexagonal Architecture selectively for filesystem, clock, logging, and persistence boundaries where tests benefit.
- Use patterns only to solve a real problem; avoid speculative factories, singletons, and abstractions.
- Document thread safety, ownership, lifetime, error, and cancellation contracts.
- The current lock-free queue must only be used with its proven producer/consumer topology. Never silently change SPSC assumptions to MPSC.

## C standards
- C17 is the default project language baseline for production builds.
- C11 compatibility is tested because the implementation relies on C11 atomics and should remain usable with mature toolchains.
- C23 is an optional compatibility target, not a requirement for users or the production release until supported compilers are validated.
- Select the standard explicitly with `-DFC_C_STANDARD=11`, `17`, or `23`; CI must build and test all three independently.
- Do not add C23-only syntax to core modules unless a documented need outweighs the portability cost.
- Use clear names, small functions, single-purpose modules, and explicit status returns.
- Validate external input, sizes, integer conversions, path lengths, and allocations.
- Check all correctness- and security-relevant system-call results; preserve errno across cleanup.
- Avoid unbounded recursion and unchecked fixed-size path buffers.
- Avoid global mutable state in new code; pass context explicitly.
- Define who allocates, frees, and owns every pointer.
- Do not hide warnings, use undefined behavior, or rely on undocumented platform assumptions.

## Filesystem safety
- Treat paths, filenames, configuration, and watcher events as untrusted.
- Default new user-facing workflows to dry-run.
- Do not follow symlinks accidentally. Prefer descriptor-relative APIs and no-follow flags where supported.
- Defend against traversal, symlink swaps, TOCTOU, special files, and source/destination overlap.
- Configured output folders must be relative single components; reject absolute paths, dot-dot, nested separators, and the reserved journal directory.
- Canonicalize the input root and reject overlap with configured outputs, fallback output, and journal paths.
- Reject symlink input roots and symlink/non-directory output destinations; --force must not bypass these invariants.
- Never delete source until destination is fully written and safely published.
- Use exclusive creation; never truncate an existing user file.
- Journal intent and completion. Recovery must be idempotent and must not guess when state is ambiguous.
- Force must not bypass path validation or filesystem invariants.
- Destructive tests must use isolated temporary directories and assert source preservation on failures.

## FCJ4 journal and recovery safety gates

- FCJ4 is a separate future protocol; never append FCJ4 records to the FCJ2/legacy `journal/move.log` and never mix FCJ3 with FCJ2/legacy.
- The FCJ4 parser/replay must remain pure, whole-buffer, all-or-nothing, and read-only. Replay validity is not authenticity, filesystem truth, or authorization to recover.
- Keep FCJ4 parser, serializer, and in-memory writer-state modules isolated from the production executable and CLI until a separately reviewed integration milestone is approved.
- Writer prepare/commit is in-memory state management only. Never describe it as a durable journal commit.
- The approved Linux-only FCJ4 test adapter may use a caller-supplied descriptor only for a disposable private temporary file. It must remain in the test-support tree and a Linux-only test target, use explicit-offset `pwrite()`, reject `O_APPEND`, validate identity/size/ownership/mode/link count, acquire a non-blocking advisory lock, and require the fsync → writer-commit acknowledgement handshake.
- The test adapter must never open/create `journal/move.fcj4`, enter the production source list/executable, or be exposed through the CLI. Advisory locking does not stop non-cooperating writers; process-exit tests do not prove power-loss durability.
- No production FCJ4 descriptor, lock, journal creation, append/write, fsync/close adapter, planner, executor, quarantine, publication, source retirement, deletion, or recovery mutation is authorized by the test-adapter design or implementation.
- Recovery action lifecycle is independent from move lifecycle. Move completion must never imply action completion.
- Never unlink or retire a source until a separately approved protocol and execution design proves durable recovery evidence and exact filesystem identity checks.

## Concurrency
- Document producer/consumer counts, ownership transfer, synchronization, and shutdown.
- Check pthread, eventfd, read, write, close, and allocation failures.
- No busy-spin or lock-free optimization without measurement and a documented correctness proof.
- Bound queues, worker counts, and memory. Shutdown must wake workers and account for pending work.

## Performance
- Establish a baseline before optimizing.
- Benchmark small files, large files, deep trees, many directories, and cross-filesystem moves.
- Record files/sec, bytes/sec, wall time, CPU, syscalls, memory, filesystem, storage, kernel, compiler, flags, and container limits.
- Prefer algorithmic and syscall improvements over speculative assembly.
- Do not use -march=native in portable release artifacts.
- Run sanitizers and correctness tests before performance comparisons.
- Treat nanoseconds as a measurement unit, not a promise that filesystem operations complete in nanoseconds.
- Target nanosecond-level latency only for suitable CPU-bound operations (for example, in-memory classification or hashing) when repeatable measurements support it.
- For filesystem operations, report realistic distributions and throughput; separate metadata-only rename from cross-filesystem copy, cold-cache traversal, and durable journal writes.
- Use monotonic clocks, batched microbenchmarks, repeated runs, and median/tail percentiles. Account for timer overhead and measurement noise; never optimize from a single minimum sample.

## API, ABI, and FFI
- Keep the public C API small, versioned, and independent of internal structs.
- Prefer opaque handles and fixed-width integer types.
- Define ownership and matching release functions.
- Define ABI version negotiation and size/version fields for extensible structs.
- Never expose pthread types, FILE pointers, internal queues, or platform details in the stable ABI.
- Rust FFI must use the C ABI, repr(C) where required, explicit pointer/length contracts, and documented safety invariants.
- The accepted Rust test-support crate in `tests/rust/` is the sole approved exception to C-only development. Any **new** Rust component, C/Rust FFI connection, or production use requires its applicable staged ADR review, concrete test consumer or measured safety benefit, and explicit scope approval.
- Panics, exceptions, and longjmp must never cross the C ABI boundary.

## Security and delivery
- Containers run non-root, drop capabilities, use read-only root filesystems, and mount only required paths.
- Never mount Docker socket or host root.
- Do not commit credentials, user files, profiling dumps, or runtime data.
- Every behavior change needs focused tests or a documented reason tests are not yet possible.
- A test suite with no registered tests is not evidence of correctness.
- Docker changes must build in CI and runtime tests must use disposable mounted directories.
- Update README, HLD/LLD, and runbooks when behavior changes.
- PRs state scope, risk, security impact, tests actually run, gaps, and rollback plan.
- Never claim tests, builds, merges, or runtime behavior without evidence.

## Milestone completion and merge governance

Every milestone is an independently reviewable delivery unit with written scope and acceptance criteria.

1. Work on a dedicated milestone branch created from the latest `main`.
2. Before implementation, record the objective, scope, risks, security impact, and measurable acceptance criteria.
3. A milestone passes only when its required tests, security checks, documentation, and acceptance criteria pass. A successful compile alone is not a pass.
4. Record exact commands, CI run links/results, test outcomes, and known limitations in the pull request.
5. After the milestone passes, review the **exact PR diff, comments, unresolved threads, relevant tests and current-head CI**. If working under the owner's **active GOV-02 standing autopilot merge authorization**, merge only eligible, correctly scoped, non-blocked PRs in compliance with [merge policy](docs/AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md) and GitHub protections, using the expected head SHA. Otherwise **request specific owner approval**.
6. Verify the merge commit and rerun/inspect required checks on `main`.
7. Start the next milestone from the updated `main`; do not stack the next milestone on an unmerged branch.
8. If any gate fails, keep work within that milestone, fix it, and rerun the gates. Do not declare it complete or advance.
9. Never merge a failing, unreviewed, or unverified milestone just to maintain schedule. If GitHub permissions or branch protections prevent the merge, report the blocker and ask the repository owner to resolve it.

The repository owner expects eligible, reviewed milestones to reach `main` before their dependents begin. **Specific PR approval OR the owner's active GOV-02 standing authorization** is required for merge; the latter is not permission to bypass P0 data-safety/CI/design gates. Never claim a merge until GitHub confirms the exact resulting commit.

## Product and management
Prioritize customer value, data safety, reliability, support cost, and total cost of ownership. Build the smallest reliable product first. Add GUI, API server, cloud, and distributed processing only when customer demand justifies them.


## Mandatory production language: pure C17; isolated Rust test support

The project owner has made a firm product and engineering decision: **the production engine and shipped native targets are pure C, not C++**. The accepted Rust ADR permits a narrow **test-only** Rust-support crate; it does not authorize a Rust production core.

- Implement the application, core modules, production tests/tools and shipped native build targets in C; the approved isolated Rust verification harness under `tests/rust/` is the only present exception.
- Do not introduce C++ source files or C++ implementation: no `.cpp`, `.cc`, or `.cxx` files; no C++ classes, templates, exceptions, STL, or C++ runtime dependency.
- Do not enable CXX in CMake or add C++ compiler/build targets.
- Use C17 as the default production language baseline. Preserve C11 compatibility where practical and validate C23 as an optional compatibility target; do not let C23-only features silently raise the minimum compiler requirement.
- Use portable, well-defined C and documented POSIX/Linux interfaces. Keep platform-specific code behind narrow C module boundaries.
- If interoperability is ever required, expose a documented, versioned C ABI with explicit ownership, lifetime, error, and thread-safety contracts. A C ABI does not authorize rewriting the core in C++.
- Assembly may be considered only for a measured, CPU-bound bottleneck. Keep it isolated behind a C interface, provide a tested portable C fallback, and verify correctness and security implications. Never use assembly speculatively.
- Do not introduce Rust, C++, or another implementation language into the core without an explicit new decision from the project owner and a documented architecture/security review.

This policy overrides generic examples or recommendations that suggest C++ as an option. When in doubt, choose C.


## Mandatory engineering playbook and skills

The detailed, enforceable engineering standards live in:
- `skills.md` — day-to-day contributor and coding-agent workflow, review checklist, and implementation recipes.
- `SKILLS.md` — non-binding knowledge catalogue; it does not override `AGENTS.md`, the accepted ADRs or `skills.md`.
- `next.md` — living milestone status, sequencing and evidence board; it is not itself approval for a risky operation.
- `docs/architecture/ENGINEERING_PLAYBOOK.md` — HLD/LLD, module design, architecture styles, design patterns, SOLID/DRY/KISS, testing, and documentation rules.
- `docs/security/THREAT_MODEL.md` — assets, trust boundaries, attacker model, filesystem threats, controls, and security acceptance gates.

These documents are normative together with this file. If guidance conflicts, apply this precedence:
1. Explicit current project-owner decisions and accepted ADRs (C17 production, isolated Rust test-only pilot), without weakening mandatory data-preservation rules.
2. Safety and security invariants in this file and the threat model.
3. Public API/ABI and journal-format compatibility contracts.
4. Architecture playbook and skills.
5. Local implementation convenience.

### Design and implementation rules
- Apply SOLID as a design aid, not as a requirement to create one-file-per-function abstractions. In C, use cohesive modules, opaque context structs where useful, narrow headers, and explicit function contracts.
- Apply DRY to duplicated knowledge and policy, not merely repeated syntax. Do not create premature generic frameworks.
- Prefer KISS and YAGNI. Every abstraction, dependency, thread, cache, state, and configuration option must have a documented reason.
- Use DDD selectively: name core concepts consistently (scan, classification, move operation, journal transaction, recovery); keep domain decisions independent from OS calls where practical. Do not create artificial aggregates/repositories for simple local data.
- Use Hexagonal Architecture at real boundaries (filesystem, journal persistence, clock, logging, randomness, and test fault injection). Keep adapters replaceable in tests; do not build a framework around every call.
- Use pipeline/producer-consumer for bounded work flow. Preserve documented queue topology and ownership transfer.
- Treat journal processing as a durable transaction state machine. Journal records are not a distributed event bus. Do not claim full Event-Driven Architecture unless independent producers/consumers communicate through an explicit event contract and delivery semantics.
- Design state transitions explicitly: allowed predecessor, next state, immutable transaction identity, idempotency, failure semantics, and crash points must be documented and tested.
- Keep platform-specific Linux/POSIX behavior behind narrow C interfaces and document unsupported filesystems/platforms.

### Required change discipline
- Before changing a module, read its public header, implementation, callers, tests, and relevant design docs.
- Before changing persistent formats, public APIs, concurrency, filesystem mutation, or recovery, write/update an ADR or design document and obtain review before implementation.
- For every behavior change, update focused unit/integration/fault-injection tests and user/operator documentation.
- Never weaken an invariant to make a test pass. Fix the test only when its assumption is demonstrably wrong and explain why.
- Do not use `assert()` for expressions with side effects. Tests must remain meaningful in Release/NDEBUG builds.
- Keep production behavior independent from test-only hooks. Compile test fault injection only under the designated test configuration.
- Preserve errno and original failure context across cleanup paths. Cleanup failures must be reported without hiding the primary failure.
- Use bounded inputs, checked arithmetic, explicit integer conversions, and allocation overflow checks.
- No silent fallback to a weaker security or durability guarantee. Unsupported capability means an explicit safe failure or documented non-destructive mode.

### Required verification and reporting
- Run the narrowest relevant test first, then `make test`, then `make validate` for changes that can affect core behavior.
- Run static analysis, sanitizer, Docker/non-root, and crash/fault-injection checks according to the risk of the change.
- For journal, mover, recovery, path, or deletion changes, require adversarial filesystem tests and crash-boundary tests; a normal happy-path test is insufficient.
- Report exact commands and observed results. Separate tests passed, tests not run, known limitations, and inferred risks.
- A benchmark is evidence only for the measured workload and environment. Never generalize one filesystem, one machine, or one run into a universal performance claim.
- Do not modify or delete user data during development. Destructive tests must create and verify disposable fixtures.


CONFIG-EXT-MISSING-01: bare or empty ext.* destination mappings are rejected; PR pending CI. CONFIG-MODE-MISSING-01 merged in #152 at `aba6200dd4d6ecaa6306d720e81e733c6e6e5af3`. Manual acceptance NOT RUN; FS-01B/REL-01 blocked.


## 23 Nov 2026 bounded config-read handoff

CONFIG-READ-ERROR-01: fail closed on `ferror`/`fclose` in configuration parsing; disposable Linux directory-read negative regression. PR exact-head CI and merge are pending. No customer files, source retirement, release, production Rust, or deployment authorized. CONFIG-EXT-KEY-01 #154 merged at `5b858c90c4c63b4a978f6fcaf9e159de5c4e02a1`; manual acceptance NOT RUN. Other architecture/ABI/FCJ/security/Docker/performance/business documents N/A: no interfaces, filesystem mutation, operational or financial behavior changed.


> **CONFIG-ATOMIC-LOAD-01 (23 Nov 2026, proposed PR):** C17 configuration loading now stages scalar settings and caller-owned extension map entries, committing only after successful parse/read/close. Failed loads retain prior caller state; disposable late-failure regression added. This is not a release or filesystem-retirement safety approval. Exact-head CI and manual acceptance remain NOT RUN pending PR verification. FS-01B and REL-01 remain BLOCKED.

> **CONFIG-KEY-WIDTH-01 (24 Nov 2026, proposed PR):** Reject overlong config keys before bounded sscanf parsing; disposable regression and same-PR feature-ledger reconciliation. C17 production; C11/C23 compatibility requires CI verification. No source retirement, deployment, release or ABI change. FS-01B/REL-01 BLOCKED; manual acceptance NOT RUN.
