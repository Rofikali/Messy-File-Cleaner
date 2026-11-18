"""Unit checks for per-child CPU benchmark measurements."""

import sys
import unittest
from pathlib import Path
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parent))
from benchmark_cli import child_cpu_delta_ns


class ChildCpuDeltaTests(unittest.TestCase):
    def test_positive_user_and_system_cpu(self):
        before = SimpleNamespace(ru_utime=1.0, ru_stime=2.0)
        after = SimpleNamespace(ru_utime=1.25, ru_stime=2.125)
        self.assertEqual(child_cpu_delta_ns(before, after), (250_000_000, 125_000_000))

    def test_zero_cpu_delta(self):
        usage = SimpleNamespace(ru_utime=1.0, ru_stime=2.0)
        self.assertEqual(child_cpu_delta_ns(usage, usage), (0, 0))

    def test_decreasing_counter_rejected(self):
        before = SimpleNamespace(ru_utime=2.0, ru_stime=1.0)
        after = SimpleNamespace(ru_utime=1.0, ru_stime=1.0)
        with self.assertRaisesRegex(RuntimeError, "decreased"):
            child_cpu_delta_ns(before, after)


if __name__ == "__main__":
    unittest.main()
