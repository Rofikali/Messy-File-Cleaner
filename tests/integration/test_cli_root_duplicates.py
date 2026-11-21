#!/usr/bin/env python3
"""Reject ambiguous input roots before config or filesystem access."""
import subprocess
import sys

binary = sys.argv[1]
cases = [
    ["--root", "/nonexistent/a", "--root", "/nonexistent/b"],
    ["/nonexistent/a", "--root", "/nonexistent/b"],
    ["--root", "/nonexistent/a", "/nonexistent/b"],
    ["/nonexistent/a", "/nonexistent/b"],
]
for args in cases:
    result = subprocess.run(
        [binary, *args, "--config", "/nonexistent/config"],
        capture_output=True, text=True, timeout=5, check=False,
    )
    assert result.returncode != 0, (args, result)
    assert "Usage:" in result.stderr, (args, result.stderr)
    assert "Config load failed" not in result.stdout, (args, result.stdout)
print("ambiguous root arguments rejected before configuration loading")
