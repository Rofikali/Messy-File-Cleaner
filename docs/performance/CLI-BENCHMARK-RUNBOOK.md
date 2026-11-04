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
