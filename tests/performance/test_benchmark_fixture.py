"""Regression tests for disposable dry-run benchmark fixture integrity."""

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from benchmark_cli import build_fixture, verify_fixture


class FixtureIntegrityTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="benchmark-integrity-test-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        build_fixture(self.root, 4)

    def test_intact_fixture_accepted(self):
        verify_fixture(self.root, 4)

    def test_removed_fixture_rejected(self):
        (self.root / "set-0000" / "fixture-00000000.jpg").unlink()
        with self.assertRaisesRegex(RuntimeError, "modified or removed"):
            verify_fixture(self.root, 4)

    def test_modified_fixture_rejected(self):
        (self.root / "set-0000" / "fixture-00000001.png").write_bytes(b"changed")
        with self.assertRaisesRegex(RuntimeError, "modified or removed"):
            verify_fixture(self.root, 4)

    def test_extra_fixture_rejected(self):
        (self.root / "unexpected.txt").write_bytes(b"unexpected")
        with self.assertRaisesRegex(RuntimeError, "entry count"):
            verify_fixture(self.root, 4)

    def test_symlink_replacement_rejected(self):
        original = self.root / "set-0000" / "fixture-00000000.jpg"
        original.unlink()
        original.symlink_to(self.root / "set-0000" / "fixture-00000001.png")
        with self.assertRaisesRegex(RuntimeError, "modified or removed"):
            verify_fixture(self.root, 4)


if __name__ == "__main__":
    unittest.main()
