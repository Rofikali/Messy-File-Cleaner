# CLI performance benchmark

## Purpose

The script at tests/performance/benchmark_cli.py measures the existing CLI dry-run scan/classification path using disposable files. It includes process startup, recursive traversal, classification, queue/worker work, and diagnostic formatting. It does not measure real file moves, journal durability, or isolated syscall latency.

The benchmark uses Python's monotonic time.perf_counter_ns() clock, two warm-up runs, repeated measurements, and reports elapsed time plus files/second. It writes raw samples to CSV and environment/configuration metadata and percentile summaries to JSON.

## Run a benchmark

~~~bash
make release
python3 tests/performance/benchmark_cli.py \
  --binary build/release/file-cleaner \
  --config configs/default.conf \
  --files 10000 \
  --runs 30 \
  --output benchmark-results/cli-dry-run
~~~

This produces benchmark-results/cli-dry-run.csv and benchmark-results/cli-dry-run.json. Input fixtures are created under a temporary directory and removed automatically. The app runs with --dry-run; no source files are moved.

## Compare results correctly

- Use the same machine, filesystem, build type, compiler, C standard, worker count, and fixture shape.
- Run at least 30 repetitions for useful tail-percentile context; more runs improve p95/p99 stability.
- Compare medians and distributions, not the fastest single sample.
- Keep cold-cache and warm-cache experiments separate. This harness includes warm-up and repeated filesystem cache effects.
- Record the exact commit and environment with results before making code changes.
- Benchmark real moves separately in disposable directories because filesystem type, storage, cross-filesystem copy, and journal fsync materially affect results.
- Nanosecond values are clock units/resolution, not a promise that each file operation takes nanoseconds.

## Assembly/SIMD gate

Do not add handwritten assembly based on intuition. First use profiling to identify a repeatable CPU-bound hotspot. Any optimized implementation must have a portable C fallback, correctness tests, ABI-safe calling conventions, and a measurable end-to-end improvement without weakening filesystem safety or durability.


## Real-move benchmark (disposable only)

The separate tests/performance/benchmark_moves.py harness measures actual batch moves and journal durability. Each repetition gets a fresh temporary workspace; fixture generation is outside the timed interval. The harness verifies that the source tree is empty, all expected files exist in category output directories, and the journal is nonempty before accepting a sample.

~~~bash
make release
python3 tests/performance/benchmark_moves.py \
  --binary build/release/file-cleaner \
  --config configs/default.conf \
  --files 1000 \
  --bytes-per-file 1024 \
  --runs 10 \
  --output benchmark-results/cli-moves
~~~

Or use make benchmark-moves. Adjust MOVE_BENCH_FILES, MOVE_BENCH_BYTES, MOVE_BENCH_RUNS, and MOVE_BENCH_OUTPUT.

This benchmark measures same-filesystem disposable runs on the temporary filesystem: process startup, traversal, file copy/publish, source retirement, and journal durability. It does not model cross-filesystem mounts or per-file latency distributions. The reported amortized ns/file is total batch time divided by file count, not a measured individual-file latency. Use separate controlled experiments for cross-filesystem behavior and different storage/cache policies.


## Automated CI benchmark artifacts

The Performance benchmark artifacts workflow runs on benchmark-related pushes to main and can also be started manually from GitHub Actions. Manual inputs let you set scan fixture size/repetitions and real-move fixture size/repetitions/payload bytes.

The workflow builds a portable C17 Release binary on ubuntu-latest, runs both harnesses, records runner/commit metadata, and uploads CSV/JSON plus environment details as a 30-day artifact. These numbers are a CI-runner baseline, not a substitute for measurements on the target workstation/VPS. Use identical workflow inputs and compare runs from the same runner image when possible.


## Worker-count scaling

Both harnesses accept --workers 1..16. The CI artifact workflow runs worker counts 1, 2, 4, and 8, producing separate scan-workers-N and move-workers-N result files. This helps determine whether extra workers improve throughput or instead increase synchronization/storage contention. Compare runs on the same runner and do not assume the optimal count transfers to a different CPU or filesystem.
