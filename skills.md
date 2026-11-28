# Engineering Skills and Contributor Workflow

**Linux-native next research (2026-11-28):** #312 merged at `23edab1ac888cae370d2f43c38c03a5aef7e0551`; exact-head 14/14 checks PASS, GCC C17 Release and Ubuntu C17 95 passed/1 skipped (96 registered), all three Inotify references PASS (not skipped); integrated-main 10/14 checks PASS, 4 pending at inspection; independent review unverified. WATCH-OVERFLOW-EVAL-01 fourth candidate adds bounded fresh inotify instance restart after real overflow, discards pending old-instance events, validates quiescent snapshot and accepts only new-instance notifications. No production recovery, event replay, arbitrary concurrent consistency, imports, spend, release or FS-01B closure. CI pending. [Hypotheses and evidence](docs/upstream/LINUX_NEXT_HYPOTHESES.md).

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

**Owner-approved Linux-inspired library architecture (2026-11-27):** [Engineering requirements](docs/architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). Required skills: ar/nm/linker artifact inspection, reproducibility, dependency direction, API/ABI ownership, visibility, sanitizer/fault testing and measured optimization; no unsupported kernel-grade claim.

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

> **Current PR workflow:** the [zero-human-review owner policy](docs/AUTOPILOT_ZERO_REVIEW_AND_DOC_SYNC.md) authorizes autopilot self-review and gated merges without human approval; never present the self-review as external. Inspect current GitHub before using dated references below. Import [ruleset JSON](.github/rulesets/autopilot-main-no-human-review.json) separately.

This file is an operational companion to [AGENTS.md](AGENTS.md). It tells humans and coding agents how to work in this repository. The architecture and security rationale is in [the Engineering Playbook](docs/architecture/ENGINEERING_PLAYBOOK.md) and [Threat Model](docs/security/THREAT_MODEL.md).

## Five-PR execution and handoff recipe

For every new feature PR, select the stable active ID in [docs/feature-budget.json](docs/feature-budget.json), put `Feature-ID: ID` in the PR body, open one focused PR, then **add GitHub's actual assigned PR number to the ledger in that same PR**. Local CI validates five slots and the PR association. Count closed/failed PRs; never reset by renaming tasks. End after up to five, document DONE/PARTIAL/BLOCKED with exact head/main SHA, manual NOT RUN and remaining FS/data-preservation risks, and switch to the next independent READY feature. Historical PLAN-REPORT-01 is PARTIAL; select the current task from the live board. Do not enable production moves or treat reproduced unsafe unlink as fixed. Full rules: [feature policy](docs/FEATURE_DELIVERY_AND_ACCEPTANCE_POLICY.md).

## Advanced-feature intake checklist

Consult the [advanced roadmap](docs/ADVANCED_FEATURE_ROADMAP.md) and its [acceptance matrix](docs/ADVANCED_FEATURE_ACCEPTANCE.md) **after** live GitHub, [next.md](next.md) and the [feature ledger](docs/feature-budget.json). If the live board selects a read-only preview gap: inventory actual dry-run output, write a bounded schema/negative-case plan, require positive source/destination/journal non-mutation assertions and protect untrusted filename output. Do not start blocked P0 mover/undo/recovery changes or future C API/Rust FFI/desktop work without accepted design, real consumer or owner approval. Apply L1–L7, CISSP, Linux/SRE, proportionate CCIE, CA/MBA/management review with measurable evidence, no marketing claims. When a candidate becomes active, keep one stable ID and <=5 opened feature PRs, record actual PR number in same PR and update affected docs with UPDATED/N/A. The roadmap is not a second NOW queue.

## Project-specific documentation-impact recipe (2026-11-20)

Start with the compact [live work board](next.md) and live GitHub status; use [next-history.md](next-history.md) only as a dated archive. For every meaningful change, keep documentation in the **same implementation PR**: identify current milestone, tested SHA, PR/CI evidence and next READY task in `next.md`; review AGENTS, README, this contributor recipe, the docs index and the relevant FS-01/FCJ3/FCJ4, HLD/LLD, CMake/Makefile, Docker, C11/C17/C23, Rust test-only and benchmark contracts. List UPDATED or N/A and why in the [PR template](.github/pull_request_template.md). Do not duplicate live status throughout docs, create status-only PRs after every merge, or claim a green test fixes the same-UID source-retirement P0. [Canonical transaction](docs/MILESTONE_DOCUMENTATION_WORKFLOW.md).

## Always-on task selection and evidence recipe

