"""Reject conflicting CLI execution modes before config or filesystem access."""
import subprocess
import sys


def main():
    binary = sys.argv[1]
    cases = (
        ("--once", "--watch"),
        ("--watch", "--once"),
        ("--once", "--watch", "--once"),
    )
    for flags in cases:
        result = subprocess.run(
            [binary, "/nonexistent-cli-mode-test-root",
             "--config", "/nonexistent-cli-mode-test-config", *flags],
            capture_output=True, text=True, timeout=5, check=False,
        )
        if result.returncode == 0 or "Usage:" not in result.stderr:
            raise AssertionError(f"conflicting modes {flags!r} not rejected by CLI parser: {result.returncode}, {result.stderr!r}")
        if "Config load failed" in result.stdout:
            raise AssertionError("configuration accessed before rejecting modes")
    print("CLI mode conflicts rejected before configuration access")


if __name__ == "__main__":
    main()
