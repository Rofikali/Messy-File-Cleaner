"""Linux/POSIX per-child RSS benchmark regression tests."""

import os
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parent))
from benchmark_cli import peak_rss_kib, run_with_peak_rss


@unittest.skipUnless(sys.platform.startswith("linux") and hasattr(os, "wait4"),
                     "per-child peak RSS contract requires Linux wait4")
class PeakRssTests(unittest.TestCase):
    def test_per_child_usage_and_success(self):
        with tempfile.TemporaryDirectory() as temp:
            command = [sys.executable, "-c", "data = bytearray(8 * 1024 * 1024); print(len(data))"]
            code, detail, usage = run_with_peak_rss(command, Path(temp), True)
        self.assertEqual(code, 0)
        self.assertEqual(detail, "")
        self.assertGreater(peak_rss_kib(usage), 0)

    def test_nonzero_exit_preserves_bounded_diagnostics(self):
        with tempfile.TemporaryDirectory() as temp:
            command = [sys.executable, "-c",
                       "import sys; sys.stderr.write('x' * 8000 + 'TAIL'); sys.exit(7)"]
            code, detail, usage = run_with_peak_rss(command, Path(temp), True)
        self.assertEqual(code, 7)
        self.assertLessEqual(len(detail), 6000)
        self.assertTrue(detail.endswith("TAIL"))
        self.assertGreaterEqual(peak_rss_kib(usage), 0)

    def test_invalid_usage_rejected(self):
        for value in (-1, float("nan"), float("inf")):
            with self.subTest(value=value):
                with self.assertRaisesRegex(RuntimeError, "invalid"):
                    peak_rss_kib(SimpleNamespace(ru_maxrss=value))

    def test_usage_is_not_cumulative_max(self):
        with tempfile.TemporaryDirectory() as temp:
            large = [sys.executable, "-c", "x = bytearray(32 * 1024 * 1024); print(len(x))"]
            small = [sys.executable, "-c", "pass"]
            _, _, large_usage = run_with_peak_rss(large, Path(temp), False)
            _, _, small_usage = run_with_peak_rss(small, Path(temp), False)
        self.assertGreater(peak_rss_kib(large_usage), peak_rss_kib(small_usage))


if __name__ == "__main__":
    unittest.main()
