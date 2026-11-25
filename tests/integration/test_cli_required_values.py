#!/usr/bin/env python3
"""CLI-VALUES-01: malformed path operands fail without configuration or filesystem writes."""
import os
import pathlib
import stat
import subprocess
import sys
import tempfile


def main():
    binary = str(pathlib.Path(sys.argv[1]).resolve())
    cases = [
        ["--root"], ["--root", ""], ["--root", "--dry-run"],
        ["--config"], ["--config", ""], ["--config", "--once"],
        [""], ["--root", "valid", "--config", "--dry-run"],
        ["valid", "--workers", "1", "--workers", "2"],
        ["valid", "--workers", "2", "--workers", "2"],
        ["--workers", "1", "--root", "valid", "--workers", "16"],
    ]
    with tempfile.TemporaryDirectory(prefix="fc-cli-values-") as tmp:
        workspace = pathlib.Path(tmp)
        for args in cases:
            result = subprocess.run([binary, *args], cwd=tmp,
                                    capture_output=True, text=True,
                                    timeout=5, check=False)
            assert result.returncode != 0, (args, result.returncode)
            assert "Usage:" in result.stderr, (args, result.stderr)
            assert "Config load failed" not in result.stdout, (args, result.stdout)
            assert not (workspace / "journal").exists(), args
            assert not (workspace / "configs").exists(), args

        # No writer is opened: a blocking FIFO config would time out.
        source = workspace / "source"
        source.mkdir()
        fixture = source / "keep.txt"
        fixture.write_bytes(b"preserve")
        fifo = workspace / "config.fifo"
        os.mkfifo(fifo, 0o600)
        fifo_link = workspace / "fifo-link.conf"
        fifo_link.symlink_to(fifo)
        for config_path in (fifo, fifo_link, source):
            result = subprocess.run(
                [binary, str(source), "--config", str(config_path), "--dry-run", "--once"],
                cwd=tmp, capture_output=True, text=True, timeout=5, check=False)
            assert result.returncode != 0, result
            assert "config: expected a regular file" in result.stderr, result.stderr
            assert "Config load failed" in result.stdout, result.stdout
            assert fixture.read_bytes() == b"preserve"
            assert stat.S_ISFIFO(fifo.stat().st_mode) and fifo_link.is_symlink()
            assert not (workspace / "journal").exists()
            assert not (workspace / "Others").exists()
        # A regular configuration reached through a symlink remains supported.
        config = workspace / "valid.conf"
        config.write_text("mode=batch\nworker_threads=1\n")
        config_link = workspace / "valid-link.conf"
        config_link.symlink_to(config)
        result = subprocess.run(
            [binary, str(source), "--config", str(config_link), "--dry-run", "--once"],
            cwd=tmp, capture_output=True, text=True, timeout=5, check=False)
        assert result.returncode == 0, (result.stdout, result.stderr)
        assert fixture.read_bytes() == b"preserve"
        assert not (workspace / "journal").exists()
        assert not (workspace / "Others").exists()


if __name__ == "__main__":
    main()
