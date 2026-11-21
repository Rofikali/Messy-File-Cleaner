#!/usr/bin/env python3
"""Reject malformed worker counts before config access or filesystem mutation."""
import subprocess
import sys
import unittest


class WorkerArgumentTests(unittest.TestCase):
    def test_invalid_worker_counts_fail_before_config_loading(self):
        binary = sys.argv[1]
        for value in ("0", "-1", "+1", "1x", "1.5", "999999999999999999999999999", "999999"):
            with self.subTest(value=value):
                result = subprocess.run(
                    [binary, "--root", "/nonexistent-cli-input-fixture",
                     "--config", "/nonexistent-cli-input-config", "--workers", value,
                     "--dry-run", "--once"],
                    capture_output=True, text=True, timeout=5, check=False)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Usage:", result.stderr)
                self.assertNotIn("Config load failed", result.stdout)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
