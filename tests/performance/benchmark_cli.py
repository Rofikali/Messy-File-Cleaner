#!/usr/bin/env python3
"""Repeatable, safe CLI dry-run benchmark. Uses only disposable temporary files."""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import json
import platform
import statistics
import subprocess
import sys
import tempfile
import time
from pathlib import Path


def percentile(values: list[float], percent: float) -> float:
    ordered = sorted(values)
    if len(ordered) == 1:
        return ordered[0]
    position = (len(ordered) - 1) * percent / 100.0
    lower = int(position)
    upper = min(lower + 1, len(ordered) - 1)
    fraction = position - lower
    return ordered[lower] + (ordered[upper] - ordered[lower]) * fraction


def build_fixture(root: Path, file_count: int) -> None:
    extensions = (".jpg", ".png", ".txt", ".html")
    for index in range(file_count):
        directory = root / f"set-{index // 100:04d}"
        directory.mkdir(parents=True, exist_ok=True)
        path = directory / f"fixture-{index:08d}{extensions[index % len(extensions)]}"
        path.write_bytes(b"benchmark fixture\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True, help="Built file-cleaner executable")
    parser.add_argument("--config", required=True, help="Configuration file")
    parser.add_argument("--files", type=int, default=10000, help="Fixture file count (default: 10000)")
    parser.add_argument("--runs", type=int, default=20, help="Measured repetitions (default: 20)")
    parser.add_argument("--output", default="benchmark-results/cli-dry-run",
                        help="Output base path; writes .csv and .json")
    args = parser.parse_args()

    if args.files < 1 or args.runs < 1:
        parser.error("--files and --runs must be positive integers")

    binary = Path(args.binary).resolve()
    config = Path(args.config).resolve()
    if not binary.is_file():
        parser.error(f"binary not found: {binary}")
    if not config.is_file():
        parser.error(f"config not found: {config}")

    samples_ns: list[int] = []
    with tempfile.TemporaryDirectory(prefix="file-cleaner-bench-") as temp:
        workspace = Path(temp)
        fixture_root = workspace / "input"
        fixture_root.mkdir()
        build_fixture(fixture_root, args.files)

        command = [str(binary), str(fixture_root), "--config", str(config),
                   "--dry-run", "--once"]
        for _ in range(2):
            warmup = subprocess.run(command, cwd=workspace,
                                    stdout=subprocess.DEVNULL,
                                    stderr=subprocess.DEVNULL,
                                    check=False)
            if warmup.returncode != 0:
                raise RuntimeError(f"warm-up run failed with exit code {warmup.returncode}")

        for _ in range(args.runs):
            start = time.perf_counter_ns()
            completed = subprocess.run(command, cwd=workspace,
                                       stdout=subprocess.DEVNULL,
                                       stderr=subprocess.DEVNULL,
                                       check=False)
            elapsed = time.perf_counter_ns() - start
            if completed.returncode != 0:
                raise RuntimeError(f"measured run failed with exit code {completed.returncode}")
            samples_ns.append(elapsed)

    throughput = [args.files * 1_000_000_000 / value for value in samples_ns]
    output_base = Path(args.output)
    output_base.parent.mkdir(parents=True, exist_ok=True)
    csv_path = output_base.with_suffix(".csv")
    json_path = output_base.with_suffix(".json")

    with csv_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=[
            "run", "files", "elapsed_ns", "files_per_second"
        ])
        writer.writeheader()
        for index, (elapsed, rate) in enumerate(zip(samples_ns, throughput), start=1):
            writer.writerow({
                "run": index,
                "files": args.files,
                "elapsed_ns": elapsed,
                "files_per_second": f"{rate:.3f}",
            })

    metadata = {
        "benchmark": "file-cleaner-cli-dry-run",
        "created_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
        "binary": str(binary),
        "config": str(config),
        "files": args.files,
        "measured_runs": args.runs,
        "warmup_runs": 2,
        "measurement_clock": "time.perf_counter_ns (monotonic)",
        "stdout_stderr": "redirected to /dev/null",
        "platform": platform.platform(),
        "machine": platform.machine(),
        "processor": platform.processor(),
        "python": sys.version,
        "elapsed_ns": samples_ns,
        "files_per_second": [round(value, 3) for value in throughput],
        "elapsed_ns_summary": {
            "min": min(samples_ns),
            "median": statistics.median(samples_ns),
            "p95": percentile(samples_ns, 95),
            "p99": percentile(samples_ns, 99),
            "max": max(samples_ns),
        },
        "files_per_second_summary": {
            "min": min(throughput),
            "median": statistics.median(throughput),
            "p95": percentile(throughput, 95),
            "p99": percentile(throughput, 99),
            "max": max(throughput),
        },
        "scope_note": (
            "Measures process startup plus recursive scan/classification in dry-run mode. "
            "It is not a disk-move benchmark; stdout/stderr are discarded, but per-file "
            "diagnostic formatting still occurs."
        ),
    }
    json_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")

    print(f"Files/run: {args.files}; measured runs: {args.runs}")
    print(f"Elapsed ns median/p95/p99: "
          f"{metadata['elapsed_ns_summary']['median']:.0f} / "
          f"{metadata['elapsed_ns_summary']['p95']:.0f} / "
          f"{metadata['elapsed_ns_summary']['p99']:.0f}")
    print(f"Files/s median/p95/p99: "
          f"{metadata['files_per_second_summary']['median']:.1f} / "
          f"{metadata['files_per_second_summary']['p95']:.1f} / "
          f"{metadata['files_per_second_summary']['p99']:.1f}")
    print(f"CSV: {csv_path}")
    print(f"Metadata: {json_path}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(f"benchmark error: {error}", file=sys.stderr)
        raise SystemExit(1)
