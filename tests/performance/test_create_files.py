#!/usr/bin/env python3
"""Safety and correctness checks for the standalone C fixture generator."""

from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path


def run(binary: Path, *arguments: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(binary), *arguments],
        check=False,
        capture_output=True,
        text=True,
        timeout=30,
    )


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: test_create_files.py /path/to/create-files", file=sys.stderr)
        return 2

    binary = Path(sys.argv[1]).resolve()
    if not binary.is_file():
        print(f"generator binary not found: {binary}", file=sys.stderr)
        return 2

    with tempfile.TemporaryDirectory(prefix="create-files-test-") as temp:
        workspace = Path(temp)
        output = workspace / "fixture"

        completed = run(
            binary,
            "--output", str(output),
            "--files", "123",
            "--threads", "4",
            "--seed", "2026",
        )
        if completed.returncode != 0:
            raise AssertionError(f"generation failed: {completed.stderr}")

        files = sorted(path for path in output.iterdir() if path.is_file())
        if len(files) != 123:
            raise AssertionError(f"expected 123 files, found {len(files)}")
        if any(path.stat().st_size != 0 for path in files):
            raise AssertionError("generator must create empty files")
        if len({path.name for path in files}) != 123:
            raise AssertionError("generated filenames are not unique")
        if any(path.suffix not in {".jpg", ".jpeg", ".png", ".html", ".js", ".java"}
               for path in files):
            raise AssertionError("unexpected generated file extension")

        # Existing output directories must never be reused or have files truncated.
        sentinel_dir = workspace / "existing"
        sentinel_dir.mkdir()
        sentinel = sentinel_dir / "keep.txt"
        sentinel.write_bytes(b"do not modify")
        refused = run(binary, "--output", str(sentinel_dir), "--files", "1")
        if refused.returncode == 0:
            raise AssertionError("generator unexpectedly reused an existing directory")
        if sentinel.read_bytes() != b"do not modify":
            raise AssertionError("existing user data was modified")

        # Invalid arguments must fail before creating an output directory.
        invalid_output = workspace / "invalid"
        invalid = run(binary, "--output", str(invalid_output),
                      "--files", "10", "--threads", "0")
        if invalid.returncode == 0:
            raise AssertionError("zero worker count was accepted")
        if invalid_output.exists():
            raise AssertionError("invalid arguments created an output directory")

    print("fixture generator safety tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
