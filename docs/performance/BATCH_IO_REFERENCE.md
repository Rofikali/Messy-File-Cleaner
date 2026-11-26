# BATCH-IO-EVAL-01 — bounded synchronous vectored I/O reference

Baseline 6b079defdd3ad59a884d9f621245b7993acd45ef. Production copy_fd in src/mover/mover.c uses a 128 KiB scalar buffer with EINTR/partial-write handling, source identity/metadata validation, permissions and fsync; none of that implementation is replaced. Compare three independent content-only models: scalar 32 KiB, scalar 128 KiB and readv/writev with up to four 32 KiB iovecs over the same 128 KiB backing buffer. [POSIX/Linux vectored I/O reference](https://man7.org/linux/man-pages/man2/readv.2.html); no source imports.

Seven fixture sizes 0, 1, 32767, 32768, 32769, 131072, 131089 bytes; 21 samples. Fresh exclusive mkstemp files immediately unlinked, deterministic source bytes, exact input/output positions, source and destination lengths/bytes/EOF checked independently. Vectored writes reconstruct remaining iovecs across segment boundaries after positive partial writes; read/write and readv/writev retry EINTR, reject zero write and out-of-range lengths. Report actual transfer read/write call counts including EOF and EINTR attempts, exclude preparation and verification. Scalar 32 KiB is a small-transfer reference, not production. Scalar 128 KiB is chunk-size-equivalent only, not production copy_fd. No forced partial syscall, timing, cache/CPU/RSS, metadata, durability, cross-filesystem, cancellation, source-retirement or speedup proof. Existing C17 Release verbose reporting adds exact selector; 15-second timeout; local execution NOT RUN at creation; exact-head CI required. No dependency install, source import, production code, ABI, spending, scale dispatch or release. FS-01B/REL-01/license lock unchanged. Test-only rollback; production architecture/security/network/business scope N/A.


## Bounded wall and process CPU comparison — second PR

Baseline 25745200468e5b2ebf01306f87208a451a8b9816; #238 merged, all 14 exact-head PR jobs passed (CMake 45764029466, Ubuntu 45764029455, Docker 45764029462, Makefile 45764029444, FCJ4 TSan 45764029440). C17 Release job 139720502388 reports 66/66 tests and verbose batch_io_reference PASS samples=21. Runtime Linux 7.0.0-1020-azure x86_64, GCC Ubuntu 15.2.0-16ubuntu1, glibc 2.43. Actual read+write call counts (include EOF/retries; exclude preparation and verification):

| Bytes | scalar32k read/write | scalar128k read/write | vector4x32k read/write |
|---:|---:|---:|---:|
| 0 | 1/0 | 1/0 | 1/0 |
| 1 | 2/1 | 2/1 | 2/1 |
| 32767 | 2/1 | 2/1 | 2/1 |
| 32768 | 2/1 | 2/1 | 2/1 |
| 32769 | 3/2 | 2/1 | 2/1 |
| 131072 | 5/4 | 2/1 | 2/1 |
| 131089 | 6/5 | 3/2 | 3/2 |

128 KiB scalar and vectored models have equal call counts in all seven cases; reductions versus small scalar are not production improvements. Integrated-main push workflows 45764187710/45764187827/45764187803/45764187697/45764187690 in progress at inspection; native benchmark push failure 45764186276 unresolved. No dispatch.

Retain the 21 original samples; add 18 timing samples (131072 and 1048593 bytes, three modes, three repetitions). Rotate method order per repetition so each occupies each position once per size. Fresh independent source/destination files per sample; same 128 KiB backing buffer across modes. Measure CLOCK_MONOTONIC and CLOCK_PROCESS_CPUTIME_ID around transfer loop, including syscall counting, iovec construction and EOF; wall interval also includes CPU clock reads, overhead not subtracted. Check nonnegative intervals and report only after complete independent byte/length/offset verification. No timing thresholds. Process CPU time includes system and user CPU, not cycles or isolated kernel overhead. Uncontrolled cache and scheduling; no cache flush, CPU pinning, RSS measurement or forced short-transfer fault injection. Logical static backing bytes=131072, not measured memory peak. Existing verbose CI selector and 15s timeout retained. Local execution NOT RUN at creation; exact-head CI and raw timing evidence required. No production copy_fd equivalence, adoption, latency-tail, throughput, metadata, durability, cross-filesystem, source retirement or speedup proof. No imports, dependencies, ABI changes, spending, dispatch or release. FS-01B/REL-01/license lock unchanged; test-only rollback.


## Controlled partial transfer and failure handling — third PR

Baseline 4ebec8a3c1c72a03a7e61f8dbad909e3e7bd4a87; #239 merged, 13/13 returned exact-head PR jobs passed (CMake 45764451561, Ubuntu 45764451564, Docker 45764451490, Makefile 45764451505). C17 Release job 139721881394 reports 66/66 tests, batch_io_reference PASS samples=21 and batch_io_timing PASS samples=18. Runtime Linux 7.0.0-1020-azure x86_64, GCC Ubuntu 15.2.0-16ubuntu1, glibc 2.43. Raw elapsed/process CPU nanoseconds by round:

| Bytes | Mode | Wall 1/2/3 | CPU 1/2/3 | Wall median | CPU median | Calls |
|---:|---|---|---|---:|---:|---:|
| 131072 | scalar32k | 32479/30927/32909 | 30515/30285/32258 | 32479 | 30515 | 9 |
| 131072 | scalar128k | 24016/23426/22794 | 23345/22714/22163 | 23426 | 22714 | 3 |
| 131072 | vector4x32k | 22995/22945/23836 | 22364/22314/23134 | 22995 | 22364 | 3 |
| 1048593 | scalar32k | 261523/236206/234223 | 260649/235291/233328 | 236206 | 235291 | 67 |
| 1048593 | scalar128k | 176736/176496/170977 | 175842/175602/170074 | 176496 | 175602 | 19 |
| 1048593 | vector4x32k | 173972/177978/173612 | 173078/177114/172737 | 173972 | 173078 | 19 |

Vectored model median elapsed 1.84%/1.43% lower and CPU 1.54%/1.44% lower than scalar128k; same call count and backing bytes. Three repetitions, overlapping samples, uncontrolled cache/scheduling, no durability/production timing. Retain existing production buffered mover. FCJ4 TSan not returned for #239; integrated-main CMake 45764700422 and Docker 45764700434 success; Makefile 45764700459, Ubuntu 45764700324, performance 45764700371 in progress at inspection; native benchmark push failure 45764699048 unresolved.

Add three private anonymous-file fault scenarios via the independent reference write path: (1) injected EINTR before progress, then four deliberately capped real writev calls spanning iovec boundaries (32767/32771/32771/32763 bytes) and complete 131072-byte verification; (2) real 32767-byte prefix then injected zero return, reject with EIO; (3) same prefix then injected EIO, fail without retry. Verify exact attempts, real call counts, committed bytes, output offset and exact prefix, unchanged complete source. Controlled short requests are not actual kernel-induced short returns; EINTR/zero/EIO are injected, not OS faults. Keep original 21+18 cases; timing reference now calls a small shared vector_write helper with NULL trace, so do not assume unchanged timings. No forced kernel pressure, production fault injection, fsync, crash or source retirement proof. No production change, dependencies, imports, spending, dispatch or release. Local execution NOT RUN at creation; exact-head CI required, existing 15s timeout and verbose selector. FS-01B/REL-01/license lock unchanged; test-only rollback.
