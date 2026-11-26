# Performance Engineering Policy

**io_uring capability:** [baseline](LINUX_URING_REFERENCE.md) reports ring setup and operation availability only, not submitted operations, async throughput or latency.

**Linux I/O range evidence:** [baseline](LINUX_IO_REFERENCE.md) records actual prior transfer samples and adds untimed explicit offset/EOF cases with exact untouched byte verification.

**Linux I/O timing:** [baseline](LINUX_IO_REFERENCE.md) records transfer-loop elapsed ns for three repetitions per mode at two sizes, validates bytes outside timing, and excludes fsync, setup and production mover costs.

**Linux I/O reference:** [baseline](LINUX_IO_REFERENCE.md) validates exact bytes, size, offsets and EOF across six boundary sizes before any timing or integration claim.

**Watcher lifecycle reference:** [baseline](LINUX_WATCH_REFERENCE.md) observes procfs descriptor counts at readiness and joined boundaries, not continuous peak consumption or latency.

**Watcher batch reference:** [baseline](LINUX_WATCH_REFERENCE.md) adds bounded real-record batching with observed read/record counts; no timing, throughput or queue-overflow claim.

**Linux watcher kernel reference:** [baseline](LINUX_WATCH_REFERENCE.md) adds untimed real-event identity and terminal behavior validation, not throughput or event-loss recovery evidence.

**Path error reference:** [baseline](LINUX_PATH_REFERENCE.md) adds ten untimed Linux request/error/recovery comparisons; not a syscall benchmark or a production fallback implementation.

**Path identity reference:** [baseline](LINUX_PATH_REFERENCE.md) adds untimed stable rename/replacement checks, not syscall timing or concurrent race proof.

**Path resolution reference:** [baseline](LINUX_PATH_REFERENCE.md) is an untimed Linux capability/semantics test, not lexical normalization, syscall timing or production speed evidence.

**Scanner CI samples:** [reference baseline](SCANNER_REFERENCE_BASELINE.md) captures all existing scanner reference sample rows and descriptor/stop checks in one C17 Release job; no speed threshold or representative workload claim.

**Scanner early-stop checks:** [reference baseline](SCANNER_REFERENCE_BASELINE.md) keeps stop and rescan validation outside timed traversal measurements; no asynchronous cancellation, race or end-to-end performance claim.

**Scanner FD observation:** [reference baseline](SCANNER_REFERENCE_BASELINE.md) separates procfs callback-visible descriptor high-water from uninstrumented timing and continuous process peak. No system-wide resource-limit change or scale dispatch.

**Scanner reference comparison:** [baseline](SCANNER_REFERENCE_BASELINE.md) separates small bounded exact-set traversal comparison from exhausted BENCH-SCALE-01; no scale dispatch, representative throughput or production backend change.

**String local evidence:** [extension baseline](STRING_EXTENSION_BASELINE.md) records three C17 Release samples per mode/prefix, standalone environment and exact source hashes; no full CMake local run, statistical significance or whole-program speed claim.

**String differential checks:** [extension baseline](STRING_EXTENSION_BASELINE.md) separates exhaustive untimed correctness checks from existing timed classification samples; no speed claim or Unicode normalization.

**String extension comparison:** [baseline](STRING_EXTENSION_BASELINE.md) compares identical synthetic paths and actual 64-slot map, varying prefix length, verifying all classifications. Includes strlen and lookup/verification, no timing threshold or production promotion.

**Failure/interrupt coverage:** [path allocation](PATH_ALLOCATION_BASELINE.md) adds untimed deterministic unwind checks; do not interpret these as latency samples, system allocator failure or production cancellation evidence.

**Nested lifetime workload:** [path allocation](PATH_ALLOCATION_BASELINE.md) compares variable-length nested paths with transient sibling lifetimes; report peak live payload and separately reserved arena bytes, not RSS or scanner throughput.

**Arena comparison:** [measurement](PATH_ALLOCATION_BASELINE.md) includes backing allocation/free in timed samples and reports logical requested bytes, not RSS. Identical bounded batches and verification; alternating order, no threshold or production promotion.

**Path allocation baseline:** [measurement](PATH_ALLOCATION_BASELINE.md) distinguishes requested bytes and live payload from actual allocator overhead/RSS, with bounded owned copies of real lexical path helper outputs. No timing threshold or allocator promotion.

**Collision workload:** [baseline](HASHMAP_BASELINE.md) measures bounded fixed keys sharing slot 63 at 64 slots. Larger capacities reuse the same keys, not necessarily identical initial buckets; no timing threshold or production claim.

**Classifier evaluation:** [hash-map baseline](HASHMAP_BASELINE.md) adds native classifier-plus-map mixed-path samples. Includes extraction and verification, not filesystem or whole application throughput; no timing threshold.

**Controlled hash-map capacity:** [baseline](HASHMAP_BASELINE.md) separates shipped default mapping set from synthetic full-capacity stress, alternates capacity order and reports lookup timings and logical storage. Not a real workload distribution or end-to-end speedup.

**Hash-map evidence:** [baseline](HASHMAP_BASELINE.md) distinguishes hits and misses at synthetic table occupancies; record all samples, size, timing overhead, compiler and key distribution. No scale-benchmark dispatch or production speedup claim.

**Affinity evidence:** [queue baseline](QUEUE_BASELINE.md) supports optional explicit Linux CPU IDs for test-only original and padded layouts; report topology, mask, scheduling and all samples. Pinning is not isolation or a guarantee of repeatability.

