# Performance Engineering Policy

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
