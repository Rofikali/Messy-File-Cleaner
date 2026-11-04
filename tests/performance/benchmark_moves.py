#!/usr/bin/env python3
"""Measure real moves and journal durability using disposable fixtures only."""

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


CATEGORIES = ("Jpeg", "Png", "Html", "Others")
EXTENSIONS = (".jpg", ".png", ".txt", ".html")


def percentile(values: list[float], percent: float) -> float:
    ordered = sorted(values)
    if len(ordered) == 1:
        return ordered[0]
    position = (len(ordered) - 1) * percent / 100.0
    lower = int(position)
    upper = min(lower + 1, len(ordered) - 1)
    fraction = position - lower
    return ordered[lower] + (ordered[upper] - ordered[lower]) * fraction


def build_fixture(root: Path, file_count: int, bytes_per_file: int) -> None:
    payload = b"m" * bytes_per_file
    for index in range(file_count):
        directory = root / f"set-{index // 100:04d}"
        directory.mkdir(parents=True, exist_ok=True)
        path = directory / f"fixture-{index:08d}{EXTENSIONS[index % len(EXTENSIONS)]}"
        path.write_bytes(payload)


def run_once(binary: Path, config: Path, file_count: int,
             bytes_per_file: int, workers: int, measured: bool,
             diagnostics: bool = False) -> int:
    with tempfile.TemporaryDirectory(prefix="file-cleaner-move-bench-") as temp:
        workspace = Path(temp)
        source = workspace / "input"
        source.mkdir()
        (workspace / "journal").mkdir()
        build_fixture(source, file_count, bytes_per_file)

        command = [str(binary), str(source), "--config", str(config), "--once",
                   "--workers", str(workers)]
        start = time.perf_counter_ns() if measured else 0
        completed = subprocess.run(command, cwd=workspace,
                                   stdout=subprocess.DEVNULL,
                                   stderr=subprocess.PIPE if diagnostics else subprocess.DEVNULL,
                                   check=False)
        elapsed = time.perf_counter_ns() - start if measured else 0
        if completed.returncode != 0:
            detail = completed.stderr.decode(errors="replace")[-6000:] if diagnostics and completed.stderr else ""
            raise RuntimeError(f"move run failed with exit code {completed.returncode}\\n{detail}")

        source_files = sum(1 for path in source.rglob("*") if path.is_file())
        moved_files = sum(
            1 for category in CATEGORIES
            for path in (workspace / category).rglob("*")
            if path.is_file()
        )
        journal = workspace / "journal" / "move.log"
        if source_files != 0 or moved_files != file_count or not journal.is_file() or journal.stat().st_size == 0:
            raise RuntimeError(
                f"verification failed: source_files={source_files}, "
                f"moved_files={moved_files}, expected={file_count}, "
                f"journal_present={journal.is_file()}"
            )
        return elapsed


def filesystem_type(path: Path) -> str | None:
    try:
        result = subprocess.run(
            ["stat", "-f", "-c", "%T", str(path)],
            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
            text=True, check=True,
        )
        return result.stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True, help="Built file-cleaner executable")
    parser.add_argument("--config", required=True, help="Configuration file")
    parser.add_argument("--files", type=int, default=1000, help="Fixture file count (default: 1000)")
    parser.add_argument("--bytes-per-file", type=int, default=1024,
                        help="Fixture payload size (default: 1024)")
    parser.add_argument("--runs", type=int, default=10, help="Measured repetitions (default: 10)")
    parser.add_argument("--workers", type=int, default=2, help="Worker count (default: 2)")
    parser.add_argument("--diagnostics", action="store_true", help="Capture and print CLI stderr on failure")
    parser.add_argument("--output", default="benchmark-results/cli-moves",
                        help="Output base path; writes .csv and .json")
    args = parser.parse_args()

    if args.files < 1 or args.runs < 1 or args.bytes_per_file < 0 or not 1 <= args.workers <= 16:
        parser.error("--files and --runs must be positive; --bytes-per-file must be nonnegative; --workers must be 1..16")

    binary = Path(args.binary).resolve()
    config = Path(args.config).resolve()
    if not binary.is_file():
        parser.error(f"binary not found: {binary}")
    if not config.is_file():
        parser.error(f"config not found: {config}")

    # Warm up the executable and filesystem path on a separate disposable fixture.
    run_once(binary, config, min(args.files, 20), args.bytes_per_file, args.workers, measured=False, diagnostics=args.diagnostics)

    samples_ns: list[int] = []
    for _ in range(args.runs):
        samples_ns.append(run_once(binary, config, args.files,
                                  args.bytes_per_file, args.workers, measured=True, diagnostics=args.diagnostics))

    throughput = [args.files * 1_000_000_000 / value for value in samples_ns]
    amortized_file_ns = [value / args.files for value in samples_ns]
    output_base = Path(args.output)
    output_base.parent.mkdir(parents=True, exist_ok=True)
    csv_path = output_base.with_suffix(".csv")
    json_path = output_base.with_suffix(".json")

    with csv_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=[
            "run", "files", "bytes_per_file", "elapsed_ns",
            "files_per_second", "amortized_ns_per_file"
        ])
        writer.writeheader()
        for index, (elapsed, rate, per_file) in enumerate(
                zip(samples_ns, throughput, amortized_file_ns), start=1):
            writer.writerow({
                "run": index,
                "files": args.files,
                "bytes_per_file": args.bytes_per_file,
                "elapsed_ns": elapsed,
                "files_per_second": f"{rate:.3f}",
                "amortized_ns_per_file": f"{per_file:.3f}",
            })

    metadata = {
        "benchmark": "file-cleaner-cli-real-move",
        "created_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
        "binary": str(binary),
        "config": str(config),
        "files_per_run": args.files,
        "bytes_per_file": args.bytes_per_file,
        "measured_runs": args.runs,
        "workers": args.workers,
        "warmup_runs": 1,
        "measurement_clock": "time.perf_counter_ns (monotonic)",
        "platform": platform.platform(),
        "machine": platform.machine(),
        "processor": platform.processor(),
        "python": sys.version,
        "filesystem_type": filesystem_type(Path(tempfile.gettempdir())),
        "elapsed_ns": samples_ns,
        "files_per_second": [round(value, 3) for value in throughput],
        "amortized_ns_per_file": [round(value, 3) for value in amortized_file_ns],
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
            "Measures process startup, traversal, copy/publish, source retirement, "
            "and journal durability for disposable same-filesystem fixtures. "
            "It is not a cross-filesystem benchmark or per-file latency distribution."
        ),
    }
    json_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")

    summary = metadata["elapsed_ns_summary"]
    rate = metadata["files_per_second_summary"]
    print(f"Files/run: {args.files}; bytes/file: {args.bytes_per_file}; runs: {args.runs}; workers: {args.workers}")
    print(f"Elapsed ns median/p95/p99: {summary['median']:.0f} / {summary['p95']:.0f} / {summary['p99']:.0f}")
    print(f"Files/s median/p95/p99: {rate['median']:.1f} / {rate['p95']:.1f} / {rate['p99']:.1f}")
    print(f"Amortized ns/file median: {statistics.median(amortized_file_ns):.0f}")
    print(f"CSV: {csv_path}")
    print(f"Metadata: {json_path}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(f"benchmark error: {error}", file=sys.stderr)
        raise SystemExit(1)
