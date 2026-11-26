# UPSTREAM-AUDIT-01 — Initial reference-only module assessment

**Compatibility closure:** #270 13/13 returned jobs; 324 header probes and two explicit capability reports complete; no production adoption. [Public API design](../architecture/C-API-ABI-FFI-ROADMAP.md) keeps kernel internals hidden.

**Platform capability evidence:** [compatibility reference](UPSTREAM_COMPAT_REFERENCE.md) records 324 header probes and explicit reporting of existing Linux/FCJ3 capability fixtures; no production adoption.

**Compiler compatibility:** [reference](UPSTREAM_COMPAT_REFERENCE.md) records #268 hosted Clang sanitizer preflight and Debug 70/70; internal header candidate pending.

**#268 correction:** missing Clang runtime archives prevented container Debug linkage; existing hosted clang-18 candidate with preflight, no new dependencies.

**Compiler compatibility:** [reference](UPSTREAM_COMPAT_REFERENCE.md) records full Clang Release suite 69 passed/1 skipped; Debug sanitizer candidate pending CI.

**Clang compatibility:** [reference](UPSTREAM_COMPAT_REFERENCE.md) records production C11/C17/C23 success; full C17 Release suite candidate pending CI.

**Compiler compatibility:** [reference](UPSTREAM_COMPAT_REFERENCE.md) reuses existing Clang in development image; no imported code or new library.

**Final supply-chain reference:** [acceptance](../security/SUPPLY_CHAIN_REFERENCE.md) enforces literal direct full-SHA action form and tests drift independently of inventory updates; no source import.

**Action pin reference:** [supply-chain reference](../security/SUPPLY_CHAIN_REFERENCE.md) pins existing official action revisions only, no dependency addition or source import.

**Build-input inventory:** [supply-chain reference](../security/SUPPLY_CHAIN_REFERENCE.md) records mutable CI/container inputs and existing dependency-free Rust test support; no source import.

**APT deterministic reference:** [build integrity](../security/SUPPLY_CHAIN_REFERENCE.md) tests shell retry control flow with private stubs, not upstream imports.

**Supply-chain reference:** [build integrity](../security/SUPPLY_CHAIN_REFERENCE.md) responds to observed Ubuntu mirror inconsistency without source imports or new dependencies.

**Final fault reference:** [fault acceptance](../security/UPSTREAM_FAULT_REFERENCE.md) combines real stdio read/close reports and test-only allocation/acquisition instrumentation; no upstream source imported.

**Acquisition fault reference:** [fault acceptance](../security/UPSTREAM_FAULT_REFERENCE.md) uses isolated open/fdopen wrapping to verify ownership before stream acquisition; no external source imported.

**Allocation fault reference:** [fault acceptance](../security/UPSTREAM_FAULT_REFERENCE.md) uses test-target-only malloc/free linker wraps for staged config ownership, no source import.

**Close fault reference:** [fault acceptance](../security/UPSTREAM_FAULT_REFERENCE.md) extends isolated GNU stream callbacks to distinguish EOF and close report, no production modification.

**Fault reference:** [fault acceptance](../security/UPSTREAM_FAULT_REFERENCE.md) uses test-only linker wrapping and GNU libc stream callbacks, not upstream source imports.

**Linux final selftest reference:** [scanner security](../security/FILESYSTEM-SCANNER-SECURITY.md) tests deliberate child-only deepest inode inheritance and rejection oracle; no external source imported.

**Linux deep descriptor reference:** [scanner security](../security/FILESYSTEM-SCANNER-SECURITY.md) evaluates real descriptor lifecycle across scanner frame growth, no external source import.

**Linux scanner abort lifecycle reference:** [scanner security](../security/FILESYSTEM-SCANNER-SECURITY.md) adds real process inheritance plus abort/rescan ownership checks; no upstream source imported.

**Linux scanner exec reference:** [scanner security](../security/FILESYSTEM-SCANNER-SECURITY.md) independently tests descriptor inheritance across actual exec with a positive control; no upstream source imported.

