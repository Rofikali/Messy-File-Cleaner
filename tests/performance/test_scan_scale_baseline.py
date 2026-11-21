"""Small, disposable self-checks for the enumeration baseline."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

MODULE = Path(__file__).with_name("scan_scale_baseline.py")
spec = importlib.util.spec_from_file_location("scan_scale_baseline", MODULE)
baseline = importlib.util.module_from_spec(spec)
spec.loader.exec_module(baseline)


class ScanScaleBaselineTests(unittest.TestCase):
    def test_small_disposable_case(self):
        report = baseline.run([3], 2)
        self.assertTrue(report["not_engine_scanner"])
        self.assertEqual(report["results"][0]["files"], 3)
        self.assertGreater(report["results"][0]["median_entries_per_second"], 0)

    def test_rejects_unbounded_or_invalid_workload(self):
        for counts, repeats in [([0], 2), ([10001], 2), ([2], 0), ([], 1)]:
            with self.subTest(counts=counts, repeats=repeats):
                with self.assertRaises(ValueError):
                    baseline.run(counts, repeats)

    def test_detects_unexpected_fixture(self):
        with tempfile.TemporaryDirectory() as tmp:
            (Path(tmp) / "file").write_bytes(b"x")
            with self.assertRaises(RuntimeError):
                baseline.measure(Path(tmp), 2, 1)


if __name__ == "__main__":
    unittest.main()
