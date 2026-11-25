"""Disposable structural import-lock tests; no third-party source."""
import copy
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from upstream_import_check import EXPECTED, RESERVED, validate


class ImportLockTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.registry = self.root / "docs/upstream/upstream-registry.json"
        self.registry.parent.mkdir(parents=True)
        self.write(copy.deepcopy(EXPECTED))

    def write(self, value):
        self.registry.write_text(json.dumps(value), encoding="utf-8")

    def test_pending_empty_passes(self):
        validate(self.root)

    def test_unapproved_registry_changes_fail(self):
        for key, value in (("project_license_status", "APPROVED"),
                           ("external_import_status", "APPROVED"),
                           ("components", [{"id": "candidate"}]),
                           ("schema_version", True),
                           ("unexpected", "field")):
            with self.subTest(key=key):
                data = copy.deepcopy(EXPECTED)
                data[key] = value
                self.write(data)
                with self.assertRaises(ValueError):
                    validate(self.root)

    def test_duplicate_keys_and_missing_registry_fail(self):
        self.registry.write_text('{"schema_version":1,"schema_version":1}',
                                 encoding="utf-8")
        with self.assertRaises(ValueError):
            validate(self.root)
        self.registry.unlink()
        with self.assertRaises(ValueError):
            validate(self.root)

    def test_reserved_paths_fail(self):
        for name in RESERVED:
            with self.subTest(name=name):
                path = self.root / name
                path.write_text("fixture", encoding="utf-8")
                with self.assertRaises(ValueError):
                    validate(self.root)
                path.unlink()
                path.mkdir()
                with self.assertRaises(ValueError):
                    validate(self.root)
                path.rmdir()
                path.symlink_to(self.root / "missing")
                with self.assertRaises(ValueError):
                    validate(self.root)
                path.unlink()

    def test_registry_symlink_fails(self):
        target = self.root / "other.json"
        self.registry.rename(target)
        self.registry.symlink_to(target)
        with self.assertRaises(ValueError):
            validate(self.root)


if __name__ == "__main__":
    unittest.main()