**Linux selftest reference:** [scanner security](../security/FILESYSTEM-SCANNER-SECURITY.md) uses independent Linux/POSIX descriptor lifecycle checks, no kselftest source imported.

**CPU final reference:** [baseline](../performance/CPU_OPT_REFERENCE.md) adds Linux mmap/mprotect guard-page research, no external source, dependency or production change.

**CPU dispatch reference:** [baseline](../performance/CPU_OPT_REFERENCE.md) adds test-only capability intersection and forced portable policy; no imported implementation or ABI change.

**CPU explicit vector reference:** [baseline](../performance/CPU_OPT_REFERENCE.md) uses compiler intrinsics and runtime checks, no copied implementation or dependencies.

**CPU timing reference:** [baseline](../performance/CPU_OPT_REFERENCE.md) compares portable C against libc and requests compiler evidence; no external code import.

**CPU optimization reference:** [baseline](../performance/CPU_OPT_REFERENCE.md) compares original portable byte counting with libc memchr; no imports or production ABI change.

**Cache-layout final candidate:** [baseline](../performance/CACHE_LAYOUT_REFERENCE.md) adds direct sanitizer instrumentation only; no external implementation, ABI or production change.

**Cache-layout capacity reference:** [baseline](../performance/CACHE_LAYOUT_REFERENCE.md) uses trusted owner extents and POSIX readers in test-only research; no external implementation or ABI change.

**Cache-layout safety reference:** [baseline](../performance/CACHE_LAYOUT_REFERENCE.md) evaluates independent packed snapshots with bounded input/metadata checks and owned allocations; no external source or production ABI change.

**Cache-layout timing reference:** [baseline](../performance/CACHE_LAYOUT_REFERENCE.md) compares synthetic fixed snapshots and packed owned snapshots; no repeat of queue padding or production queue/ABI change.

**Cache-layout reference:** [baseline](../performance/CACHE_LAYOUT_REFERENCE.md) is independent read-only packed path/index research; no repeated SPSC padding feature, imported source or production queue ABI change.

**Batch I/O failure comparison:** [baseline](../performance/BATCH_IO_REFERENCE.md) uses installed writev with deliberately shortened requests and test-only injected errors; no production adapter or imported source.

**Batch I/O measured reference:** [baseline](../performance/BATCH_IO_REFERENCE.md) compares installed scalar and vectored APIs, recording equal call counts for 128 KiB backing models and measuring bounded wall/CPU times. No import or production adoption.

**Batch I/O reference:** [baseline](../performance/BATCH_IO_REFERENCE.md) evaluates installed POSIX readv/writev APIs and scalar references; no imported source or production adapter.

**io_uring synchronization/error reference:** [baseline](../performance/LINUX_URING_REFERENCE.md) evaluates installed UAPI mixed completion semantics with private anonymous files. No source imports, liburing install or production adapter.

**io_uring vectored reference:** [baseline](../performance/LINUX_URING_REFERENCE.md) uses installed Linux UAPI with bounded private anonymous files; no upstream code import, liburing install or production adapter.

**io_uring NOP reference:** [baseline](../performance/LINUX_URING_REFERENCE.md) uses bounded raw UAPI shared ring mappings and acquire/release ownership for non-file NOP requests only. No copied source, liburing install or runtime adapter.

**io_uring reference:** [baseline](../performance/LINUX_URING_REFERENCE.md) uses installed Linux UAPI for bounded setup/probe only before optional liburing assessment; no source import or package dependency.

**Linux I/O offset comparison:** [baseline](../performance/LINUX_IO_REFERENCE.md) retains existing production buffered transfer after measured reference results; adds explicit range/EOF semantics using installed libc only.

**Linux I/O measured reference:** [baseline](../performance/LINUX_IO_REFERENCE.md) uses installed libc copy_file_range and independent buffered model, with bounded comparative timing only. No source import or production adoption.

