#!/usr/bin/env python3
"""CLI-VALUES-01: malformed path operands fail without configuration or filesystem writes."""
import pathlib
import subprocess
import sys
import tempfile


def main():
    binary = str(pathlib.Path(sys.argv[1]).resolve())
    cases = [
        ["--root"], ["--root", ""], ["--root", "--dry-run"],
        ["--config"], ["--config", ""], ["--config", "--once"],
        [""], ["--root", "valid", "--config", "--dry-run"],
        ["valid", "--workers", "1", "--workers", "2"],
        ["valid", "--workers", "2", "--workers", "2"],
        ["--workers", "1", "--root", "valid", "--workers", "16"],
    ]
    with tempfile.TemporaryDirectory(prefix="fc-cli-values-") as tmp:
        workspace = pathlib.Path(tmp)
        for args in cases:
            result = subprocess.run([binary, *args], cwd=tmp,
                                    capture_output=True, text=True,
                                    timeout=5, check=False)
            assert result.returncode != 0, (args, result.returncode)
            assert "Usage:" in result.stderr, (args, result.stderr)
            assert "Config load failed" not in result.stdout, (args, result.stdout)
            assert not (workspace / "journal").exists(), args
            assert not (workspace / "configs").exists(), args


if __name__ == "__main__":
    main()
