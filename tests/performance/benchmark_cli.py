#!/usr/bin/env python3
"""Repeatable, safe CLI dry-run benchmark. Uses only disposable temporary files."""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import hashlib
import json
import os
import platform
import resource
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



def verify_fixture(root: Path, file_count: int) -> None:
    """Fail if a purported dry-run removed, altered, or added fixture entries.

    Validation is deliberately outside timed measurements. This is a benchmark
    integrity check, not a production filesystem security guarantee.
    """
    if root.is_symlink() or not root.is_dir():
        raise RuntimeError("dry-run replaced fixture root with a symlink or non-directory")
    extensions = (".jpg", ".png", ".txt", ".html")
    payload = b"benchmark fixture\n"
    for directory_index in range((file_count + 99) // 100):
        directory = root / f"set-{directory_index:04d}"
        if directory.is_symlink() or not directory.is_dir():
            raise RuntimeError(f"dry-run replaced fixture directory with a symlink or non-directory: {directory}")
    for index in range(file_count):
        path = root / f"set-{index // 100:04d}" / f"fixture-{index:08d}{extensions[index % len(extensions)]}"
        if path.is_symlink() or not path.is_file() or path.stat().st_nlink != 1 or path.read_bytes() != payload:
            raise RuntimeError(f"dry-run modified or removed fixture: {path}")
    # Check the directory tree too: extra empty directories and directory
    # symlinks are not captured by the file-only count.
    expected_dirs = {f"set-{index:04d}" for index in range((file_count + 99) // 100)}
    actual_dirs = set()
    entries = 0
    for path in root.rglob("*"):
        if path.is_symlink():
            raise RuntimeError(f"dry-run introduced a symlink: {path}")
        if path.is_dir():
            relative = path.relative_to(root).as_posix()
            actual_dirs.add(relative)
        elif path.is_file():
            entries += 1
        else:
            raise RuntimeError(f"dry-run introduced an unexpected entry: {path}")
    if actual_dirs != expected_dirs:
        raise RuntimeError("dry-run changed fixture directory layout")
    if entries != file_count:
        raise RuntimeError(f"dry-run changed fixture entry count: expected {file_count}, got {entries}")

def child_cpu_delta_ns(before, after) -> tuple[int, int]:
    """Return nonnegative child CPU deltas (user, system), excluding harness work."""
    user = round((after.ru_utime - before.ru_utime) * 1_000_000_000)
    system = round((after.ru_stime - before.ru_stime) * 1_000_000_000)
    if user < 0 or system < 0:
        raise RuntimeError("child CPU usage counters decreased")
    return user, system



def run_with_peak_rss(command: list[str], cwd: Path, diagnostics: bool):
    """Return (exit status, bounded stderr, child resource usage) on Linux/POSIX.

    wait4 captures per-process peak RSS. RUSAGE_CHILDREN.ru_maxrss is a
    historical maximum and cannot be differenced to obtain per-run RSS.
    """
    with tempfile.TemporaryFile() as stderr_file:
        with subprocess.Popen(
            command, cwd=cwd, stdout=subprocess.DEVNULL,
            stderr=stderr_file if diagnostics else subprocess.DEVNULL,
        ) as process:
            while True:
                try:
                    _, status, usage = os.wait4(process.pid, 0)
                    break
                except InterruptedError:
                    continue
            process.returncode = os.waitstatus_to_exitcode(status)
        detail = ""
        if diagnostics and process.returncode != 0:
            stderr_file.seek(0, os.SEEK_END)
            size = stderr_file.tell()
            stderr_file.seek(max(0, size - 6000))
            detail = stderr_file.read().decode(errors="replace")
        return process.returncode, detail, usage


def peak_rss_kib(usage) -> int:
    """Linux wait4 ru_maxrss reports peak resident set size in KiB."""
    value = usage.ru_maxrss
    if not isinstance(value, (int, float)) or not 0 <= value < float("inf"):
        raise RuntimeError("invalid child peak RSS")
    return int(value)



def filesystem_context(path: Path) -> dict[str, int | None]:
    """Capture POSIX filesystem capacity for the actual disposable workspace.

    Values describe the filesystem at sampling time, not benchmark I/O usage.
    A zero inode count means the filesystem does not report an inode limit.
    """
    stats = os.statvfs(path)
    device = path.stat().st_dev
    block_size = stats.f_frsize or stats.f_bsize
    if block_size <= 0 or stats.f_blocks < 0 or not 0 <= stats.f_bavail <= stats.f_blocks:
        raise RuntimeError("invalid filesystem capacity counters")
    if stats.f_files < 0 or not 0 <= stats.f_favail <= stats.f_files:
        raise RuntimeError("invalid filesystem inode counters")
    return {
        "device_id": device,
        "block_size_bytes": block_size,
        "capacity_bytes": stats.f_blocks * block_size,
        "available_bytes": stats.f_bavail * block_size,
        "total_inodes": stats.f_files or None,
        "available_inodes": stats.f_favail if stats.f_files else None,
    }


def sha256_file(path: Path) -> str:
    """Stream a reproducible fingerprint without retaining the input in memory."""
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def verify_input_fingerprints(binary: Path, config: Path, expected: dict[str, str]) -> None:
    """Reject changes to the executable or configuration during measurement."""
    if sha256_file(binary) != expected["binary_sha256"]:
        raise RuntimeError("benchmark executable changed during measurements")
    if sha256_file(config) != expected["config_sha256"]:
        raise RuntimeError("benchmark configuration changed during measurements")


def verify_measurement_checkpoint(root: Path, count: int, binary: Path,
                                  config: Path, expected: dict[str, str]) -> None:
    """Validate fixture and both inputs outside timed subprocess execution."""
    verify_fixture(root, count)
    verify_input_fingerprints(binary, config, expected)


def open_private_output(path: Path, *, newline: str | None = None):
    """Create non-overwriting benchmark evidence with owner-only permissions.

    The final pathname is protected by exclusive creation; this is not an
    atomic transaction across CSV and JSON or a hostile-parent-directory defense.
    """
    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL
    if hasattr(os, "O_NOFOLLOW"):
        flags |= os.O_NOFOLLOW
    fd = os.open(path, flags, 0o600)
    try:
        return os.fdopen(fd, "w", encoding="utf-8", newline=newline)
    except BaseException:
        os.close(fd)
        raise


def ensure_fresh_output_paths(csv_path: Path, json_path: Path) -> None:
    """Refuse to overwrite prior benchmark evidence or follow output symlinks.

    This is an early diagnostic, not a race-free reservation: exclusive
    creation at write time remains the authoritative no-clobber guard.
    """
    for path in (csv_path, json_path):
        if path.is_symlink() or path.exists():
            raise FileExistsError(f"benchmark output already exists: {path}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True, help="Built file-cleaner executable")
    parser.add_argument("--config", required=True, help="Configuration file")
    parser.add_argument("--files", type=int, default=10000, help="Fixture file count (default: 10000; maximum: 1000000)")
    parser.add_argument("--runs", type=int, default=20, help="Measured repetitions (default: 20)")
    parser.add_argument("--workers", type=int, default=2, help="Worker count (default: 2)")
    parser.add_argument("--diagnostics", action="store_true", help="Capture and print CLI stderr on failure")
    parser.add_argument("--output", default="benchmark-results/cli-dry-run",
                        help="Output base path; writes .csv and .json")
    args = parser.parse_args()

    if args.files < 1 or args.files > 1_000_000 or args.runs < 1 or not 1 <= args.workers <= 16:
        parser.error("--files must be between 1 and 1000000; --runs must be positive; --workers must be between 1 and 16")

    binary = Path(args.binary).resolve()
    config = Path(args.config).resolve()
    if not binary.is_file():
        parser.error(f"binary not found: {binary}")
    if not config.is_file():
        parser.error(f"config not found: {config}")

    fingerprints = {
        "binary_sha256": sha256_file(binary),
        "config_sha256": sha256_file(config),
    }
    samples_ns: list[int] = []
    cpu_user_ns: list[int] = []
    cpu_system_ns: list[int] = []
    peak_rss_kib_samples: list[int] = []
    fixture_creation_ns = 0
    fixture_directory_count = 0
    filesystem_before: dict[str, int | None] = {}
    filesystem_after: dict[str, int | None] = {}
    with tempfile.TemporaryDirectory(prefix="file-cleaner-bench-") as temp:
        workspace = Path(temp)
        fixture_root = workspace / "input"
        fixture_root.mkdir()
        filesystem_before = filesystem_context(workspace)
        fixture_start = time.perf_counter_ns()
        build_fixture(fixture_root, args.files)
        fixture_creation_ns = time.perf_counter_ns() - fixture_start
        fixture_directory_count = sum(1 for path in fixture_root.iterdir() if path.is_dir())
        print(f"Fixture ready: {args.files:,} files in {fixture_directory_count:,} directories; creation {fixture_creation_ns / 1_000_000_000:.1f}s", flush=True)

        command = [str(binary), str(fixture_root), "--config", str(config),
                   "--dry-run", "--once", "--workers", str(args.workers)]
        for _ in range(2):
            returncode, detail, _ = run_with_peak_rss(command, workspace, args.diagnostics)
            if returncode != 0:
                raise RuntimeError(f"warm-up run failed with exit code {returncode}\\n{detail}")

        verify_measurement_checkpoint(fixture_root, args.files, binary, config, fingerprints)

        for _ in range(args.runs):
            cpu_before = resource.getrusage(resource.RUSAGE_CHILDREN)
            start = time.perf_counter_ns()
            returncode, detail, usage = run_with_peak_rss(command, workspace, args.diagnostics)
            elapsed = time.perf_counter_ns() - start
            cpu_after = resource.getrusage(resource.RUSAGE_CHILDREN)
            user_ns, system_ns = child_cpu_delta_ns(cpu_before, cpu_after)
            if returncode != 0:
                raise RuntimeError(f"measured run failed with exit code {returncode}\\n{detail}")
            samples_ns.append(elapsed)
            cpu_user_ns.append(user_ns)
            cpu_system_ns.append(system_ns)
            peak_rss_kib_samples.append(peak_rss_kib(usage))
            # Outside the measured interval: detect mutation at the run
            # that caused it, rather than only after all repetitions.
            verify_measurement_checkpoint(fixture_root, args.files, binary, config, fingerprints)

        filesystem_after = filesystem_context(workspace)
        verify_input_fingerprints(binary, config, fingerprints)

    throughput = [args.files * 1_000_000_000 / value for value in samples_ns]
    output_base = Path(args.output)
    csv_path = output_base.with_suffix(".csv")
    json_path = output_base.with_suffix(".json")
    ensure_fresh_output_paths(csv_path, json_path)
    output_base.parent.mkdir(parents=True, exist_ok=True)

    with open_private_output(csv_path, newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=[
            "run", "files", "elapsed_ns", "files_per_second",
            "child_cpu_user_ns", "child_cpu_system_ns", "child_peak_rss_kib"
        ])
        writer.writeheader()
        for index, (elapsed, rate, user_ns, system_ns, rss_kib) in enumerate(
                zip(samples_ns, throughput, cpu_user_ns, cpu_system_ns, peak_rss_kib_samples), start=1):
            writer.writerow({
                "run": index,
                "files": args.files,
                "elapsed_ns": elapsed,
                "child_cpu_user_ns": user_ns,
                "child_cpu_system_ns": system_ns,
                "child_peak_rss_kib": rss_kib,
                "files_per_second": f"{rate:.3f}",
            })

    metadata = {
        "benchmark": "file-cleaner-cli-dry-run",
        "created_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
        "binary": str(binary),
        "binary_sha256": fingerprints["binary_sha256"],
        "config_sha256": fingerprints["config_sha256"],
        "input_fingerprint_scope_note": "SHA-256 of executable and configuration before execution, after warm-up, after each measured run and at benchmark completion; detects changes at checkpoints, not concurrent modifications or provenance.",
        "config": str(config),
        "files": args.files,
        "fixture_directories": fixture_directory_count,
        "filesystem_before": filesystem_before,
        "filesystem_after": filesystem_after,
        "filesystem_scope_note": "POSIX statvfs capacity/inode snapshots of disposable workspace before fixture creation and after measurements; not bytes written, cache state, disk speed or filesystem type.",
        "fixture_creation_elapsed_ns": fixture_creation_ns,
        "fixture_file_bytes": len(b"benchmark fixture\n"),
        "measured_runs": args.runs,
        "workers": args.workers,
        "warmup_runs": 2,
        "measurement_clock": "time.perf_counter_ns (monotonic)",
        "stdout_stderr": "redirected to /dev/null",
        "platform": platform.platform(),
        "machine": platform.machine(),
        "processor": platform.processor(),
        "python": sys.version,
        "elapsed_ns": samples_ns,
        "child_cpu_user_ns": cpu_user_ns,
        "child_cpu_system_ns": cpu_system_ns,
        "child_peak_rss_kib": peak_rss_kib_samples,
        "child_peak_rss_kib_summary": {
            "min": min(peak_rss_kib_samples),
            "median": statistics.median(peak_rss_kib_samples),
            "p95": percentile(peak_rss_kib_samples, 95),
            "max": max(peak_rss_kib_samples),
        },
        "rss_scope_note": "Linux os.wait4 per-child ru_maxrss (KiB), including CLI startup; not cumulative child usage.",
        "cpu_scope_note": "POSIX RUSAGE_CHILDREN user/system CPU deltas per CLI subprocess; excludes Python fixture verification.",
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
    with open_private_output(json_path) as handle:
        handle.write(json.dumps(metadata, indent=2) + "\n")

    print(f"Files/run: {args.files}; measured runs: {args.runs}; workers: {args.workers}")
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
