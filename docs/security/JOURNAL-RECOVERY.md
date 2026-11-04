# Journal format, locking, and recovery

## Journal format

New records use the versioned FCJ2 format:

`FCJ2|timestamp_ns|state|error_code|source_length|destination_length|source_hex|destination_hex`

Paths are hex-encoded so that pipes, newlines, and other non-NUL filename bytes cannot corrupt record boundaries. Parsing is bounded, validates lengths and state names, and rejects malformed or truncated lines. Legacy pipe-delimited records are accepted only when their fields are unambiguous; legacy paths containing a pipe are rejected rather than guessed.

States are STARTED, PUBLISHED, COMPLETED, FAILED, and RECOVERED. The mover records PUBLISHED only after the destination file and directory have been synchronized, before it attempts to retire the source.

## Recovery policy

Recovery is deliberately conservative and idempotent:

- COMPLETED, FAILED, and RECOVERED records are terminal and are not replayed.
- STARTED with a source present and destination absent is recorded as a safe no-op; the source remains available for a future scan.
- STARTED with both source and destination present preserves both. It does not assume the destination belongs to this operation.
- STARTED with a missing source and existing destination is ambiguous and stops startup.
- PUBLISHED with destination present and source absent is recorded as recovered.
- PUBLISHED with both present preserves both copies; it never deletes a source based only on path strings.
- PUBLISHED with source present and destination absent preserves the source and records the missing destination condition.
- Both paths missing, a symlink, a special file, malformed input, or an unrecognized state stops recovery.

Recovery never deletes temporary files or guesses ownership from a filename. Ambiguous state requires operator investigation.

## Durability and process coordination

- Each record is flushed and fsynced before the call reports success.
- Journal files are opened without following a final-component symlink, must be regular files, and are restricted to mode 0600.
- An exclusive flock prevents two application processes from concurrently writing/recovering the same journal.
- Recovery holds the exclusive lock while parsing and appending its RECOVERED decisions.
- `journal_close` reports flush, fsync, and close failures; normal shutdown propagates failure to the process exit status.

## Known limitations

- The journal is a local single-file log, not a distributed transaction coordinator.
- Directory creation for nested journal parents is currently single-level; deployment should use the configured existing journal directory.
- Recovery preserves ambiguous duplicates rather than automatically reconciling them. This favors data preservation over automatic cleanup.
- Legacy records whose path contains `|` cannot be parsed unambiguously and cause fail-closed recovery. Back up the journal and manually reconcile such entries before upgrading.
- Journal compaction, rotation, checksums, and operator-facing repair tooling are future work.
