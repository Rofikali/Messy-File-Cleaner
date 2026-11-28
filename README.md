# Messy File Cleaner

**PIDFD epoll identity routing candidate (2026-11-28):** #325 merged at `f64030a0b1a2e82eb62643c36928508e1d5ac319`; exact head `94e4ccffe4665ed48129477a1d2a50e63581a221`, five workflow families and 14/14 returned jobs PASS, C17 Release and Ubuntu C17 106 passed/2 skipped, both PIDFD references PASS. Head-to-main comparison returned zero changed files; integrated-main CI NOT VERIFIED. Third test-only candidate registers owned gated child and live self pidfds with distinct epoll tags; initial empty readiness, child-only exit event, deregistration and child reaping are required. Linux-first owner policy applies proactively even outside the roadmap: state unmet need, upstream interface, falsifiable hypothesis, baseline, negative controls, security, portability, provenance/license, maintenance/TCO, customer value, rollback and adopt/retain/reject/defer decision; no blind kernel source copying. Local NOT RUN; candidate CI pending. No production helper, imported code, dependencies, spend or release; FS-01B-FIX and REL-01 remain blocked.

**PIDFD negative-control candidate (2026-11-28):** #324 merged at `f3a9788ce603c746af2db6f12ec95e3ada25a050`; exact head `5fbfd2a3bec75974052f55ac44b0288080951901`, five workflow families and 14/14 returned jobs PASS, C17 Release and Ubuntu C17 105 passed/2 skipped, pidfd lifecycle PASS. Head-to-main comparison returned zero changed files; separate integrated-main CI NOT VERIFIED. Second test-only candidate opens pidfd for self, rejects invalid flags with EINVAL, confirms no readiness while live and verifies EBADF/POLLNVAL after close without intervening allocation. Missing header or restricted/unavailable syscall is NOT VERIFIED, not PASS. Original owned-child test unchanged. Local NOT RUN; candidate CI pending. No production helper, external source import, dependencies, spend or release; FS-01B-FIX and REL-01 blocked.

**Linux pidfd research (2026-11-28):** #323 merged at `d579c883fe6eb86546ddedca13c909ac994fb7db`; four returned exact-head workflow families passed, Ubuntu in progress at inspection; C17 Release 104 passed/2 skipped, actual reflink fallback/error references PASS, clone NOT VERIFIED (EOPNOTSUPP 95, magic 0xef53). REFLINK-EVAL-01 PARTIAL 3/5; retain references, DEFER production; do not spend slots on repeated skips. Select distinct PIDFD-LIFECYCLE-EVAL-01 test-only reference for a possible future isolated preview helper: own child, pipe-gated exit, pidfd CLOEXEC, poll readiness and waitpid cleanup. No production helper approval. Local NOT RUN; CI pending; independent review and integrated-main unverified. No imports, dependencies, spend or release.

**Reflink error policy research (2026-11-28):** #322 merged at `626fb5c368617d80c83a058d90dde934e1e1e029`; five exact-head workflow families PASS, C17 Release and Ubuntu C17 103 passed/2 skipped, actual unsupported fallback PASS, clone NOT VERIFIED. C17 Release fixture filesystem magic 0xef53, FICLONE EOPNOTSUPP (95). Third test-only candidate tests four allowed and twelve denied policy cases, actual invalid destination EBADF and source marker/identity preservation. Unexpected errors remain fail-closed; no claim of actual injected EIO/ENOSPC. Local NOT RUN; CI pending; independent review and integrated-main unverified. No production change, imports, dependencies, spend or release.

**Reflink fallback research (2026-11-28):** #321 merged at `ae205795daf6c629a70582a8e673824ae0e556c3`; five exact-head workflow families PASS, C17 Release and Ubuntu C17 102 passed/2 skipped, reflink capability NOT VERIFIED in both. Second test-only candidate exposes filesystem magic and actual ioctl errno, discards attempted destination on explicitly unsupported capability, copies into a fresh distinct temporary file and verifies complete bytes and bidirectional write isolation. Supported clone environments skip the actual-unsupported fallback test rather than fake evidence. No production fallback, imports, dependencies, spend or release. Local NOT RUN; CI pending; independent review and integrated-main unverified.

