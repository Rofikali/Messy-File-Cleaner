"""No-clobber regression tests for disposable benchmark result paths."""

import stat
import tempfile
import unittest
from pathlib import Path

from benchmark_cli import ensure_fresh_output_paths, open_private_output


class ExclusiveBenchmarkOutputTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="perf-output-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.csv = self.root / "measurements.csv"
        self.json = self.root / "measurements.json"

    def test_private_csv_creation_preserves_text_and_permissions(self):
        with open_private_output(self.csv, newline="") as handle:
            handle.write("run,elapsed_ns\\n1,42\\n")
        self.assertEqual(self.csv.read_text(encoding="utf-8"), "run,elapsed_ns\\n1,42\\n")
        self.assertEqual(stat.S_IMODE(self.csv.stat().st_mode) & 0o077, 0)

    def test_private_json_creation_preserves_text_and_permissions(self):
        with open_private_output(self.json) as handle:
            handle.write('{"runs":1}\\n')
        self.assertEqual(self.json.read_text(encoding="utf-8"), '{"runs":1}\\n')
        self.assertEqual(stat.S_IMODE(self.json.stat().st_mode) & 0o077, 0)

    def test_private_creation_rejects_existing_file_without_clobber(self):
        self.json.write_text("prior result", encoding="utf-8")
        with self.assertRaises(FileExistsError):
            with open_private_output(self.json) as handle:
                handle.write("unreachable")
        self.assertEqual(self.json.read_text(encoding="utf-8"), "prior result")

    def test_private_creation_rejects_symlink_to_existing_evidence(self):
        original = self.root / "original.json"
        original.write_text("prior evidence", encoding="utf-8")
        self.json.symlink_to(original)
        with self.assertRaises(FileExistsError):
            open_private_output(self.json)
        self.assertEqual(original.read_text(encoding="utf-8"), "prior evidence")

    def test_private_creation_rejects_dangling_symlink(self):
        self.json.symlink_to(self.root / "missing.json")
        with self.assertRaises(FileExistsError):
            open_private_output(self.json)

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