**Linux I/O reference:** [baseline](../performance/LINUX_IO_REFERENCE.md) evaluates installed libc copy_file_range API only; no kernel/GNU source imports or production adoption.

**Watcher lifecycle comparison:** [baseline](../performance/LINUX_WATCH_REFERENCE.md) checks actual inotify descriptor lifecycle and terminal root deletion, without kernel source import or alternate backend.

**Watcher batch comparison:** [baseline](../performance/LINUX_WATCH_REFERENCE.md) uses raw inotify records as inputs to the actual parser and dry-run workers. No kernel source import, production change or alternate event backend.

**Linux watcher reference:** [baseline](../performance/LINUX_WATCH_REFERENCE.md) examines real inotify root invalidation and watch-object identity rather than reimplementing or importing kernel source. Existing terminal policy retained.

**Path fail-closed reference:** [baseline](../performance/LINUX_PATH_REFERENCE.md) characterizes invalid openat2 requests on private fixtures, with explicit errno checks; no source imported, no production fallback.

**Path identity comparison:** [baseline](../performance/LINUX_PATH_REFERENCE.md) distinguishes openat2 resolution constraints from immutable pathname identity using a private rename/replacement fixture; no kernel source imported or production adoption.

**Path resolution candidate:** [baseline](../performance/LINUX_PATH_REFERENCE.md) examines Linux openat2 UAPI confinement relative to openat O_NOFOLLOW; no kernel source imported or production adoption.

**Scanner evidence reporting:** [reference baseline](../performance/SCANNER_REFERENCE_BASELINE.md) exposes bounded reference measurements in existing CI; no external code imported, no scanner replacement.

**Scanner stop comparison:** [reference baseline](../performance/SCANNER_REFERENCE_BASELINE.md) distinguishes native cooperative cancellation and callback errno propagation from nftw's synchronous callback stop. No equivalent security or cancellation guarantee inferred; production scanner retained.

**Scanner descriptor comparison:** [reference baseline](../performance/SCANNER_REFERENCE_BASELINE.md) evaluates libc nftw budgets 1/4/16 against native ancestor retention on bounded fixtures. Descriptor reuse does not establish directory identity or race/cancellation equivalence; retain production scanner.

**Scanner reference:** [baseline](../performance/SCANNER_REFERENCE_BASELINE.md) compares libc nftw physical traversal with actual descriptor-relative scanner on bounded private fixtures. No source import or new production traversal backend. Retain existing scanner identity checks.

**String measured decision:** [extension baseline](../performance/STRING_EXTENSION_BASELINE.md) shows reverse scan benefit in long synthetic paths, shortest paths effectively tied; retain libc production implementation without representative evidence or external imports.

**String boundary evaluation:** [extension baseline](../performance/STRING_EXTENSION_BASELINE.md) adds independent forward-reference routing and deterministic byte-string tests. No GNU/libc code imported; production classifier retained.

**String candidate:** [extension baseline](../performance/STRING_EXTENSION_BASELINE.md) compares libc strrchr-based production classification with independent test-only reverse scanning after strlen. No libc source copied or replacement justified.

**Arena unwind candidate:** [path allocation](../performance/PATH_ALLOCATION_BASELINE.md) adds bounded-capacity and simulated interruption cleanup checks; no upstream imports, scanner or production allocator changes.

**Nested arena evaluation:** [path allocation](../performance/PATH_ALLOCATION_BASELINE.md) models strict LIFO directory paths and transient siblings, not actual scanner callbacks or file I/O. Existing libc allocation retained; no imports.

**Arena candidate:** [path allocation](../performance/PATH_ALLOCATION_BASELINE.md) compares existing libc-backed path joins with independent test-only bounded batch storage. No imported allocator, dependency, filesystem or ABI change; scanner lifetimes not established.

