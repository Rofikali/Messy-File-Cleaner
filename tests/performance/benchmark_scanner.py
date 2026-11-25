#!/usr/bin/env python3
"""Bounded read-only native scanner benchmark on generated disposable fixtures."""
import argparse
import json
import math
import platform
import statistics
import subprocess
import tempfile
from pathlib import Path

from benchmark_cli import build_fixture, verify_fixture, filesystem_context, sha256_file


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
            if type(sample.get("files")) is not int or sample["files"] != files:
                raise RuntimeError("scanner callback count mismatch")
            for key in ("elapsed_seconds", "user_cpu_seconds", "system_cpu_seconds",
                        "process_peak_rss_kib"):
                value = sample[key]
                if type(value) not in (int, float) or not math.isfinite(value) or value < 0:
                    raise RuntimeError("invalid native measurement")
            if sample["elapsed_seconds"] <= 0:
                raise RuntimeError("nonpositive elapsed time")
            verify_fixture(root, files)
            if sha256_file(binary) != fingerprint:
                raise RuntimeError("scanner benchmark binary changed")
            if iteration:
                samples.append(sample)

    elapsed = [s["elapsed_seconds"] for s in samples]
    return {
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--files", type=int, default=1000)
    parser.add_argument("--runs", type=int, default=3)
    args = parser.parse_args()
    print(json.dumps(run(args.binary, args.files, args.runs), indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