1. Open the **LIVE NOW / NEXT / LATER board** in [next.md](next.md), the [component completion inventory](docs/PROJECT_COMPLETION_AND_STATUS_INVENTORY.md) and [docs index](docs/README.md). Confirm actual `main` SHA, outstanding PRs and CI before choosing the next READY milestone. Retained historical `next.md` notes must never override the live board.
2. Read [agent continuation](docs/AGENT_CONTINUATION_AND_HANDOFF.md), [engineering operating model](docs/ENGINEERING_OPERATING_MODEL.md), [CI/Makefile operations](docs/CI_CD_AND_MAKEFILE_OPERATIONS.md), and the affected module's specific contracts/tests.
3. Choose **one** bounded dependency-unblocked task. Define acceptance, negative cases, filesystem and data preservation invariants, scope, non-goals, rollback and finance/support cost.
4. Execute focused local tests, open review PR when authorized, inspect the **current exact-head** required CI checks, and summarize only statuses and failed-step excerpts.
5. Do not call CI pass MERGED; do not call MERGED RELEASED. The owner has **separately granted scoped standing PR merge authorization** to the hourly repository autopilot; use the exact-head review/protection checks in [GOV-02](docs/AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md). Human approval remains required for risky or blocked changes, real customer-file operations, production releases and paid/irreversible actions.
6. A task file is a continuation instruction, not an always-running AI system. Resume later in a new approved session/runner from GitHub and the checkpoint.

### Standing owner command: "go ahead"

In a repo-related conversation, interpret **"go ahead" / "continue" / "next"** as **start the next eligible bounded item from the live `next.md` NOW/NEXT/LATER queue without requiring another copy-paste prompt**. First reconcile GitHub main/PRs/CI. Work on the existing PR if it is still open or its checks failed; never create duplicate work from stale history. Otherwise choose highest-priority unblocked work, scope it, make a review PR, record tests/NOT RUN and update the queue. If blocked, explain the missing prerequisite and advance a different independent READY item only when it stays within ordinary non-destructive authorization. See [agent continuation](docs/AGENT_CONTINUATION_AND_HANDOFF.md). The separate [owner-authorized GOV-02 PR review/merge process](docs/AGENT_AUTOPILOT_PR_REVIEW_MERGE_POLICY.md) permits **eligible** automatic PR merges only after mandatory checks; **no bypass of security, destructive user-file operation, release, spending or background execution from Markdown alone**.

### Test-only Rust exception

PR #81 accepted a narrow Rust verification pilot; PR #82 merged a dependency-free `tests/rust/fcj4-test-support` crate. Production remains pure C17. For this isolated crate, run:

```sh
cd tests/rust/fcj4-test-support
cargo fmt --all -- --check
cargo clippy --locked --all-targets -- -D warnings
cargo test --locked
```

Rust FFI is **not implemented**; a future test-only shim requires a separate safety review. Public static/shared libraries, production Rust and FCJ4 real recovery are not authorized. Existing C11/C17/C23, Docker, sanitizer and required CI checks remain mandatory. `SKILLS.md` is a catalogue; this lower-case file owns contributor workflow recipes.

## 1. Non-negotiable project identity

- Implement the **production product in pure C**. C17 is the production baseline; validate C11 and C23 independently. The accepted, isolated Rust test-support pilot under `tests/rust/` is an explicit exception, not permission for production Rust.
- No C++, C++ runtime, production Rust core or additional product implementation language without a new explicit owner decision and written review. The approved test-only Rust crate is governed by the accepted ADR.
- Assembly is allowed only after profiling proves a CPU-bound bottleneck; it must be isolated behind C, have a portable C fallback, and pass equivalence tests.
- Linux/POSIX is the current platform target. Document platform and filesystem assumptions; do not imply universal portability.
- Data preservation and security outrank speed or delivery convenience.

## 2. Start-of-task workflow

1. Read `AGENTS.md`, this file, relevant architecture/security docs, and the module's public header.
2. Confirm the current branch and latest `main`; do not build new work on an obsolete or unmerged milestone.
3. Inspect callers, tests, error paths, ownership, thread topology, and existing invariants before editing.
4. State the task's scope, expected behavior, risks, and acceptance criteria.
5. For journal formats, recovery, destructive filesystem operations, public API/ABI, or concurrency changes, write or update a design/ADR before implementation.
6. Use a dedicated milestone branch. Keep changes small enough to review, but do not split a logically atomic safety change across uncoordinated PRs.

## 3. Design skills

