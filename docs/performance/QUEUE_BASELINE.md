# QUEUE-UPSTREAM-EVAL-01 — Bounded in-memory baseline

Baseline repository main 8e9ca9eaec9cf755b7afaae8fd9b89bfc2c3ed6a. [Linux 6.12 circular-buffer documentation](https://docs.kernel.org/6.12/core-api/circular-buffers.html) was consulted in the initial upstream audit. Existing C acquire/release atomics and fixed 1024-slot ring are retained. Do not assume replacing constant modulo with a mask improves machine code; compilers may already optimize it. No upstream source imported or dependency added.

## Measurement
Test-only queue-benchmark executable links the actual src/core/lf_queue.c and current headers. 256 prepared events with distinct path contents and alternating types; each sample runs 256 batches (65,536 enqueue/dequeue pairs), three samples with one untimed warmup batch per sample. Every dequeue compares full path bytes and type, each batch verifies empty; return nonzero on any mismatch, queue failure or failed/nonpositive monotonic time. Reports elapsed ns, mean ns per pair and pairs/sec for each sample. The timed region includes comparison, loop and empty checks. It is not concurrent transfer latency or filesystem throughput, and does not measure cold-cache behavior, tail latency or individual atomic instruction cost. No timing pass threshold. Fixed bounded workload; no filesystem fixtures, affinity, privileged operations, new CI job or manual benchmark dispatch.

## Reproduction
In an existing disposable build directory or local checkout:
```sh
cmake -S . -B build-queue-baseline -DCMAKE_BUILD_TYPE=Release -DFC_C_STANDARD=17 -DBUILD_TESTING=ON
cmake --build build-queue-baseline --target queue-benchmark
ctest --test-dir build-queue-baseline -R '^queue_benchmark_smoke$' -V
```
Record exact commit/build flags, compiler version, CPU/kernel, cache/warmup policy and all three sample values before interpreting results. Debug and Release outputs are not directly comparable. CTest smoke runs in existing C11/17/23 build matrix; 15s timeout is a test safety bound, not performance acceptance. Local tests NOT RUN at PR creation; no measured speedup yet. Future candidate comparison must use same build/environment, repeat measurements, preserve functional/security regression coverage, and justify any cache/layout or memory-order change. BENCH-SCALE-01 remains PARTIAL 5/5; this distinct in-memory baseline does not reopen scanner scale work.

## Observed standalone local C17 evidence (25 November 2026)

Four retrieved file contents were reconstructed in an isolated container and verified against the exact Git blob IDs from PR head 4ee5e83b5d2117af4d435e62559b815370084126 before compiling:
- src/core/lf_queue.c: b26965dee6c8cd560e16725224473254d398ef78
- include/core/lf_queue.h: f0d50072239fe6b11f29ae1ef85a2dccfd0f4fd7
- include/core/event.h: e0813324e05b8968e2a1cff2f8a60cb6a812f0cf
- tests/performance/queue_benchmark.c: e19543d9b586f38dd372b5d6a55d82ceb02c680c

GCC 14.2.0 (Debian 14.2.0-19), Linux 6.18.57 x86_64, AMD EPYC 9V74 80-Core Processor reported by lscpu (not dedicated hardware). Direct compile: gcc -std=c17 -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2 -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -Iinclude tests/performance/queue_benchmark.c src/core/lf_queue.c -Wl,-z,relro,-z,now -o queue-benchmark. Exit success; all 196,608 timed pairs verified; 1028 bytes/event. No CPU pinning; one warmup batch per sample. This is a narrow standalone run, not full checkout/CMake/CTest, CI, concurrent transfer or scanner performance evidence.

| Sample | Elapsed ns | ns/pair | Pairs/second |
|---|---:|---:|---:|
| 1 | 4094351 | 62.475 | 16006444.001 |
| 2 | 4435456 | 67.680 | 14775481.935 |
| 3 | 3887228 | 59.314 | 16859314.658 |

Median of three samples: 62.475 ns/pair, about 16.0 million verified pairs/s in this specific sequential workload. Full local tests NOT RUN. No comparative speedup claimed; shared container scheduling and cache effects limit generalization.

Compiler probe: gcc -std=c17 -O3 -Iinclude -S src/core/lf_queue.c -o lf_queue.s; grep for x86 integer div/idiv mnemonics found none. This supports keeping the existing constant modulo in this compiler/architecture rather than copying a kernel mask algorithm; it does not establish equivalence or performance on other compilers or architectures. Raw machine artifacts not committed. No new costs or real filesystem workloads.

## Review
L1–L7: bounded original test harness, existing production/ABI unchanged. CISSP/Linux/SRE: no real files, no privileges, no changed concurrency contract. CCIE/network N/A. CA/MBA/management: small existing CI workload, no new service or spend authorized, no assumed savings. License/import lock unchanged; FS-01B/REL-01 remain blocked. Rollback by revert.

## Concurrent SPSC baseline — second PR under the same feature

Baseline main e349649a4d3ca57a2006d9864fc1619709ecb13e; prior sequential PR #206 merged with 14/14 exact-head checks. Test-only queue-concurrent-benchmark uses existing LFQueue, one producer (main) and one consumer pthread, 65,536 unique verified transfers per sample and three samples. Synthetic path bytes include sequence after NUL and alternating types. Full payload compared; peer failure atomic flag and monotonic five-second retry deadline checked every 256 retries, with 25s CTest process timeout. Deadline is cooperative while retrying, not a guaranteed bound on scheduling or a running syscall. Timing includes event creation, copy, verification, yield and thread startup/join. No affinity, privileges, files, changed queue layout or memory order. No direct comparison with sequential benchmark, no per-event latency distribution or speedup claim. Run existing Release CMake build with target queue-concurrent-benchmark and ctest -R '^queue_concurrent_benchmark_smoke$' -V. Local execution NOT RUN at creation. Existing Linux 6.12 circular-buffer reference, no new code import. No new CI job or manual benchmark dispatch. FS-01B/REL-01 and import lock remain blocked.

### Observed standalone concurrent C17 evidence — 25 November 2026

Reconstructed four files in a disposable local container and verified exact Git blob IDs from c753645b692eedbe38f8d1c5869f18721b3dc367: queue_concurrent_benchmark.c 49e532256fd1756c8df82f667d8dce459db76826; src/core/lf_queue.c b26965dee6c8cd560e16725224473254d398ef78; include/core/lf_queue.h f0d50072239fe6b11f29ae1ef85a2dccfd0f4fd7; include/core/event.h e0813324e05b8968e2a1cff2f8a60cb6a812f0cf. GCC 14.2.0, Linux 6.18.57 x86_64, AMD EPYC 9V74 reported by lscpu, shared container, no affinity. Command: gcc -std=c17 -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2 -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -Iinclude tests/performance/queue_concurrent_benchmark.c src/core/lf_queue.c -pthread -Wl,-z,relro,-z,now -o queue-concurrent-benchmark; timeout 25 ./queue-concurrent-benchmark. Exit zero; 196,608 verified unique events, 1028 bytes/event.

| Sample | Elapsed ns | Mean ns/transfer | Events/second |
|---|---:|---:|---:|
| 1 | 46466949 | 709.029 | 1410378.805 |
| 2 | 12885785 | 196.621 | 5085914.440 |
| 3 | 50328850 | 767.957 | 1302155.722 |

Median sample 709.029 ns/transfer, 1.410 million events/s. Large observed variability (1.30–5.09 million events/s); three shared-container samples are not stable capacity evidence or tail latency. Sequential #206 uses a different workload and is not a speedup comparison. Full local CMake/CTest, TSan of this harness, target-host/ARM64, CPU affinity and cache-layout comparison NOT RUN. No production changes or optimization claims.

## Cache-layout evaluation — third PR in QUEUE-UPSTREAM-EVAL-01

Baseline f5bc93f900056c63dc8b3fdb2e4671ceddff26a2; #207 merged, 14/14 exact-head and integrated-main checks. [Linux 6.11 false-sharing documentation](https://docs.kernel.org/6.11/kernel-hacking/false-sharing.html) motivates measuring shared cache-line effects and memory tradeoffs, not copying kernel code. The existing head/tail are adjacent; actual cache-line placement depends on object alignment. CMake creates a private build-directory header by changing only the two live index declarations to _Alignas(64), then compiles the exact same queue source and concurrent harness in a separate test-only executable. Reconfigure when original header changes; fail if anchor disappears. The candidate is never used by production or public ABI. It assumes a 64-byte experimental separation, not universal hardware cache-line size. Both targets print type size/alignment, offsets and address residues (not full addresses); candidate additionally checks its actual index separation. Both retain full sequence/payload verification, retry bounds, same optimization/hardening, and 25s CTest limits. No timing pass threshold. CTest smoke does not prove false sharing or speedup. No hardware counter measurement, production layout change, license unlock, new dependency, benchmark dispatch or spending. Run both Release targets and alternate process order in repeated comparisons on same host; report all samples and memory overhead. Full local CMake/CTest NOT RUN at creation. No candidate promotion without repeatable representative benefit, weak-memory and ABI/allocation review, supported-platform validation.

### Observed local comparison — 25 November 2026

Standalone build from four exact Git blobs at PR head 87936057f7a8da6ad336848466263be9a80d856d: queue_concurrent_benchmark.c d1ea73d178d2a251b18313ddf0b4b2b6e34ea23e; src/core/lf_queue.c b26965dee6c8cd560e16725224473254d398ef78; include/core/lf_queue.h f0d50072239fe6b11f29ae1ef85a2dccfd0f4fd7; include/core/event.h e0813324e05b8968e2a1cff2f8a60cb6a812f0cf. Candidate header generated locally by the identical two-declaration substitution; full CMake/CTest not run. GCC 14.2.0, Linux 6.18.57 x86_64, AMD EPYC 9V74, shared container, 5 available logical CPUs, CPU0 cache line reported 64 bytes; no affinity or hardware counter measurements. Both compiled with -std=c17 -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2 -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -pthread -Wl,-z,relro,-z,now; candidate adds -DFC_QUEUE_PADDED_EXPERIMENT and private header include before production include. Each process ran under timeout 25, three samples of 65,536 verified unique transfers. Process order baseline, padded, padded, baseline, baseline, padded; all six exited successfully. 589,824 transfers per layout, 1,179,648 total. No real files.

| Layout | Size bytes | Alignment | Head offset | Tail offset | Actual head/tail address mod 64 |
|---|---:|---:|---:|---:|---|
| Original | 1052680 | 4 | 1052672 | 1052676 | 32/36 |
| Padded | 1052800 | 64 | 1052672 | 1052736 | 0/0 |

| Layout | All nine mean ns/transfer samples, in order | Median ns/transfer | Range ns/transfer |
|---|---|---:|---|
| Original | 232.263, 215.547, 218.447, 231.868, 222.729, 212.875, 191.365, 126.445, 194.155 | 215.547 | 126.445–232.263 |
| Padded | 219.964, 231.505, 222.609, 197.591, 255.677, 201.139, 228.587, 126.979, 211.106 | 219.964 | 126.979–255.677 |

Observed median time +2.05% for padded (throughput -2.01%; 4.64M vs 4.55M transfers/s); additional 120 bytes per queue, +0.0114% size. Overlapping ranges and uncontrolled scheduler/cache placement make the observed difference inconclusive, not proof of a regression or no false sharing. Retain original production layout: no demonstrated repeatable benefit sufficient to justify changed alignment/ABI/allocation constraints. No cache-line universal assumption or performance improvement claim; full local CMake/CTest, candidate sanitizer/TSan, target host and ARM64 tests NOT RUN. Compare representative pinned-core workloads and counters only if independently justified within remaining feature budget. No imported code, spending or scale dispatch.

## Controlled affinity evaluation — fourth PR in QUEUE-UPSTREAM-EVAL-01

Baseline 742e33710b0a9d6eabbb9f699d83125808a60602; #208 merged. Linux sched_setaffinity/sched_getaffinity interfaces allow test-process thread pinning. Optional positional arguments PRODUCER_CPU CONSUMER_CPU must be unsigned decimal distinct IDs within CPU_SETSIZE and original allowed mask. Main thread pins itself before timing; consumer pins itself after creation; each verifies the exact singleton mask. Consumer pin failure signals peer and fails sample. No arguments preserves default unpinned CI. No privileges, changes to parent/system affinity, production queue layout or memory order. CTest negative cases for both binaries cover negative, duplicate, missing and overflow arguments. Existing full-payload, deadline and smoke checks remain. Run the two Release executables with the same two allowed CPU IDs, record CPU topology and allowed mask, alternate order and report all samples. Distinct logical IDs may share a physical core; affinity is not isolation. Timing includes consumer pinning/startup. Fixed cpu_set_t only; unsupported larger masks fail explicitly. No performance threshold, imports, additional CI job, manual workflow dispatch or spending. Local execution NOT RUN at creation; exact-head CI required. No promotion without representative evidence. FS-01B/REL-01 and import lock unchanged.

### Observed controlled local comparison — 25 November 2026

Four file contents verified against Git blob IDs from cc48f1295a200e4236ffdbb11939db2e15b47864 before compiling: tests/performance/queue_concurrent_benchmark.c 634cff47d9d9173e0d7fa9e092aa3d2660f278b5; src/core/lf_queue.c b26965dee6c8cd560e16725224473254d398ef78; include/core/lf_queue.h f0d50072239fe6b11f29ae1ef85a2dccfd0f4fd7; include/core/event.h e0813324e05b8968e2a1cff2f8a60cb6a812f0cf. Reused disposable candidate header from #208. GCC 14.2.0, Linux 6.18.57 x86_64, shared container allowed CPUs 0–4; reported CPU0/CPU1 different core IDs on same socket, 64-byte CPU0 cache line. Both standalone targets built with -std=c17 -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2 -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wstrict-prototypes -pthread -Wl,-z,relro,-z,now and original include or private candidate include + FC_QUEUE_PADDED_EXPERIMENT. No full local checkout/CMake/CTest. Each binary rejected seven invalid inputs (negative, duplicate, missing, integer overflow, fixed-mask out-of-range 1024, whitespace, CPU5 outside allowed set), exact exit 1, empty stdout and expected diagnostic; 14/14 passed. Both default runs succeeded (393216 transfers, 6 samples). Pinned runs producer CPU0/consumer CPU1, order original/padded/padded/original/original/padded, 25s external subprocess timeout each, all passed; 589824 transfers/layout, 1179648 pinned total. Parent allowed mask observed unchanged before pinned runs; no system affinity changes. Full payload and singleton masks checked by harness.

| Layout | Nine pinned mean ns/transfer samples in order | Median | Range |
|---|---|---:|---|
| Original | 228.747, 192.801, 193.487, 217.129, 191.462, 195.317, 220.083, 202.771, 188.243 | 195.317 | 188.243–228.747 |
| Padded | 192.006, 186.248, 178.693, 196.895, 186.887, 190.132, 228.153, 192.782, 180.248 | 190.132 | 178.693–228.153 |

Observed median throughput 5.120M vs 5.260M transfers/s (candidate +2.73%, time -2.65%), layout sizes 1052680 vs 1052800 B. Default unpinned samples in same run: original 233.140/198.450/201.520, padded 218.888/237.435/222.735 ns. Small pinned difference, overlapping ranges, unpinned direction reversal and shared scheduler/cache conditions do not justify production alignment/ABI change. Retain original. CPU pinning is not exclusive resource isolation; startup and consumer pinning included, no hardware counters, dedicated host, ARM64 or candidate sanitizer/TSan. Total local verification 1572864 transfers across default and pinned samples plus 14 invalid cases. No speedup claim for production. Close evaluation when acceptance complete rather than spend fifth slot unnecessarily; follow live board and prior safety gates.
