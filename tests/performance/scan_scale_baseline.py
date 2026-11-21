#!/usr/bin/env python3
"""Disposable POSIX directory-enumeration baseline; NOT an engine benchmark."""
import argparse
import json
import os
from pathlib import Path
import platform
import resource
import statistics
import tempfile
import time


def measure(root: Path, expected: int, repeats: int):
    samples = []
    for _ in range(repeats):
        start = time.perf_counter_ns()
        with os.scandir(root) as entries:
            observed = sum(1 for entry in entries if entry.is_file(follow_symlinks=False))
        elapsed = time.perf_counter_ns() - start
        if observed != expected:
            raise RuntimeError(f"fixture count changed: expected {expected}, got {observed}")
        samples.append(elapsed)
    return {"files": expected, "repeats": repeats,
            "median_seconds": statistics.median(samples) / 1e9,
            "min_seconds": min(samples) / 1e9,
            "max_seconds": max(samples) / 1e9,
            "median_entries_per_second": expected / (statistics.median(samples) / 1e9)}


def run(counts, repeats):
    if not 1 <= repeats <= 20 or not 1 <= len(counts) <= 5 or any(n < 1 or n > 10000 for n in counts) or sum(counts) > 20000:
        raise ValueError("counts must be 1..10000, 1..5 cases, total <= 20000; repeats 1..20")
    results = []
    with tempfile.TemporaryDirectory(prefix="mfc-bench-scale-") as temporary:
        root = Path(temporary)
        for count in counts:
            case = root / f"count-{count}"
            case.mkdir()
            for index in range(count):
                (case / f"fixture-{index:06d}.txt").write_bytes(b"x")
            results.append(measure(case, count, repeats))
    usage = resource.getrusage(resource.RUSAGE_SELF)
    return {"benchmark": "python-os-scandir-disposable-baseline",
            "not_engine_scanner": True,
            "not_production_safety_or_release_evidence": True,
            "platform": platform.platform(), "python": platform.python_version(),
            "cpu_count": os.cpu_count(), "maxrss_platform_units": usage.ru_maxrss,
            "maxrss_units_note": "KiB on Linux; bytes on macOS; platform-specific elsewhere",
            "fixture": "generated one-byte regular files in owned temporary directory",
            "results": results}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--counts", nargs="+", type=int, default=[100, 1000])
    parser.add_argument("--repeats", type=int, default=3)
    args = parser.parse_args()
    print(json.dumps(run(args.counts, args.repeats), indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