- Model the domain before selecting a pattern. Use precise terms for source, destination, move operation, journal transaction, publication, retirement, and recovery.
- Keep modules cohesive and interfaces narrow. Prefer opaque context types and explicit status/error returns over hidden global state.
- Separate policy from mechanism: domain decisions should not depend unnecessarily on raw syscalls; filesystem adapters perform OS-specific operations.
- Use dependency inversion at meaningful boundaries (filesystem, journal, clock, randomness, logging) so tests can inject deterministic failures.
- Use state machines for workflows with durable or externally observable transitions. Define states, allowed transitions, terminal states, retry rules, and crash behavior.
- Use event-like notifications for worker coordination where they improve responsiveness. Do not introduce a broker or distributed EDA into this local application without a real multi-process/distributed requirement.
- Apply SOLID, DRY, KISS, and YAGNI pragmatically. Avoid both duplicated business rules and abstraction-heavy frameworks.
- Document ownership, lifetime, nullability, thread safety, cancellation, blocking behavior, and error semantics in public headers.

## 4. C implementation skills

- Compile with the project's explicit C standard and strict warnings. Do not depend on compiler extensions unless isolated and documented.
- Use fixed-width integer types for persistent formats and stable interfaces.
- Validate all external input, path components, lengths, integer conversions, arithmetic, allocation sizes, and syscall results.
- Check for overflow before allocation or buffer arithmetic.
- Prefer bounded formatting and parsing; check truncation and conversion results.
- Define ownership for every allocated object and file descriptor. Close/free exactly once; preserve the primary error during cleanup.
- Do not return pointers to temporary storage or expose internal structs across a stable ABI.
- Avoid undefined behavior, data races, unsafe signal-handler work, and non-atomic access to shared state.
- Do not place side effects inside `assert()`; tests must execute correctly with `NDEBUG`.
- Keep test hooks out of production builds.

## 5. Filesystem and transaction skills

- Treat names, paths, directory entries, symlinks, mount points, and watcher events as untrusted and potentially changing concurrently.
- Prefer descriptor-relative operations, `O_NOFOLLOW`, `AT_SYMLINK_NOFOLLOW`, exclusive creation, and no-replace publication where supported.
- Check source and destination identity/type immediately before operations that depend on them; document remaining race boundaries.
- Never overwrite an existing user destination.
- Never remove the source until the destination is complete, verified, safely published, and the protocol permits retirement.
- Never infer ownership of a temporary file from a filename prefix.
- Journal records are durable evidence, not ordinary log messages. A partial, malformed, conflicting, or ambiguous journal must fail closed.
- Recovery must be idempotent where possible and must preserve unknown or mismatched entries.
- FCJ2 and legacy records must retain their documented compatibility behavior. Old formats must never authorize cleanup of temporary entries that they cannot identify.
- Do not add a silent fallback that weakens atomicity, durability, or security.

## FCJ4 journal-state skills

- Treat FCJ4 as a separate protocol, not an extension of FCJ2/legacy or FCJ3.
- Keep replay parsing pure and all-or-nothing. Never perform filesystem or journal I/O in the parser.
- Reconstruct writer state only from a successfully parsed complete replay, and deep-copy all evidence needed beyond replay lifetime.
- Validate every proposed SEALED/ACTION_DEF record against immutable BEGIN/SEALED evidence before prepare. Writer acceptance must not create a history the parser rejects.
- Preserve exact move and action sequences, independent lifecycle states, unique IDs, quarantine-action dependencies, and protocol resource limits.
- Keep prepare/commit/abort semantics explicit: in-memory commit is not durable append or fsync.
- Keep FCJ4 parser, serializer, and writer-state modules isolated from production. The separately approved Linux-only test append adapter belongs only under `tests/support` and may operate only on caller-supplied descriptors for disposable private temporary files.
- Use explicit-offset `pwrite()`, reject `O_APPEND`, validate descriptor/file identity and private-file properties, and acquire a non-blocking advisory lock. Document that advisory locks do not constrain non-cooperating writers.
- Enforce the durable-write → writer-commit acknowledgement handshake. Do not accept another append while a durable record is pending in-memory commit; on commit mismatch, poison the session and rebuild from complete replay.
- Test short writes, EINTR, partial-write and fsync uncertainty, cleanup errors, descriptor ownership on init failure, restart/replay, and process exit. Process termination is not a power-loss test.
- Never open/create the production FCJ4 journal path, link the adapter into production, expose a CLI, or add recovery planners/executors or filesystem mutations as part of this milestone.

## 6. Concurrency skills

