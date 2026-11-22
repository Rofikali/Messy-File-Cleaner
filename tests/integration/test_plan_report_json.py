#!/usr/bin/env python3
"""PLAN-REPORT-01: aggregate JSON is opt-in and does not authorize file moves."""
import json
import pathlib
import subprocess
import sys
import tempfile

binary = str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix="fc-plan-json-") as temp:
    base = pathlib.Path(temp)
    source = base / "source"
    source.mkdir()
    sample = source / "private-example.jpg"
    sample.write_bytes(b"keep-original")
    config = base / "config.conf"
    config.write_text("mode=batch\nworker_threads=1\next.jpg=Jpeg/\n")
    def run(*flags):
        return subprocess.run([binary, "--root", str(source), "--config", str(config), *flags],
                              cwd=base, capture_output=True, text=True, timeout=15, check=False)
    for flags in (("--report-json",), ("--dry-run", "--watch", "--report-json")):
        invalid = run(*flags)
        assert invalid.returncode != 0, (flags, invalid)
        assert '"schema_version"' not in invalid.stdout
        assert sample.read_bytes() == b"keep-original"
        assert not (base / "journal").exists()
    baseline = run("--dry-run", "--once")
    assert baseline.returncode == 0, (baseline.stdout, baseline.stderr)
    assert '"schema_version"' not in baseline.stdout
    report = run("--dry-run", "--once", "--report-json")
    assert report.returncode == 0, (report.stdout, report.stderr)
    records = [json.loads(line) for line in report.stdout.splitlines() if line.startswith("{")]
    assert len(records) == 1, report.stdout
    record = records[0]
    assert record["schema_version"] == 1
    assert record["kind"] == "dry_run_summary"
    assert record["complete"] is True
    for key in ("events_seen", "events_enqueued", "previewed_moves", "move_failures"):
        assert isinstance(record[key], int) and record[key] >= 0, record
    assert str(sample) not in json.dumps(record)
    assert sample.read_bytes() == b"keep-original"
    assert not (base / "journal").exists()
    assert not (base / "Jpeg").exists()
    assert not (base / "Others").exists()