**Cache-layout comparison:** [Queue baseline](QUEUE_BASELINE.md) includes an isolated candidate with 64-byte index separation, identical C code and workload. Compare layout, memory overhead and repeated timings; hardware counter evidence and actual target workloads remain separate.

**Concurrent queue measurement:** [queue baseline](QUEUE_BASELINE.md) distinguishes sequential batched pairs from two-thread transfer including event generation, verification, retry/yield and thread lifecycle. No direct speedup comparison between these workloads; no timing threshold or new workflow.

**Queue baseline:** [QUEUE_BASELINE.md](QUEUE_BASELINE.md) separates a fixed-size single-threaded batch enqueue/dequeue-plus-verification measurement from concurrent SPSC or filesystem workloads. No timing gate, speedup claim, extra workflow or scale benchmark dispatch.

**Upstream evaluation:** [Master plan](../upstream/UPSTREAM_MASTER_PLAN.md) requires comparable compiler, kernel, hardware, filesystem, cache, workload and worker count; repeat and report latency/tails, CPU/RSS, descriptors and integrity. Proposed 1k–10m file workloads are not achieved results. BENCH-SCALE-01 remains exhausted; no benchmark dispatch or spending.

## Feature exit and measurement boundary (2026-11-21)

PERF-01 historical disposable benchmark fixture/evidence improvements are **merged through PR #128** and the feature lane is closed to repeated same-feature micro-PRs under the new five-PR policy. Measured capacity, filesystem-specific worst cases, host benchmarks and power-loss behavior are **not inferred** from regression count. Future independent BENCH-SCALE-01 may measure representative workloads only with a bounded, non-destructive plan and reproducible cost/provenance; it receives its own maximum five opened PRs. BENCH-SCALE-01 uses its fifth and final PR slot for report acceptance checks, independent saved-report validation before upload, and compiler version provenance. After the slot is consumed the feature must be reconciled as measured or partial without resetting its ID; new performance claims require actual evidence. No dispatch or performance claim is authorized by the workflow definition. The live selector remains next.md; no new PERF-01-only PRs.

## Objective
Achieve high throughput and predictable latency without risking user data. Do not promise nanosecond end-to-end filesystem operations.

## Cost model
- Same-filesystem rename is metadata work and usually cheaper than copying.
- Cross-filesystem moves read and write file contents; time scales with bytes and storage throughput.
- Traversal incurs metadata operations.
- Watch latency includes kernel delivery, debounce, scheduling, and worker processing.
- CPU micro-optimizations cannot remove storage latency.

## Benchmark dimensions
Test many tiny files, a few large files, deep trees, many directories, same-filesystem rename, cross-filesystem copy, warm/cold cache, and multiple worker counts.

Record CPU, RAM, kernel, filesystem, storage, compiler, flags, container limits, file count, total bytes, and cache condition.

## Metrics
- Wall-clock duration, files/sec, bytes/sec
- CPU time/utilization and peak resident memory
- Syscalls per file using strace -c
- p50/p95/p99 processing latency where measurable
- Failures by errno and count of untouched files

## Optimization order
1. Fix correctness and data-loss risks.
2. Profile representative workloads.
3. Remove unnecessary syscalls and repeated path work.
4. Improve batching and memory reuse.
5. Tune workers and queue behavior.
6. Consider SIMD, custom allocators, or assembly only if profiling proves a CPU bottleneck.

## Security constraints
Never bypass path checks for speed. Do not disable journaling or durability without an explicit documented mode. Keep bounds checks, collision safety, and source-preservation guarantees. Do not use -march=native in generic release binaries. Run correctness and sanitizer builds separately from benchmarks.

## Nanosecond-level measurement

Nanoseconds are a useful unit for timing CPU-bound work, but nanosecond units do not guarantee nanosecond accuracy or imply that filesystem operations complete in nanoseconds. Clock reads, scheduling, cache state, syscalls, storage, and durability all affect results.

- Use CLOCK_MONOTONIC (or an appropriate monotonic high-resolution source) for elapsed time.
- Batch many operations per timed interval to amortize timer overhead.
- Repeat runs; report median and tail percentiles, not only the minimum.
- Separate in-memory classifier/hash/queue microbenchmarks from end-to-end filesystem tests.
- Report filesystem type, mount type, cache condition, and whether journal fsync is enabled.
- Set targets only after the baseline exists. Never remove security or durability guarantees to hit a latency target.

## Profile provenance and generated-artifact privacy

Raw `perf.data`, `gmon.out`, `gprof` dumps and ad hoc local benchmarks may include machine details, process names and filenames. Keep them out of Git, use ignored paths or controlled CI artifacts, and share only sanitized, reproducible summaries. Existing legacy tracked profile files require owner review before any deletion. Read [profiling-artifact retention and provenance](PROFILE_ARTIFACT_RETENTION_AND_PROVENANCE.md).

## Performance claims
Include workload, environment, command, repetitions, and result distribution. Do not compare different filesystems or cache states as equivalent.


## Assembly and architecture-specific optimization

Handwritten assembly is not part of the initial implementation plan. Most file-cleaner cost is expected in filesystem traversal, metadata operations, copying, and journal durability; assembly cannot remove storage or syscall latency. First establish correctness and repeatable profiles. Consider architecture-specific code only when a measured CPU-bound hotspot dominates (for example, a hashing/checksum loop), the expected gain is material, and a portable C implementation remains available as the reference and fallback. Any optimized path must be selected safely for supported CPU features, tested for byte-for-byte equivalent results, benchmarked on representative hardware, and kept out of the generic build unless the evidence justifies it. Never use inline assembly as a substitute for algorithmic improvements or correct memory/concurrency design.
