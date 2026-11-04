# CI performance baseline — 2026-11-04

## Scope and reproducibility

These are reference measurements from GitHub Actions, not from the developer's workstation or production VPS.

- Repository commit for worker sweep: 56404f09c6972fd8ddc314d539f8e49718859ef4
- Runner: GitHub-hosted Ubuntu x86_64, 4 allocated CPUs; AMD EPYC 9V74 host CPU
- Kernel: Linux 7.0.0-1017-azure; filesystem reported by stat -f as ext2/ext3
- Build: portable C17 Release
- Clock: Python time.perf_counter_ns() monotonic clock
- Scan workload: 5,000 disposable files, 15 measured repetitions, 2 warmups, dry-run mode
- Move workload: 500 disposable files × 1 KiB, 5 measured repetitions, one warmup; actual moves and journal durability verified
- Output: stdout/stderr redirected to /dev/null for the scan benchmark
- Raw CSV/JSON and runner metadata are in the workflow artifacts linked below.

Artifacts:
- Initial baseline run: [workflow run 41842154887](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/41842154887), commit 19aa113ca1b97eb26b5ba8a733916bd0edf55051
- Worker sweep: [workflow run 41842726297](https://github.com/Rofikali/Messy-File-Cleaner/actions/runs/41842726297), commit 56404f09c6972fd8ddc314d539f8e49718859ef4

Artifacts expire after 30 days. Download them before expiry if long-term raw data retention is required.

## Initial two-worker baseline

| Workload | Median elapsed | Median throughput | Notes |
|---|---:|---:|---|
| Dry-run scan, 5,000 files | 25.75 ms | 194,210 files/s | Process startup + traversal/classification |
| Real move, 500 × 1 KiB | 353.03 ms | 1,416 files/s | Includes copy/publish, source retirement, journal durability |

The initial real-move samples ranged from 302 ms to 1.115 s. This variation is a reminder that storage/cache and runner noise materially affect results.

## Worker-count sweep

All rows below come from the same worker-sweep workflow run. Throughput and elapsed values are batch measurements, not per-file latency.

### Dry-run scan/classification

| Workers | Median elapsed | p95 elapsed | Median files/s | Sample range |
|---:|---:|---:|---:|---:|
| 1 | 17.72 ms | 17.99 ms | 282,197 | 17.36–18.18 ms |
| 2 | 18.69 ms | 30.61 ms | 267,526 | 18.29–30.70 ms |
| 4 | 35.29 ms | 36.02 ms | 141,666 | 34.64–36.12 ms |
| 8 | 40.24 ms | 41.00 ms | 124,252 | 39.68–41.08 ms |

### Real move + journal durability

| Workers | Median elapsed | Median files/s | Sample range |
|---:|---:|---:|---:|
| 1 | 651.38 ms | 768 | 648.66–823.22 ms |
| 2 | 390.95 ms | 1,279 | 356.66–417.04 ms |
| 4 | 330.04 ms | 1,515 | 325.93–333.92 ms |
| 8 | 332.52 ms | 1,504 | 325.18–346.75 ms |

Move percentiles are not emphasized because only five measured repetitions were used. The 4-vs-8 worker difference is small and may not be statistically meaningful.

## Engineering decisions

1. **Do not change the default worker count yet.** The best count differs by workload, and CI hardware/filesystem differ from the target environment.
2. **Do not add handwritten assembly or SIMD yet.** The real-move path includes filesystem copy/publish and journal durability; these results do not identify a CPU instruction hotspot.
3. **Next performance work should be profiling and controlled target-machine measurements**, not speculative micro-optimization.
4. Preserve correctness, no-replace publication, source-retirement safety, and journal durability while optimizing.
5. Repeat the worker sweep with more real-move samples and on the target Linux/Docker filesystem before tuning production defaults.

## Important interpretation

The scan harness includes process startup and per-file diagnostic formatting, even though output is discarded. The real-move harness measures same-filesystem disposable temporary workspaces. Neither benchmark measures cross-filesystem mounts, cold-cache storage behavior in a controlled way, or individual-file latency distributions. Nanoseconds are measurement units; they are not a promise of nanosecond file operations.
