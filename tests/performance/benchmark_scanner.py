#!/usr/bin/env python3
"""Bounded read-only native scanner benchmark on generated disposable fixtures."""
import argparse
import json
import math
import platform
import re
import statistics
import subprocess
import tempfile
from pathlib import Path

from benchmark_cli import build_fixture, verify_fixture, filesystem_context, sha256_file


def validate_sample(sample, files):
    if not isinstance(sample, dict) or type(sample.get("files")) is not int or sample["files"] != files:
        raise ValueError("scanner callback count mismatch")
    for key in ("elapsed_seconds", "user_cpu_seconds", "system_cpu_seconds",
                "process_peak_rss_kib"):
        value = sample.get(key)
        if type(value) not in (int, float) or not math.isfinite(value) or value < 0:
            raise ValueError("invalid native measurement: " + key)
    if sample["elapsed_seconds"] <= 0 or type(sample["process_peak_rss_kib"]) is not int:
        raise ValueError("invalid elapsed time or RSS")


def validate_report(report, files, runs, fingerprint):
    """Check semantic evidence, not a signature or independent execution proof."""
    if not 1 <= files <= 10000 or not 1 <= runs <= 10:
        raise ValueError("invalid expected workload")
    if not isinstance(fingerprint, str) or not re.fullmatch(r"[0-9a-f]{64}", fingerprint):
        raise ValueError("invalid expected fingerprint")
    if not isinstance(report, dict):
        raise ValueError("report must be an object")
    expected = {
        "schema_version": 1,
        "benchmark": "native-c-scanner-count-only",
        "not_cli_or_move_benchmark": True,
        "not_release_evidence": True,
        "binary_sha256": fingerprint,
        "files": files,
        "runs": runs,
    }
    for key, value in expected.items():
        observed = report.get(key)
        if type(observed) is not type(value) or observed != value:
            raise ValueError("report metadata mismatch: " + key)
    samples = report.get("samples")
    if not isinstance(samples, list) or len(samples) != runs:
        raise ValueError("sample count mismatch")
    for sample in samples:
        validate_sample(sample, files)
    elapsed = [sample["elapsed_seconds"] for sample in samples]
    median = statistics.median(elapsed)
    summaries = {
        "median_seconds": median,
        "median_files_per_second": files / median,
        "min_seconds": min(elapsed),
        "max_seconds": max(elapsed),
    }
    for key, expected_value in summaries.items():
        value = report.get(key)
        if (type(value) not in (int, float) or not math.isfinite(value)
                or not math.isclose(value, expected_value, rel_tol=1e-12, abs_tol=0)):
            raise ValueError("inconsistent report summary: " + key)


def run(binary: Path, files: int, runs: int):
    if not 1 <= files <= 10000 or not 1 <= runs <= 10:
        raise ValueError("files must be 1..10000 and runs 1..10")
    binary = binary.resolve(strict=True)
    fingerprint = sha256_file(binary)
    samples = []
    with tempfile.TemporaryDirectory(prefix="fc-native-scan-") as temp:
        root = Path(temp)
        build_fixture(root, files)
        filesystem = filesystem_context(root)
        # First measurement is a warmup; fixture generation/validation is untimed.
        for iteration in range(runs + 1):
            verify_fixture(root, files)
            if sha256_file(binary) != fingerprint:
                raise RuntimeError("scanner benchmark binary changed")
            result = subprocess.run([str(binary), str(root)], cwd=temp,
                                    capture_output=True, text=True, timeout=60,
                                    check=True)
            sample = json.loads(result.stdout)
            validate_sample(sample, files)
            verify_fixture(root, files)
            if sha256_file(binary) != fingerprint:
                raise RuntimeError("scanner benchmark binary changed")
            if iteration:
                samples.append(sample)

    elapsed = [s["elapsed_seconds"] for s in samples]
    report = {
        "schema_version": 1,
        "benchmark": "native-c-scanner-count-only",
        "not_cli_or_move_benchmark": True,
        "not_release_evidence": True,
        "binary_sha256": fingerprint,
        "platform": platform.platform(),
        "filesystem": filesystem,
        "fixture": "generated files in directories of 100; fixture creation and verification excluded",
        "cache_condition": "one warmup; cache not controlled",
        "peak_rss_scope": "Linux per-process lifetime, not scanner-only allocation",
        "files": files,
        "runs": runs,
        "samples": samples,
        "median_seconds": statistics.median(elapsed),
        "median_files_per_second": files / statistics.median(elapsed),
        "min_seconds": min(elapsed),
        "max_seconds": max(elapsed),
    }
    validate_report(report, files, runs, fingerprint)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--files", type=int, default=1000)
    parser.add_argument("--runs", type=int, default=3)
    parser.add_argument("--verify-report", type=Path,
                        help="Validate a saved report against this binary and workload")
    args = parser.parse_args()
    if args.verify_report is not None:
        validate_report(json.loads(args.verify_report.read_text(encoding="utf-8")),
                        args.files, args.runs, sha256_file(args.binary))
        print("Native scanner report verified")
        return
    print(json.dumps(run(args.binary, args.files, args.runs), indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