**Memory baseline:** [path allocation](../performance/PATH_ALLOCATION_BASELINE.md) starts with actual libc-backed lexical helper; compare ownership and requested bytes before considering arenas, alternate allocators or external source. Hash-map evaluation closed at 4/5; original capacity retained.

**Collision evaluation:** [baseline](../performance/HASHMAP_BASELINE.md) exercises current bounded probing with intentional 64-slot collisions without copying any hash implementation or importing code.

**Classifier capacity evaluation:** [baseline](../performance/HASHMAP_BASELINE.md) assesses map lookup in actual classifier code without filesystem access or imports. Existing capacity retained pending representative evidence.

**Hash-map capacity evaluation:** [lookup evidence](../performance/HASHMAP_BASELINE.md) compares the current 64-slot production capacity with 128/256 test-only candidates. Reference-only libc/kernel comparisons already documented; no import, different ownership model or automatic resizing.

**HASHMAP-EVAL-01:** [hash-map baseline](../performance/HASHMAP_BASELINE.md) evaluates the existing fixed-capacity linear-probing table against synthetic lookup workloads. Linux kernel hashtable.h uses hlist buckets; libc hsearch has different storage/ownership and update contracts. Reference only, no imported implementation. Queue evaluation closed at 4/5; retain original layout.

**Queue controlled measurement:** [queue baseline](../performance/QUEUE_BASELINE.md) adds optional process-local Linux CPU affinity to both test layouts; no new runtime dependency or production change. Distinct logical CPUs do not imply distinct physical cores; document topology, allowed mask and scheduling limitations.