**Linux reflink research (2026-11-28):** #320 merged at `ae2554740c2cecbeafa36738f92a371194c98f27`; five exact-head workflow families succeeded; C17 Release 102 passed/1 skipped, all five sparse tests PASS. Four paired 4 MiB trials submitted 8192 versus 4194304 bytes, reported 16 versus 8192 512-byte blocks, median 0.1845 versus 3.448 ms (18.7x on this fixture only). SPARSE-EXTENT-EVAL-01 PARTIAL 5/5; retain reference, production DEFER, no sixth PR. Independent review and integrated-main unverified. Select distinct REFLINK-EVAL-01 first test-only FICLONE candidate, 64 KiB dense fixture, full byte oracle and bidirectional write isolation; capability skip not proof. Local NOT RUN; CI pending. No production change, imports, dependencies, spend or release.

**Sparse extent final acceptance (2026-11-28):** #319 merged at `11278c4f9bc77c380231b06ea26d7e81e7d8c2fb`; five returned exact-head workflow families succeeded, C17 Release and Ubuntu C17 101 passed/1 skipped (102 registered), all four sparse tests PASS. Individual trial numbers were not exposed in those logs. Fifth test-only candidate rejects nonfresh/aliased/invalid descriptors and size mismatch before destination mutation, verifies preservation and exposes paired measurement output in existing C17 Release CI. Decision: retain reference; DEFER production adoption pending representative evidence and FS-01B safety review. No sixth PR. Local NOT RUN; CI pending, independent review and integrated-main unverified. No production change, imports, dependencies, spend or release.

**Sparse copy measurement (2026-11-28):** #318 merged at `3922dbad4a326e64370a892a6781c8d0c4006131`; four returned exact-head workflow families passed, Ubuntu in progress at inspection; seven CMake jobs PASS, C17 Release 100 passed/1 skipped, all three sparse tests PASS. Fourth test-only candidate compares four paired trials, alternating order, with fresh distinct temporary files, full byte oracle, monotonic elapsed copy/sync time, submitted bytes and allocated blocks. No speed threshold, physical I/O or production claim. Local NOT RUN; CI pending; independent review and integrated-main unverified. No production change, imports, dependencies, spend or release.

**Sparse-preserving copy reference (2026-11-28):** #317 merged at `54ff9a38cd3c1f9083eeb0194ce5a6bd9d492596`; five returned exact-head workflow families and seven CMake jobs succeeded; C17 Release 99 passed/1 skipped, both sparse references PASS. Independent review and integrated-main not verified. Third test-only candidate copies reported extents to a distinct fresh temporary file and checks all bytes against source, known oracle and buffered reference; reports transferred bytes and allocated blocks, not speed or exclusive physical savings. Local NOT RUN; CI pending. No production mover, source imports, dependencies, spend or release.

**Sparse extent boundaries (2026-11-28):** #316 merged at `17608452d63bb9bd752127b7f0e39dee21542e56`; five returned exact-head workflow families succeeded, CMake seven jobs passed, C17 Release 98 passed/1 skipped (99 registered), sparse reference PASS. Independent review and integrated-main not verified. Second test-only candidate checks empty/EOF/beyond EOF ENXIO, invalid descriptor EBADF, all-zero logical bytes and conservative extent reporting. Unsupported capability or no observable hole is NOT VERIFIED (77). Local NOT RUN; CI pending. No production change, copied code, dependencies, spend or release.