- State the number and roles of producers/consumers for every queue.
- Document who owns each item before enqueue, while queued, after dequeue, and on cancellation/shutdown.
- Bound queue capacity, worker count, and memory use.
- Handle partial thread startup and every synchronization/notification failure.
- Shutdown must stop new work, wake blocked workers, account for active and queued work, join threads, and only then close shared resources.
- Do not assume a mutex makes concurrent close safe unless the lifecycle contract proves it.
- Use atomics only with a written memory-ordering and topology rationale. Prefer simpler synchronization until profiling proves a need.

## 7. Test skills

Use layered tests:
- Unit: parsing, validation, classification, state transition rules, checked arithmetic.
- Integration: module boundaries and real temporary directories.
- End-to-end: CLI behavior, exit codes, dry-run and explicit execution guard.
- Fault injection: syscall failures, short reads/writes, EINTR, ENOSPC, EIO, fsync, close, publication, and cleanup errors.
- Crash/restart: terminate a child process at each durable transition and verify exact bytes, paths, journal state, and repeat recovery.
- Adversarial filesystem: symlink swaps, path replacement, hard links, special files, mount/filesystem capability differences, name collisions, and permission boundaries.
- Compatibility: C11/C17/C23; Debug and Release.
- Runtime: sanitizers, static analysis, Docker, and non-root tests where configured.

Assertions:
- Verify exact source and destination bytes, not only existence.
- Check file type, device/inode identity, permissions, and directory durability when relevant.
- Verify unknown or unowned entries are untouched.
- Verify retry behavior after transient and permanent errors.
- Ensure tests are deterministic and do not depend on root privileges unless root-specific behavior is the subject.
- Keep fixtures isolated and clean only files created by that fixture.

## 8. Makefile workflow

The Makefile is the primary developer interface:
- `make` — default Debug/sanitizer build.
- `make test` — Debug CTest suite.
- `make validate` — isolated C11/C17/C23 validation.
- `make release` — Release build.
- `make run` — dry-run.
- `make run-execute CONFIRM_MOVE=YES` — real file operation; only after explicit owner request and input review.
- `make benchmark` / `make benchmark-moves` — inspect workload and fixture isolation before use.
- `make clean` — build/profiling artifacts only.

Keep parallelism bounded (default `JOBS=2`). The Makefile is not a security boundary; application validation remains mandatory.

## 9. Security review checklist

Before approving code, ask:
- What are the assets and trust boundaries?
- Can an attacker control the path, filename, file type, directory, mount, configuration, or timing?
- Is there a symlink, traversal, TOCTOU, hard-link, special-file, or same-UID race?
- Can a failure cause overwrite, data loss, partial publication, or false recovery?
- Are integer overflow, truncation, resource exhaustion, and unbounded work possible?
- Are permissions, ownership, descriptor lifetime, and cleanup correct?
- Are errors fail-closed without destroying evidence?
- Are test-only controls absent from production?
- Does the threat model accurately state residual risks?

## 10. Pull request quality

Every PR must include:
- Problem and user/business impact.
- Scope and explicit non-goals.
- Architecture/design decision and affected module contracts.
- Data-safety and threat-model impact.
- Compatibility/API/journal-format impact.
- Tests actually run, with commands and CI links/results.
- Tests not run and known limitations.
- Performance measurements only when relevant, with environment and repeat count.
- Rollback or recovery plan.

Do not claim a milestone passed until its acceptance criteria pass. A passing PR is READY_FOR_REVIEW, not MERGED. Merge only with **specific owner approval or the active GOV-02 owner-authorized autopilot policy**, after full exact-SHA review, CI/protections and security gates. Verify the merge on `main` before starting dependent work.

## 11. Documentation maintenance

**Authority separation:** `AGENTS.md` sets binding engineering/safety rules, the accepted ADRs constrain architecture, this lower-case `skills.md` provides task execution recipes, uppercase `SKILLS.md` is a non-binding knowledge catalogue, `next.md` is the single current-status board and `docs/README.md` is the documentation index. Do not duplicate a task's status or contradict a newly accepted ADR in multiple places.


Update the relevant documents in the same change:
- User-visible behavior: README and CLI help.
- Module contract: header comments and LLD.
- Architecture or persistent protocol: HLD/LLD and ADR.
- Security behavior: threat model and security runbook.
- Operations: build, recovery, troubleshooting, and release notes.
- Tests: describe what is proven and what remains unproven.

Documentation must distinguish implemented behavior from planned design. Never present a design proposal as a production guarantee.
