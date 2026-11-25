#!/usr/bin/env python3
"""CLI-ERROR-01: bounded, non-echoing argument diagnostics without filesystem writes."""
import pathlib
import subprocess
import sys
import tempfile

def main():
    binary = str(pathlib.Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="fc-cli-error-") as tmp:
        root = pathlib.Path(tmp)
        cases = [
            (["--unexpected-option"], "unrecognized option"),
            (["first", "second"], "multiple input roots"),
            (["first", "--bad-\x1b[31m"], "unrecognized option"),
        ]
        for args, expected in cases:
            result = subprocess.run([binary, *args], cwd=tmp, capture_output=True,
                                    text=True, timeout=5, check=False)
            assert result.returncode != 0, (args, result.returncode)
            assert expected in result.stderr, (args, result.stderr)
            assert "Usage:" in result.stderr, (args, result.stderr)
            assert "\x1b" not in result.stderr, result.stderr
            assert not (root / "journal").exists()
            assert not (root / "configs").exists()
            assert "Config load failed" not in result.stdout

    # argv[0] is caller-controlled via execve; never print escape sequences from it.
    with tempfile.TemporaryDirectory(prefix="fc-cli-argv0-") as tmp:
        forged = "spoofed-\x1b[31m-program"
        result = subprocess.run([forged, "--invalid"], executable=binary,
                                cwd=tmp, capture_output=True, text=True,
                                timeout=5, check=False)
        assert result.returncode != 0
        assert "Usage: file-cleaner" in result.stderr
        assert "spoofed" not in result.stderr
        assert "\x1b" not in result.stderr
        assert not (pathlib.Path(tmp) / "journal").exists()

    # Root/output validation runs after config loading; rejected paths must not
    # appear in terminal diagnostics, including names containing control bytes.
    with tempfile.TemporaryDirectory(prefix="fc-root-output-error-") as tmp:
        base = pathlib.Path(tmp)
        work = base / "work"
        work.mkdir()
        config = base / "config.conf"
        config.write_text("mode=batch\nworker_threads=1\next.jpg=Jpeg/\n")
        marker = "private-root\n\x1b[31m"
        missing = base / (marker + "-missing")
        regular = base / (marker + "-file")
        regular.write_bytes(b"preserve-file")
        source = base / "source"
        source.mkdir()
        sample = source / "sample.jpg"
        sample.write_bytes(b"preserve-source")
        link = base / (marker + "-link")
        link.symlink_to(source, target_is_directory=True)
        overlap = base / (marker + "-overlap")
        overlap.mkdir()
        nested_work = overlap / "work"
        nested_work.mkdir()

        def reject(root, cwd, expected):
            result = subprocess.run([binary, "--root", str(root), "--config",
                                     str(config), "--dry-run", "--once"],
                                    cwd=cwd, capture_output=True, text=True,
                                    timeout=5, check=False)
            assert result.returncode != 0, result
            assert expected in result.stderr, result.stderr
            assert "\x1b" not in result.stderr, result.stderr
            assert "private-root" not in result.stderr, result.stderr
            assert not (cwd / "journal").exists()
            assert sample.read_bytes() == b"preserve-source"
            assert regular.read_bytes() == b"preserve-file"

        reject(missing, work, "not accessible")
        reject(regular, work, "must be a directory")
        reject(link, work, "symlink input root")
        reject(overlap, nested_work, "overlaps output directory")
        hostile_work = base / (marker + "-work")
        hostile_work.mkdir()
        (hostile_work / "Jpeg").symlink_to(source, target_is_directory=True)
        reject(source, hostile_work, "real directory")
        assert (hostile_work / "Jpeg").is_symlink()
        assert link.is_symlink()

if __name__ == "__main__":
    main()
