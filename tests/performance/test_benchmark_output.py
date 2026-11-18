"""No-clobber regression tests for disposable benchmark result paths."""

import tempfile
import unittest
from pathlib import Path

from benchmark_cli import ensure_fresh_output_paths


class ExclusiveBenchmarkOutputTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="perf-output-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.csv = self.root / "measurements.csv"
        self.json = self.root / "measurements.json"

    def test_fresh_paths_accepted(self):
        ensure_fresh_output_paths(self.csv, self.json)

    def test_existing_csv_refused_and_preserved(self):
        self.csv.write_text("existing evidence", encoding="utf-8")
        with self.assertRaisesRegex(FileExistsError, "already exists"):
            ensure_fresh_output_paths(self.csv, self.json)
        self.assertEqual(self.csv.read_text(encoding="utf-8"), "existing evidence")

    def test_existing_json_refused_and_preserved(self):
        self.json.write_text('{"existing":true}', encoding="utf-8")
        with self.assertRaises(FileExistsError):
            ensure_fresh_output_paths(self.csv, self.json)
        self.assertFalse(self.csv.exists())
        self.assertEqual(self.json.read_text(encoding="utf-8"), '{"existing":true}')

    def test_dangling_symlink_refused(self):
        self.csv.symlink_to(self.root / "missing")
        with self.assertRaises(FileExistsError):
            ensure_fresh_output_paths(self.csv, self.json)

    def test_exclusive_open_rejects_race_after_preflight(self):
        ensure_fresh_output_paths(self.csv, self.json)
        self.csv.write_text("racing writer", encoding="utf-8")
        with self.assertRaises(FileExistsError):
            self.csv.open("x", encoding="utf-8")
        self.assertEqual(self.csv.read_text(encoding="utf-8"), "racing writer")


if __name__ == "__main__":
    unittest.main()