**Sparse extent reference (2026-11-28):** #315 merged at `fb82c8ab64d53e0345c517b8cade6d2104cd7c2f`; four returned PR workflow families passed, independent review not returned. SPARSE-EXTENT-EVAL-01 first candidate: disposable 4 MiB sparse and buffered fixtures, two marker bytes, full logical byte oracle, allocated-block observation, bounded SEEK_DATA/SEEK_HOLE traversal and explicit capability skip. No performance, physical savings or production copier claim. Local NOT RUN; CI pending. FS-01B-FIX/REL-01 remain blocked.

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

**Owner-approved Linux-inspired library architecture (2026-11-27):** [Engineering requirements](docs/architecture/LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). Target is Linux-inspired user-space systems engineering, not copied kernel code or certified kernel grade. Private .a research precedes gated production static/shared libraries and Rust FFI.

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

> **Autopilot governance (23 Nov 2026):** owner allows safe reviewed-by-agent PR merges with **zero mandatory human approvals**, but not bypassing CI, protection or FS-01B/release restrictions. [Policy and post-merge docs workflow](docs/AUTOPILOT_ZERO_REVIEW_AND_DOC_SYNC.md) · [GitHub importable main ruleset](.github/rulesets/autopilot-main-no-human-review.json). Committing JSON does not activate the ruleset.

> **Configuration read-error safety (23 Nov 2026):** CONFIG-READ-ERROR-01 rejects stream read/close failures instead of accepting partial configuration; disposable Linux regression, pending exact-head CI. No destructive operations or release.

> **Configuration validation (23 Nov 2026):** scoped CONFIG-EXT-KEY-01 work rejects missing extension names (`ext.=` / `ext..=`), with disposable negative tests. PR-head CI and merge status must be checked live; this does not authorize destructive source retirement or release.

A Linux/POSIX file-organization tool with a **pure-C production engine** and isolated Rust **test-support** tooling. It scans a directory, classifies files using configurable extension rules, and processes work through a bounded worker pipeline. File moves are protected by a journal-backed transaction flow so interrupted operations can be examined and recovered conservatively.

> **Project status:** Active development; not certified for production or valuable unbacked-up data. The [FS-01B test](docs/quality/FS_01B_RETIREMENT_RACE_CHARACTERIZATION.md) reproduced a **P0 source-retirement check/use risk**: a substituted name can be unlinked after a prior inode check. The [proposed FS-01B-GUARD](docs/security/FS_01B_SHARED_SOURCE_GUARD.md) refuses moves from group/other-writable or non-owned input directories to reduce other-UID exposure, but **does not fix same-UID interference, arbitrary malicious writers, or power-loss recovery**. If merged, previously accepted shared-writable directories will receive `EPERM` instead of being moved. Require verified backups before any real move.

## Feature progression and verification (23 November 2026)

