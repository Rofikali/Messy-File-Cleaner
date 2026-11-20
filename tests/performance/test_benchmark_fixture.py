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

    def test_symlinked_fixture_root_rejected_without_touching_original(self):
        alias = self.root.with_name(self.root.name + "-alias")
        alias.symlink_to(self.root, target_is_directory=True)
        self.addCleanup(alias.unlink)
        with self.assertRaisesRegex(RuntimeError, "fixture root"):
            verify_fixture(alias, 4)
        verify_fixture(self.root, 4)

    def test_missing_fixture_root_rejected(self):
        with self.assertRaisesRegex(RuntimeError, "fixture root"):
            verify_fixture(self.root / "not-a-fixture", 4)

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

    def test_extra_empty_directory_rejected(self):
        (self.root / "unexpected-directory").mkdir()
        with self.assertRaisesRegex(RuntimeError, "directory layout"):
            verify_fixture(self.root, 4)

    def test_directory_symlink_rejected(self):
        (self.root / "directory-link").symlink_to(self.root / "set-0000", target_is_directory=True)
        with self.assertRaisesRegex(RuntimeError, "symlink"):
            verify_fixture(self.root, 4)

    def test_nested_directory_rejected(self):
        (self.root / "set-0000" / "nested").mkdir()
        with self.assertRaisesRegex(RuntimeError, "directory layout"):
            verify_fixture(self.root, 4)

    def test_multiple_fixture_directories_accepted(self):
        build_fixture(self.root, 101)
        verify_fixture(self.root, 101)

    def test_symlink_replacement_rejected(self):
        original = self.root / "set-0000" / "fixture-00000000.jpg"
        original.unlink()
        original.symlink_to(self.root / "set-0000" / "fixture-00000001.png")
        with self.assertRaisesRegex(RuntimeError, "modified or removed"):
            verify_fixture(self.root, 4)


if __name__ == "__main__":
    unittest.main()
