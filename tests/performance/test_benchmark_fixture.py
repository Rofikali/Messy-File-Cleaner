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

    def test_builder_refuses_existing_fixture_without_overwrite(self):
        fixture = self.root / "set-0000" / "fixture-00000000.jpg"
        fixture.write_bytes(b"preserve existing payload")
        with self.assertRaises(FileExistsError):
            build_fixture(self.root, 4)
        self.assertEqual(fixture.read_bytes(), b"preserve existing payload")

    def test_builder_refuses_symlink_without_touching_external_target(self):
        outside = self.root.parent / (self.root.name + "-builder-target")
        outside.write_bytes(b"external preserved")
        self.addCleanup(outside.unlink)
        fixture = self.root / "set-0000" / "fixture-00000000.jpg"
        fixture.unlink()
        fixture.symlink_to(outside)
        with self.assertRaises(FileExistsError):
            build_fixture(self.root, 4)
        self.assertEqual(outside.read_bytes(), b"external preserved")

    def test_builder_rejects_symlinked_directory_and_preserves_external(self):
        outside = self.root.parent / (self.root.name + "-builder-directory")
        outside.mkdir()
        self.addCleanup(outside.rmdir)
        alias = self.root / "set-0000"
        # This test uses an empty external directory; no customer data is touched.
        for fixture in alias.iterdir():
            fixture.unlink()
        alias.rmdir()
        alias.symlink_to(outside, target_is_directory=True)
        with self.assertRaisesRegex(RuntimeError, "fixture directory is a symlink"):
            build_fixture(self.root, 1)
        self.assertEqual(list(outside.iterdir()), [])

    def test_builder_rejects_symlinked_root_without_external_creation(self):
        with tempfile.TemporaryDirectory(prefix="benchmark-builder-root-") as temp:
            outside = Path(temp)
            alias = self.root / "builder-root-alias"
            alias.symlink_to(outside, target_is_directory=True)
            with self.assertRaisesRegex(RuntimeError, "fixture root"):
                build_fixture(alias, 1)
            self.assertEqual(list(outside.iterdir()), [])

    def test_later_symlink_directory_rejected_before_any_fixture_creation(self):
        with tempfile.TemporaryDirectory(prefix="benchmark-preflight-later-") as temp:
            root = Path(temp)
            outside = root / "external-preserved"
            outside.mkdir()
            (outside / "sentinel").write_bytes(b"preserve")
            (root / "set-0001").symlink_to(outside, target_is_directory=True)
            with self.assertRaisesRegex(RuntimeError, "fixture directory is a symlink"):
                build_fixture(root, 101)
            self.assertFalse((root / "set-0000").exists())
            self.assertEqual((outside / "sentinel").read_bytes(), b"preserve")

    def test_later_regular_file_directory_rejected_before_writes(self):
        with tempfile.TemporaryDirectory(prefix="benchmark-preflight-file-") as temp:
            root = Path(temp)
            blocker = root / "set-0001"
            blocker.write_bytes(b"preserve blocker")
            with self.assertRaisesRegex(RuntimeError, "not a real directory"):
                build_fixture(root, 101)
            self.assertFalse((root / "set-0000").exists())
            self.assertEqual(blocker.read_bytes(), b"preserve blocker")

    def test_later_existing_fixture_rejected_before_earlier_creation(self):
        with tempfile.TemporaryDirectory(prefix="benchmark-preflight-collision-") as temp:
            root = Path(temp)
            later = root / "set-0001"
            later.mkdir()
            preserved = later / "fixture-00000100.jpg"
            preserved.write_bytes(b"preserve existing")
            with self.assertRaises(FileExistsError):
                build_fixture(root, 101)
            self.assertFalse((root / "set-0000").exists())
            self.assertEqual(preserved.read_bytes(), b"preserve existing")

    def test_later_symlink_fixture_rejected_before_earlier_creation(self):
        with tempfile.TemporaryDirectory(prefix="benchmark-preflight-alias-") as temp:
            root = Path(temp)
            later = root / "set-0001"
            later.mkdir()
            outside = root / "external-sentinel"
            outside.write_bytes(b"preserve external")
            (later / "fixture-00000100.jpg").symlink_to(outside)
            with self.assertRaises(FileExistsError):
                build_fixture(root, 101)
            self.assertFalse((root / "set-0000").exists())
            self.assertEqual(outside.read_bytes(), b"preserve external")

    def test_later_unexpected_file_rejected_before_earlier_creation(self):
        with tempfile.TemporaryDirectory(prefix="benchmark-unexpected-file-") as temp:
            root = Path(temp)
            later = root / "set-0001"
            later.mkdir()
            sentinel = later / "unrelated.txt"
            sentinel.write_bytes(b"preserve unrelated")
            with self.assertRaisesRegex(FileExistsError, "not empty"):
                build_fixture(root, 101)
            self.assertFalse((root / "set-0000").exists())
            self.assertEqual(sentinel.read_bytes(), b"preserve unrelated")

    def test_later_unexpected_directory_rejected_without_mutation(self):
        with tempfile.TemporaryDirectory(prefix="benchmark-unexpected-dir-") as temp:
            root = Path(temp)
            nested = root / "set-0001" / "nested"
            nested.mkdir(parents=True)
            sentinel = nested / "sentinel"
            sentinel.write_bytes(b"preserve nested")
            with self.assertRaisesRegex(FileExistsError, "not empty"):
                build_fixture(root, 101)
            self.assertFalse((root / "set-0000").exists())
            self.assertEqual(sentinel.read_bytes(), b"preserve nested")

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

    def test_expected_directory_symlink_rejected_before_file_reads(self):
        original = self.root / "set-0000"
        sibling = self.root / "preserved"
        original.rename(sibling)
        original.symlink_to(sibling, target_is_directory=True)
        with self.assertRaisesRegex(RuntimeError, "fixture directory"):
            verify_fixture(self.root, 4)
        self.assertEqual((sibling / "fixture-00000000.jpg").read_bytes(), b"benchmark fixture\n")

    def test_external_symlink_rejected_and_target_preserved(self):
        outside = self.root.parent / (self.root.name + "-external-target")
        outside.write_bytes(b"external customer-like payload\\n")
        self.addCleanup(outside.unlink)
        fixture = self.root / "set-0000" / "fixture-00000000.jpg"
        fixture.unlink()
        fixture.symlink_to(outside)
        with self.assertRaisesRegex(RuntimeError, "modified or removed"):
            verify_fixture(self.root, 4)
        self.assertEqual(outside.read_bytes(), b"external customer-like payload\\n")

    @unittest.skipUnless(hasattr(__import__("os"), "mkfifo"), "POSIX FIFO required")
    def test_unexpected_fifo_rejected_without_opening_it(self):
        import os
        fifo = self.root / "unexpected-fifo"
        os.mkfifo(fifo)
        with self.assertRaisesRegex(RuntimeError, "unexpected entry"):
            verify_fixture(self.root, 4)
        self.assertTrue(fifo.exists())

    def test_hardlinked_fixture_rejected_and_external_payload_preserved(self):
        import os
        outside = self.root.parent / (self.root.name + "-hardlink-target")
        outside.write_bytes(b"benchmark fixture\n")
        self.addCleanup(outside.unlink)
        fixture = self.root / "set-0000" / "fixture-00000000.jpg"
        fixture.unlink()
        os.link(outside, fixture)
        with self.assertRaisesRegex(RuntimeError, "modified or removed"):
            verify_fixture(self.root, 4)
        self.assertEqual(outside.read_bytes(), b"benchmark fixture\n")

    @unittest.skipUnless(hasattr(__import__("os"), "mkfifo"), "POSIX FIFO required")
    def test_expected_fifo_rejected_without_blocking_or_reading(self):
        import os
        fixture = self.root / "set-0000" / "fixture-00000000.jpg"
        fixture.unlink()
        os.mkfifo(fixture)
        with self.assertRaisesRegex(RuntimeError, "modified or removed"):
            verify_fixture(self.root, 4)
        self.assertTrue(fixture.is_fifo())

    @unittest.skipUnless(hasattr(__import__("os"), "mkfifo"), "POSIX FIFO required")
    def test_builder_refuses_expected_fifo_without_opening_it(self):
        import os
        with tempfile.TemporaryDirectory(prefix="benchmark-fifo-builder-") as temp:
            root = Path(temp)
            directory = root / "set-0000"
            directory.mkdir()
            fifo = directory / "fixture-00000000.jpg"
            os.mkfifo(fifo)
            with self.assertRaises(FileExistsError):
                build_fixture(root, 1)
            self.assertTrue(fifo.is_fifo())

    @unittest.skipUnless(hasattr(__import__("socket"), "AF_UNIX"), "POSIX Unix socket required")
    def test_expected_unix_socket_rejected_without_connecting(self):
        import socket
        fixture = self.root / "set-0000" / "fixture-00000000.jpg"
        fixture.unlink()
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as listener:
            listener.bind(str(fixture))
            with self.assertRaisesRegex(RuntimeError, "modified or removed"):
                verify_fixture(self.root, 4)
            self.assertTrue(fixture.is_socket())

    @unittest.skipUnless(hasattr(__import__("socket"), "AF_UNIX"), "POSIX Unix socket required")
    def test_unexpected_unix_socket_rejected_and_preserved(self):
        import socket
        unexpected = self.root / "unexpected-socket"
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as listener:
            listener.bind(str(unexpected))
            with self.assertRaisesRegex(RuntimeError, "unexpected entry"):
                verify_fixture(self.root, 4)
            self.assertTrue(unexpected.is_socket())
        self.assertEqual(
            (self.root / "set-0000" / "fixture-00000001.png").read_bytes(),
            b"benchmark fixture\n",
        )

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
        # The setUp fixture already contains four files; exclusive creation
        # intentionally refuses to overwrite them. Use a fresh disposable root.
        with tempfile.TemporaryDirectory(prefix="benchmark-multidir-test-") as temp:
            root = Path(temp)
            build_fixture(root, 101)
            verify_fixture(root, 101)

    def test_symlink_replacement_rejected(self):
        original = self.root / "set-0000" / "fixture-00000000.jpg"
        original.unlink()
        original.symlink_to(self.root / "set-0000" / "fixture-00000001.png")
        with self.assertRaisesRegex(RuntimeError, "modified or removed"):
            verify_fixture(self.root, 4)


if __name__ == "__main__":
    unittest.main()