[PR #148](https://github.com/Rofikali/Messy-File-Cleaner/pull/148) merged at `3b32efbe8fad2047f3588fa51c907525a4e4dca4`, rejecting overlong physical configuration lines. Its tested PR head `36383fd2ca31cba9489a7e88b530b29b1fd6298d` passed CMake, Docker, Makefile and Ubuntu 24.04 workflow families. PR #150 subsequently merged trailing-operand rejection for `ext.*` mappings at `74c0639e9656eb37339e96cdc9ed7d312682b3f0`; a new safe feature requires live selection and ledger reconciliation; see [live board](next.md) and [feature budget](docs/feature-budget.json). These CI results do not establish manual Ubuntu/Docker Desktop/Compose, real-filesystem, backup or power-loss acceptance. FS-01B P0 and REL-01 remain blocked; no release is authorized.

## Opt-in dry-run JSON summary (PR #145, merged; aggregate-only)

Proposed usage: `file-cleaner --root /path/to/disposable-input --dry-run --once --report-json`. This emits one JSON line on standard output with `schema_version: 1`, `kind: dry_run_summary`, `complete`, `events_seen`, `events_enqueued`, `previewed_moves`, and `move_failures`. Existing human-readable output remains; consumers must select the JSON line, not parse all stdout as JSON. No filenames or paths appear in the JSON record. This is an aggregate diagnostic, **not** a deterministic per-file operation plan, collision/conflict inventory, or permission to execute. `--report-json` without dry-run or with watch mode is rejected. Feature is **merged but only aggregate-level**, not released; all FS-01B and REL-01 restrictions remain.

## Project roadmap, current status, and engineering docs

- **How documentation stays current with each code/CI milestone:** [milestone documentation transaction](docs/MILESTONE_DOCUMENTATION_WORKFLOW.md), [PR checklist](.github/pull_request_template.md), and [historical archive](next-history.md). Existing PR and CI state must be verified live; the compact board is a dated checkpoint, not a GitHub API feed.
- **What is NOW / NEXT / LATER:** [`next.md`](next.md) — live board of actionable milestones, gates and precise PR/CI evidence; older snapshots are retained under a clearly marked historical section.
- **What advanced features might come after current milestones:** [Advanced C17 filesystem/SDK/desktop roadmap](docs/ADVANCED_FEATURE_ROADMAP.md) and [security, verification, seven-level engineering and business acceptance gates](docs/ADVANCED_FEATURE_ACCEPTANCE.md). These do not certify unbuilt capabilities.
- **What is finished, half implemented, started, test-only, blocked or not started:** [Component completion and status inventory](docs/PROJECT_COMPLETION_AND_STATUS_INVENTORY.md).
- **Where to find authoritative docs:** [Documentation map](docs/README.md).
- **How seven engineering levels, security, CTO, CA/MBA and management review changes:** [Operating model](docs/ENGINEERING_OPERATING_MODEL.md).
- **CI/CD and supported Make targets:** [CI/CD and Makefile guide](docs/CI_CD_AND_MAKEFILE_OPERATIONS.md).
- **How an agent resumes and selects the next small PR:** [Continuation/handoff runbook](docs/AGENT_CONTINUATION_AND_HANDOFF.md).
- **Product direction and free/paid/SDK hypotheses:** [Product and business strategy](docs/PRODUCT_AND_BUSINESS_STRATEGY.md).

**Technology status:** The shipped application remains **C17**; C11 and C23 are compatibility-validation targets. PRs #81 and #82 approved/merged **isolated Rust test-support** for future parser verification. That Rust crate does not call C yet and is not part of the executable. No public `.a`/`.so` SDK, production Rust FFI, FCJ4 production recovery, SaaS/API, GUI, pricing, or automatic deployment is implemented by this documentation.

## Engineering direction

The project is intentionally a **modular C application (modular monolith)**, not a microservices system.

Its architecture combines:

- **Pipeline / producer-consumer processing** for scanning, dispatching, classification, queues, and worker execution.
- **A journal-backed state machine** for tracking file-move progress and supporting recovery after interruption.
- **Event-style worker notifications** to wake workers and coordinate shutdown.
- **Explicit filesystem boundaries** so risky operations can be tested with deterministic failure injection.

This is event-driven in selected places, but it is **not a full distributed Event-Driven Architecture (EDA)**. There is no message broker, distributed event bus, or eventual-consistency layer. Those would add operational complexity without a demonstrated need for this local file-management application.

### Journal states are transaction records, not a message-broker event stream

The journal records the durable progress of a move. State names describe the operation's recorded state:

| Record | Meaning in the move/recovery protocol |
|---|---|
| `STARTED` | The operation has been recorded as begun. |
| `PUBLISHED` | The destination has been published. |
| `COMPLETED` | The move operation has reached its completed state. |
| `FAILED` | The operation has recorded a failure outcome. |
| `RECOVERED` | Recovery has recorded that it handled the interrupted operation. |

The journal is evidence used by recovery; it is not merely a log for display. Recovery must follow the **last complete, valid record** and inspect filesystem state conservatively. A failed or missing transition must never be treated as if it had succeeded.

Important safety rules:

- Do not overwrite an existing destination.
- Do not retire the source before the destination has been safely copied and published.
- If an error occurs after publication, retain the complete destination; do not perform a destructive rollback just to make the operation appear atomic.
- If the journal contains a partial or ambiguous tail, fail closed rather than guessing or silently truncating evidence.
- Recovery should be repeatable where the recorded state permits it. An uncertain outcome must remain explicit.

The state names are implementation-level protocol details. If the protocol changes, update the journal format, recovery rules, tests, and documentation together.

## Current design map

| Area | Responsibility |
|---|---|
| CLI / application context | Parse options, validate roots and outputs, initialize and shut down the application |
| Configuration | Load worker settings and extension-to-folder rules |
| Scanner | Discover filesystem entries under the selected root |
| Classifier | Map files to configured destination categories |
| Queue / worker pool | Dispatch work, apply backpressure, notify workers, and coordinate shutdown |
| Mover | Copy and publish files while preserving data on failure |
| Journal | Record move progress and drive conservative recovery |
| Watcher | Feed filesystem changes into the processing pipeline in watch mode |

The production source list is explicit in `CMakeLists.txt`. Placeholder source files are not proof that a feature is implemented and are not automatically included in the executable.

## Architecture decisions

- **Production language:** pure C; an accepted Rust test-only pilot exists outside production. C++ is not part of this project.
- **Production baseline:** C17.
- **Compatibility validation:** C11 and C23 are tested as separate build targets; C23 is not the minimum production requirement.
- **Structure:** modular monolith with narrow module contracts.
- **Concurrency:** bounded queues and a worker pool; preserve each queue's documented producer/consumer topology.
- **Filesystem operations:** use explicit error handling and conservative recovery. Treat paths and filesystem entries as untrusted.
- **Testing:** unit, integration, end-to-end, fault-injection, sanitizer, container, and static-analysis checks where configured.
- **Performance:** measure before optimizing. Filesystem throughput depends on storage, filesystem metadata, cache state, file sizes, and worker count.

Do not add a broker, database, microservices, C++, **production Rust**, or assembly merely to make the architecture look more advanced. The accepted, isolated Rust test-support pilot remains separate from the C production executable. Add a new technology only when a concrete requirement and an engineering review justify it. Assembly, if ever needed, must be isolated behind a C interface, supported by a portable C fallback, and justified by repeatable profiling.

## Requirements

The canonical development environment is Ubuntu 24.04 LTS.

You need:

- GCC or Clang with C11/C17/C23 support for the selected validation target
- CMake 3.21 or newer
- POSIX/Linux development environment
- pthreads
- Python 3 for the performance/fixture test helpers

Docker builds use a multi-stage Ubuntu 24.04 image.

## Build

Clone the repository and build with CMake:

```sh
git clone https://github.com/Rofikali/Messy-File-Cleaner.git
cd Messy-File-Cleaner

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DFC_C_STANDARD=17
cmake --build build --parallel
```

For a debug build with the configured AddressSanitizer and UndefinedBehaviorSanitizer flags:

```sh
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DFC_C_STANDARD=17
cmake --build build-debug --parallel
```

To validate the language-standard matrix locally, configure separate build directories:

```sh
for standard in 11 17 23; do
  cmake -S . -B "build-c${standard}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=ON \
    -DFC_C_STANDARD="${standard}"
  cmake --build "build-c${standard}" --parallel
  ctest --test-dir "build-c${standard}" --output-on-failure
done
```

Run the test suite:

```sh
ctest --test-dir build --output-on-failure
```

## Run

The CLI accepts either a positional root or `--root`:

```sh
./build/file-cleaner /path/to/input --dry-run --once
```

Useful options:

| Option | Purpose |
|---|---|
| `<root>` or `--root <root>` | Directory to scan |
| `--config <file>` | Configuration file (defaults to `configs/default.conf`) |
| `--dry-run` | Process and report planned moves without opening the move journal or moving files |
| `--once` | Run one batch scan |
| `--watch` | Run in watcher mode |
| `--workers <n>` | Override the configured worker count |
| `--force` | Explicitly bypass the dangerous-root guard; it does **not** bypass path and filesystem safety validation |

When using Docker Compose, the default `compose.yaml` also mounts `/input` read-only; real moves require the separate **explicit** `compose.execute.yaml` overlay described in [the Docker runbook](docs/docker/DOCKER-RUNBOOK.md). Merely overriding the default command does not make the input writable.

Start with `--dry-run` on a disposable test directory. Inspect the output and configuration before running a real move. Do not point the tool at system directories, home directories, mounted data, or valuable files unless you understand the consequences and have a verified backup.

### Configuration

The default configuration is `configs/default.conf`. It currently selects batch mode, configures worker settings, and maps extensions to destination folders. For example:

```ini
mode=batch
worker_threads=2
batch_size=64
idle_sleep_ms=1

ext.jpg=Jpeg/
ext.jpeg=Jpeg/
ext.png=Png/
ext.html=Html/
```

Use a separate test configuration and disposable input/output directories when experimenting with new rules.

## FCJ4 journal development status

FCJ4 is a separate journal protocol under staged development. The repository contains a pure in-memory parser/replay implementation, an isolated canonical serializer/writer-state module, and a Linux-only test append adapter. These components are test-only, are not linked into the production executable, are not exposed through the CLI, and do not read or write `journal/move.log` or `journal/move.fcj4`.

The writer-state module owns BEGIN/SEALED/ACTION_DEF evidence, performs pre-prepare cross-record checks, imports replay state with deep-copied strings, restores exact MOVE/ACTION sequences, and enforces record-limit accounting. The separate test adapter operates only on a caller-supplied descriptor for a disposable private temporary file. It uses explicit-offset `pwrite()`, rejects `O_APPEND`, takes a non-blocking advisory `flock()`, checks file identity/size/permissions, and requires successful `fsync()` followed by an explicit in-memory writer-commit acknowledgement before another append. Fault-injection tests cover short writes, EINTR, partial-write and fsync uncertainty, commit reconciliation, cleanup failures, and process-exit replay.

This adapter does **not** open or create the application journal path, and it does not provide production durability, recovery planning, or filesystem mutation. Process-exit tests are not power-loss tests. A successful in-memory commit alone is not proof that bytes were durably written.

A valid FCJ4 replay proves only that the supplied byte sequence satisfies the parser's structural and transition rules. It does not prove journal authenticity, current filesystem state, or permission to execute recovery. Do not manually create, edit, or use FCJ4 records to recover files.

FCJ2/legacy production journal behavior remains separate. FCJ3 remains diagnostic/test-only and must not be mixed into the production FCJ2/legacy journal.

## Data safety and recovery

File movement is more complicated than copying bytes. It involves temporary files, publication, directory metadata, synchronization, source retirement, and journal updates. A process can fail between any two of these steps.

The implementation and tests therefore distinguish failures before publication from failures after publication and from failures during source retirement. In particular, a failure after a destination has been published can leave both source and destination present; this is safer than deleting the only known-good copy.

The journal is stored at the configured application-relative journal location (`journal/move.log` in the current implementation). Keep it with the application state and do not manually edit or delete it while operations may be in progress. If recovery reports an error, stop and preserve the source, destination, and journal for investigation.

No README can guarantee safety on every filesystem, kernel, mount configuration, storage device, or power-loss scenario. Durability depends on correct system-call ordering and the guarantees of the underlying platform and hardware.

## Validation and quality

The repository includes tests for configuration/path safety, scanning, queue behavior, worker lifecycle, watcher integration, moving, journaling/recovery, CLI end-to-end behavior, and benchmark smoke paths.

The quality workflow is intended to validate:

- C11, C17, and C23 builds in Debug and Release configurations
- Docker build and runtime smoke behavior
- Ubuntu 24.04 build/test behavior
- Static analysis and security-relevant failure paths
- Deterministic filesystem and journal failure injection

A passing test suite is evidence for the tested cases, not proof that every crash, filesystem, or hardware failure is handled. Read the CI results for the exact commit before relying on a validation claim.

## Performance philosophy

Correctness and data preservation come before speed.

Benchmark representative workloads: many small files, large files, deep directory trees, many directories, warm and cold cache cases, and cross-filesystem moves. Report the environment, worker count, filesystem, storage, compiler, build flags, number of repetitions, median and tail latency, throughput, and memory use.

Do not describe end-to-end file moves as nanosecond operations. Nanosecond-scale measurements may be relevant to small CPU-bound operations, but disk and filesystem work must be measured in realistic units and distributions.

## Security principles

- Treat paths, configuration values, filenames, and watcher events as untrusted input.
- Validate the input root and all output paths; reject dangerous overlap and symlink cases.
- Use least privilege. Do not run the container as root.
- Do not mount the Docker socket or host root into the container.
- Never truncate or overwrite an existing user destination.
- Check system-call results and preserve useful error information.
- Keep test fault-injection controls out of production builds.
- Never commit credentials, private user files, runtime journals, or benchmark artifacts.

## CI and release status

The project has GitHub workflows for CMake Debug/Release and C11/C17/C23 tests, Makefile validation, Ubuntu 24.04 containers, Docker smoke, Rust test support and conditional FCJ4 TSan; manual large-scale/performance benchmark workflows are separate. **These are build/test workflows, not an automatic software deployment or production release.** Use [CI/CD and Makefile operations](docs/CI_CD_AND_MAKEFILE_OPERATIONS.md) for actual triggers and evidence lanes. The existing `Makefile` remains unchanged by the documentation-alignment milestone; proposed `status`, `self-check` and `rust-check` targets are **future reviewed changes**, not current commands.

## Contributing

Read [AGENTS.md](AGENTS.md) for binding rules and [skills.md](skills.md) for the contributor workflow; [SKILLS.md](SKILLS.md) is a non-binding engineering knowledge catalogue. Check the current [work register](next.md) and [documentation index](docs/README.md) before choosing a new milestone.

Every change should:

1. State the problem and expected behavior.
2. Define module ownership, pointer lifetime, thread-safety, error, and cancellation contracts where relevant.
3. Include focused tests, including failure-path tests for filesystem or journal changes.
4. Update documentation when behavior or protocol semantics change.
5. Run the relevant build, test, sanitizer, and static-analysis checks.
6. Report exactly what was run, what passed, and what remains unverified.

Keep changes small enough to review. Never weaken a safety invariant to make a benchmark or CI job pass.

## License

No license is declared in this README. Check the repository's license file and ownership terms before redistributing or incorporating this project.


Configuration validation milestone: `CONFIG-MODE-MISSING-01` rejects a bare `mode` key rather than silently accepting the batch default. This does not change valid mode values, filesystem preservation guarantees, or release readiness. FS-01B and REL-01 remain blocked.


Configuration validation: `CONFIG-EXT-MISSING-01` rejects missing extension destination mappings rather than silently ignoring routing configuration. No filesystem retirement or release authorization is implied.


> **CONFIG-ATOMIC-LOAD-01 (23 Nov 2026, proposed PR):** C17 configuration loading now stages scalar settings and caller-owned extension map entries, committing only after successful parse/read/close. Failed loads retain prior caller state; disposable late-failure regression added. This is not a release or filesystem-retirement safety approval. Exact-head CI and manual acceptance remain NOT RUN pending PR verification. FS-01B and REL-01 remain BLOCKED.

> **CONFIG-KEY-WIDTH-01 (24 Nov 2026, proposed PR):** Configuration parsing rejects keys of 64 bytes or more instead of silently ignoring a truncated key. This is a validation hardening change, not release approval; see `next.md` and `docs/feature-budget.json` for CI and milestone status.
