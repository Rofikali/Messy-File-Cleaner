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

## Review
L1–L7: bounded original test harness, existing production/ABI unchanged. CISSP/Linux/SRE: no real files, no privileges, no changed concurrency contract. CCIE/network N/A. CA/MBA/management: small existing CI workload, no new service or spend authorized, no assumed savings. License/import lock unchanged; FS-01B/REL-01 remain blocked. Rollback by revert.
