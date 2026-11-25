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
