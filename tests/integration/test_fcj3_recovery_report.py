#!/usr/bin/env python3
"""Black-box safety tests for the isolated FCJ3 read-only report CLI."""

import json
import os
import pathlib
import subprocess
import sys
import tempfile


def write_file(path: pathlib.Path, data: bytes = b"fixture") -> None:
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    try:
        os.write(fd, data)
    finally:
        os.close(fd)


def record(operation_id: str, sequence: int, state: str,
           source: pathlib.Path, destination: pathlib.Path,
           stage_name: str, directory_stat: os.stat_result,
           stage_stat: os.stat_result) -> bytes:
    source_bytes = os.fsencode(str(source))
    destination_bytes = os.fsencode(str(destination))
    stage_bytes = os.fsencode(stage_name)
    fields = [
        "FCJ3", "1", operation_id, str(sequence), state, "0",
        str(len(source_bytes)), str(len(destination_bytes)),
        str(directory_stat.st_dev), str(directory_stat.st_ino),
        str(stage_stat.st_dev), str(stage_stat.st_ino),
        str(len(stage_bytes)), source_bytes.hex(), destination_bytes.hex(),
        stage_bytes.hex(),
    ]
    return ("|".join(fields) + "\n").encode("ascii")


def run(binary: str, journal: pathlib.Path) -> subprocess.CompletedProcess:
    return subprocess.run(
        [binary, str(journal)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


def main() -> int:
    binary = os.path.abspath(sys.argv[1])
    with tempfile.TemporaryDirectory(prefix="fcj3-report-test-") as temp:
        root = pathlib.Path(temp)
        source = root / 'source-"quoted".bin'
        destination = root / "destination.bin"
        stage_name = ".fc-tmp-report"
        stage = root / stage_name
        journal = root / "move.fcj3"
        write_file(source, b"source-data")
        write_file(stage, b"staged-data")

        root_stat = os.stat(root)
        stage_stat = os.stat(stage, follow_symlinks=False)
        operation_id = "0123456789abcdef0123456789abcdef"
        journal_bytes = (
            record(operation_id, 1, "STAGED", source, destination,
                   stage_name, root_stat, stage_stat)
            + record(operation_id, 2, "STARTED", source, destination,
                     stage_name, root_stat, stage_stat)
        )
        journal.write_bytes(journal_bytes)

        before_entries = sorted(p.name for p in root.iterdir())
        before_source = source.read_bytes()
        before_stage = stage.read_bytes()
        before_journal = journal.read_bytes()

        completed = run(binary, journal)
        if completed.returncode != 0:
            raise AssertionError(
                f"valid report failed: {completed.stderr.decode(errors='replace')}"
            )
        report = json.loads(completed.stdout)
        assert report["schema_version"] == 1
        assert report["read_only"] is True
        assert report["changes_made"] is False
        assert report["transaction_count"] == 1
        transaction = report["transactions"][0]
        assert transaction["state"] == "STARTED"
        assert transaction["source"] == str(source)
        assert transaction["stage_evidence"] == "MATCHING_REGULAR"
        assert transaction["destination_evidence"] == "ABSENT"
        assert transaction["source_evidence"] == "PRESENT_ADVISORY_ONLY"
        assert transaction["classification"] == "PRESERVE_INCOMPLETE"
        assert transaction["reason"] == "NONE"

        # A malformed suffix must reject the whole journal and emit no JSON.
        journal.write_bytes(journal_bytes + b"not-a-valid-record\n")
        malformed = run(binary, journal)
        assert malformed.returncode != 0
        assert malformed.stdout == b""

        # A final-component symlink must not be followed.
        journal.unlink()
        journal.symlink_to(source)
        symlink_journal = run(binary, journal)
        assert symlink_journal.returncode != 0
        assert symlink_journal.stdout == b""

        # An intermediate symlink must not be traversed either.
        link_dir = root / "linked-directory"
        link_dir.symlink_to(root, target_is_directory=True)
        linked_path = link_dir / "move.fcj3"
        linked_path.write_bytes(journal_bytes)
        intermediate = run(binary, linked_path)
        assert intermediate.returncode != 0
        assert intermediate.stdout == b""

        # Reporting must not change fixture contents or directory entries.
        assert source.read_bytes() == before_source
        assert stage.read_bytes() == before_stage
        assert sorted(p.name for p in root.iterdir()) == sorted(
            before_entries + ["linked-directory", "move.fcj3"]
        )
        # Journal was intentionally replaced for malformed/symlink tests.
        assert journal.is_symlink()\n        assert (root / "move.fcj3").read_bytes() == journal_bytes\n        assert before_journal == journal_bytes

    print("FCJ3 read-only recovery report integration tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
