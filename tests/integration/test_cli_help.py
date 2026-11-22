#!/usr/bin/env python3
"""CLI-HELP-01: --help must not load configuration or touch filesystem state."""
import pathlib
import subprocess
import sys
import tempfile


def run(binary, args, cwd):
    return subprocess.run(
        [binary, *args], cwd=cwd, text=True, capture_output=True,
        timeout=5, check=False,
    )


def main():
    binary = str(pathlib.Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="fc-help-") as directory:
        root = pathlib.Path(directory)
        result = run(binary, ["--help"], directory)
        assert result.returncode == 0, result.stderr
        assert "Usage:" in result.stderr, result.stderr
        assert not (root / "journal").exists()
        assert not (root / "configs").exists()
        for args in (["--help", "--once"], ["--root", "missing", "--help"]):
            invalid = run(binary, args, directory)
            assert invalid.returncode != 0, (args, invalid.stderr)
            assert not (root / "journal").exists()


if __name__ == "__main__":
    main()
