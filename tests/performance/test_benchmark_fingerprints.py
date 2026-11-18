"""Benchmark input fingerprint reproducibility regression tests."""

import hashlib
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from benchmark_cli import sha256_file, verify_input_fingerprints


class InputFingerprintTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="perf-input-fingerprint-")
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.binary = root / "binary"
        self.config = root / "config"
        self.binary.write_bytes(b"test executable")
        self.config.write_bytes(b"test config")
        self.expected = {
            "binary_sha256": sha256_file(self.binary),
            "config_sha256": sha256_file(self.config),
        }

    def test_known_digest(self):
        self.assertEqual(sha256_file(self.binary),
                         hashlib.sha256(b"test executable").hexdigest())

    def test_empty_file_digest(self):
        self.config.write_bytes(b"")
        self.assertEqual(sha256_file(self.config), hashlib.sha256(b"").hexdigest())

    def test_unchanged_inputs_accepted(self):
        verify_input_fingerprints(self.binary, self.config, self.expected)

    def test_binary_mutation_rejected(self):
        self.binary.write_bytes(b"tampered executable")
        with self.assertRaisesRegex(RuntimeError, "executable changed"):
            verify_input_fingerprints(self.binary, self.config, self.expected)

    def test_config_mutation_rejected(self):
        self.config.write_bytes(b"tampered config")
        with self.assertRaisesRegex(RuntimeError, "configuration changed"):
            verify_input_fingerprints(self.binary, self.config, self.expected)

    def test_missing_input_rejected(self):
        self.config.unlink()
        with self.assertRaises(FileNotFoundError):
            verify_input_fingerprints(self.binary, self.config, self.expected)

    def test_large_file_streaming_digest(self):
        payload = b"abcdef0123456789" * 140000
        self.binary.write_bytes(payload)
        self.assertEqual(sha256_file(self.binary), hashlib.sha256(payload).hexdigest())


if __name__ == "__main__":
    unittest.main()
