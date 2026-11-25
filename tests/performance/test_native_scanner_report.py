"""Evidence acceptance regression; synthetic reports are not performance results."""
import copy
import unittest
from benchmark_scanner import validate_report


class NativeScannerReportTests(unittest.TestCase):
    def setUp(self):
        self.fingerprint = "a" * 64
        self.report = {
            "schema_version": 1, "benchmark": "native-c-scanner-count-only",
            "not_cli_or_move_benchmark": True, "not_release_evidence": True,
            "binary_sha256": self.fingerprint, "files": 100, "runs": 2,
            "samples": [
                {"files": 100, "elapsed_seconds": elapsed,
                 "user_cpu_seconds": 0.01, "system_cpu_seconds": 0.02,
                 "process_peak_rss_kib": 4000}
                for elapsed in (1.0, 3.0)
            ],
            "median_seconds": 2.0, "median_files_per_second": 50.0,
            "min_seconds": 1.0, "max_seconds": 3.0,
        }

    def validate(self, report):
        validate_report(report, 100, 2, self.fingerprint)

    def test_valid(self):
        self.validate(self.report)

    def test_bad_metadata(self):
        for key, value in (
            ("schema_version", True), ("files", 101), ("runs", 3),
            ("binary_sha256", "b" * 64), ("not_release_evidence", False),
            ("not_cli_or_move_benchmark", 1), ("median_seconds", 1.0),
            ("median_files_per_second", 100.0), ("max_seconds", 1.0),
            ("min_seconds", float("nan")),
        ):
            with self.subTest(key=key):
                report = copy.deepcopy(self.report)
                report[key] = value
                with self.assertRaises(ValueError):
                    self.validate(report)

    def test_bad_samples(self):
        for key, value in (
            ("files", 99), ("elapsed_seconds", 0), ("elapsed_seconds", float("inf")),
            ("user_cpu_seconds", -1), ("system_cpu_seconds", True),
            ("process_peak_rss_kib", 1.5),
        ):
            with self.subTest(key=key):
                report = copy.deepcopy(self.report)
                report["samples"][0][key] = value
                with self.assertRaises(ValueError):
                    self.validate(report)
        report = copy.deepcopy(self.report)
        report["samples"].pop()
        with self.assertRaises(ValueError):
            self.validate(report)


if __name__ == "__main__":
    unittest.main()
