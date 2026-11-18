"""Regression tests for disposable benchmark filesystem metadata."""

import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent))
from benchmark_cli import filesystem_context


class FilesystemContextTests(unittest.TestCase):
    def test_real_disposable_workspace(self):
        with tempfile.TemporaryDirectory(prefix="perf-fs-context-") as temp:
            result = filesystem_context(Path(temp))
        self.assertGreater(result["block_size_bytes"], 0)
        self.assertGreaterEqual(result["capacity_bytes"], result["available_bytes"])
        self.assertGreaterEqual(result["available_bytes"], 0)
        self.assertGreaterEqual(result["device_id"], 0)
        if result["total_inodes"] is not None:
            self.assertGreaterEqual(result["total_inodes"], result["available_inodes"])

    def test_uses_fragment_size_and_available_not_free_blocks(self):
        stats = SimpleNamespace(f_frsize=4096, f_bsize=8192, f_blocks=100,
                                f_bavail=20, f_bfree=30, f_files=50, f_favail=10)
        with tempfile.TemporaryDirectory() as temp:
            with patch("benchmark_cli.os.statvfs", return_value=stats):
                result = filesystem_context(Path(temp))
        self.assertEqual(result["block_size_bytes"], 4096)
        self.assertEqual(result["capacity_bytes"], 409600)
        self.assertEqual(result["available_bytes"], 81920)
        self.assertEqual(result["available_inodes"], 10)

    def test_unreported_inode_capacity_is_null(self):
        stats = SimpleNamespace(f_frsize=0, f_bsize=512, f_blocks=10,
                                f_bavail=5, f_files=0, f_favail=0)
        with tempfile.TemporaryDirectory() as temp:
            with patch("benchmark_cli.os.statvfs", return_value=stats):
                result = filesystem_context(Path(temp))
        self.assertEqual(result["block_size_bytes"], 512)
        self.assertIsNone(result["total_inodes"])
        self.assertIsNone(result["available_inodes"])

    def test_invalid_block_counters_rejected(self):
        stats = SimpleNamespace(f_frsize=4096, f_bsize=4096, f_blocks=10,
                                f_bavail=-1, f_files=10, f_favail=5)
        with tempfile.TemporaryDirectory() as temp:
            with patch("benchmark_cli.os.statvfs", return_value=stats):
                with self.assertRaisesRegex(RuntimeError, "capacity"):
                    filesystem_context(Path(temp))

    def test_available_blocks_cannot_exceed_capacity(self):
        stats = SimpleNamespace(f_frsize=4096, f_bsize=4096, f_blocks=10,
                                f_bavail=11, f_files=10, f_favail=5)
        with tempfile.TemporaryDirectory() as temp:
            with patch("benchmark_cli.os.statvfs", return_value=stats):
                with self.assertRaisesRegex(RuntimeError, "capacity"):
                    filesystem_context(Path(temp))

    def test_available_inodes_cannot_exceed_total(self):
        stats = SimpleNamespace(f_frsize=4096, f_bsize=4096, f_blocks=10,
                                f_bavail=5, f_files=10, f_favail=11)
        with tempfile.TemporaryDirectory() as temp:
            with patch("benchmark_cli.os.statvfs", return_value=stats):
                with self.assertRaisesRegex(RuntimeError, "inode"):
                    filesystem_context(Path(temp))

    def test_invalid_inode_counters_rejected(self):
        stats = SimpleNamespace(f_frsize=4096, f_bsize=4096, f_blocks=10,
                                f_bavail=5, f_files=10, f_favail=-1)
        with tempfile.TemporaryDirectory() as temp:
            with patch("benchmark_cli.os.statvfs", return_value=stats):
                with self.assertRaisesRegex(RuntimeError, "inode"):
                    filesystem_context(Path(temp))


if __name__ == "__main__":
    unittest.main()
