#!/usr/bin/env python3
"""CLI-ERROR-01: bounded, non-echoing argument diagnostics without filesystem writes."""
import pathlib
import subprocess
import sys
import tempfile

def main():
    binary = str(pathlib.Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="fc-cli-error-") as tmp:
        root = pathlib.Path(tmp)
        cases = [
            (["--unexpected-option"], "unrecognized option"),
            (["first", "second"], "multiple input roots"),
            (["first", "--bad-\x1b[31m"], "unrecognized option"),
        ]
        for args, expected in cases:
            result = subprocess.run([binary, *args], cwd=tmp, capture_output=True,
                                    text=True, timeout=5, check=False)
            assert result.returncode != 0, (args, result.returncode)
            assert expected in result.stderr, (args, result.stderr)
            assert "Usage:" in result.stderr, (args, result.stderr)
            assert "\x1b" not in result.stderr, result.stderr
            assert not (root / "journal").exists()
            assert not (root / "configs").exists()
            assert "Config load failed" not in result.stdout

if __name__ == "__main__":
    main()
