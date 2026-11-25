# UPSTREAM-AUDIT-01 — Initial reference-only module assessment

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
