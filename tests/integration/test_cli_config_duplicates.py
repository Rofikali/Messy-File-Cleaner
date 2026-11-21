"""Reject duplicate --config before configuration or filesystem access."""
import subprocess
import sys

def main():
    binary = sys.argv[1]
    cases = [
        ["--config", "/nonexistent/first.conf", "--config", "/nonexistent/second.conf"],
        ["--config", "/nonexistent/same.conf", "--config", "/nonexistent/same.conf"],
    ]
    for args in cases:
        result = subprocess.run([binary, "/nonexistent/input-root", *args],
                                capture_output=True, text=True, timeout=10)
        assert result.returncode != 0, (args, result.stdout)
        assert "Usage:" in result.stderr, (args, result.stderr)
        assert "Config load failed" not in result.stdout, (args, result.stdout)

if __name__ == "__main__":
    main()