**Queue cache-layout comparison:** [Linux 6.11 false-sharing documentation](https://docs.kernel.org/6.11/kernel-hacking/false-sharing.html) informs an isolated 64-byte index-alignment experiment, not a copied implementation. [Evidence and constraints](../performance/QUEUE_BASELINE.md); no production or ABI change.

**Queue evaluation continuation:** [Queue baseline](../performance/QUEUE_BASELINE.md) includes separate sequential and concurrent test-only workloads; the latter uses one producer and one consumer, unique sequence bytes and full payload checks. Retain existing ring and C memory ordering. No cache-line padding or kernel import without comparative evidence.

**QUEUE-UPSTREAM-EVAL-01:** [Queue baseline](../performance/QUEUE_BASELINE.md) adds bounded measurements of existing queue operations with payload checking. Linux 6.12 circular-buffer reference already reviewed; no kernel code copied. Measure before changing modulo, cache layout or synchronization.

**Follow-up — SCANNER-SPECIAL-FILE-01:** [Linux inode(7)](https://man7.org/linux/man-pages/man7/inode.7.html) and [stat(2)](https://man7.org/linux/man-pages/man2/fstatat.2.html) describe inode type classification and descriptor-relative metadata. Retain the scanner's existing fstatat/no-follow and S_ISREG/S_ISDIR policy; add independent FIFO/no-writer and bound AF_UNIX pathname-socket regression. No need for a copied VFS implementation, d_type shortcut or new dependency. No device-node or speed testing; no source-retirement guarantee.

Baseline main: 53c2d28c7126e81ddc3a201bc334d6851b5d3b71 (PR #203). Production source inventory from CMakeLists.txt; source reviewed selectively, not a complete code/security audit. No new external dependency, source text, license grant, benchmark or release. Registry remains locked. This assessment is an initial triage; detailed upstream source comparisons and pinned candidate commits remain separate gated evaluations.

## Verified references, 25 November 2026
- Linux kernel **6.12** [Circular buffers](https://docs.kernel.org/6.12/core-api/circular-buffers.html): bounded SPSC rings, capacity distinction, wraparound and publication/consumption ordering. Kernel naming convention differs from our head/tail convention; do not copy kernel barriers or locking assumptions. Versioned documentation reference, not a vendored source SHA.
- Linux man-pages [openat2(2)](https://man7.org/linux/man-pages/man2/openat2.2.html): descriptor-relative path restrictions are relevant to containment; availability and exact flags need runtime compatibility validation. This reference is a live manual, not a pinned source release. No proof of retirement safety.
- Linux man-pages [inotify(7)](https://man7.org/linux/man-pages/man7/inotify.7.html): buffered events and watch identifiers, lost events and reconciliation concerns. Existing watcher remains native; no automatic resync proof.
- GNU Findutils and Coreutils manual retrieval timed out in this run. Their traversal/copy implementations remain unverified candidates, not source reviewed or approved. Other candidates in master plan remain research-only.

## Module decisions and next evidence
| Module / local evidence | Decision | Next acceptance gap |
|---|---|---|
| Queue: src/core/lf_queue.c, include/core/lf_queue.h, tests/unit/test_queue.c | KEEP. Fixed 1024 slots, effective 1023, single producer/consumer, C acquire/release atomics. Already aligns conceptually with Linux circular buffers. Compiler may already optimize constant modulus; no speed benefit established by changing it. | Add full-ring recycle and full payload regression now. Weak-memory verification, cache layout, assembly and benchmark later only if justified. |
| Scanner: src/scanner/scanner.c, test_scanner.c | KEEP. Explicit owned directory frames, first-error and cancellation/resource cleanup; evaluate descriptor-relative openat2 only with compatibility and trust review. | Compare traversal with Findutils after reference verification; actual supported-kernel tests, no source-retirement change. |
| Watcher: src/watcher/watcher.c, test_watcher.c | KEEP native inotify and current overflow/invalidation behavior, not fanotify by default. | Evaluate consistency recovery, watch scope and privilege implications; no real overflow recovery claim. |
| Worker pool: src/core/thread_pool.c, test_worker_lifecycle.c | KEEP pthread/eventfd/poll and caller-serialized lifecycle, independent actual count. | Measure wakeups/latency before new backend; no kernel workqueue import. |
| Hash map: src/utils/hashmap.c | KEEP caller-owned fixed-capacity power-of-two linear probing with bounded loops. | Compare only for measured lookup/collision bottleneck and compatible ownership. |
| Classifier/config/CLI: src/classifier/classifier.c, src/config/config.c, src/main.c | KEEP libc string matching, existing CLI and transactional config contracts. | No replacement justified; individual validation regressions remain eligible. |
| Mover: src/mover/mover.c | BLOCK production backend replacement; read/write/fsync and descriptor-relative operations observed. | FS-01B trust/crash proof required before retirement; Coreutils/copy_file_range reference study only. |
| Journal: src/journal/journal.c | KEEP existing boundary; observed write/fsync/openat. FCJ3/FCJ4 files outside explicit production list. | No new production recovery; research-only durability comparison and isolated tests. |
| Memory, build and ABI: CMakeLists.txt, scanner/queue source | KEEP existing libc/C17 and C11/C23 CI. No new allocator, library or FFI. | Measure RSS/allocations and compiler output before adapter or cache layout changes; reconcile ABI roadmap. |

## Delivered regression
Independent test code exercises a full SPSC ring over 3*QUEUE_SIZE+17 recycle steps: reject insertion when full, dequeue expected event, refill freed slot, drain remaining events. Compare all path-array bytes and both enum values; empty dequeue leaves destination unchanged. Existing concurrent FIFO and basic tests retained. This is not a new production algorithm, weak-memory proof or performance measurement. It uses no upstream source text and has no runtime dependency. Local execution NOT RUN; exact-head CI required. No benchmark dispatch.

## Review
L1–L7: bounded test and baseline triage, not module rewrite. Security/Linux/SRE: existing trust and resource boundaries remain; no kernel privileges or production changes. CCIE/networking N/A (no network runtime). CA/MBA/management: avoid integration and maintenance cost without measured benefit; no cost estimate or new spend. License pending, no legal clearance. Rollback by reverting test/audit PR. Continue reference-only detailed candidate research or safe independent work after this feature; do not reopen exhausted feature budgets. FS-01B/REL-01 BLOCKED.
