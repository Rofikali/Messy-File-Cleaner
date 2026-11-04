# Docker Setup and Safe Operation

## Prerequisites
- Docker Engine or Docker Desktop with Compose v2
- Linux shell or Docker Desktop terminal
- Backup of files that may be moved

## Prepare
From repository root:

    mkdir -p input output state

Place only disposable test files in input. Keep output outside input.

## Build and dry-run

    docker compose build
    docker compose run --rm cleaner

The default command includes dry-run and batch-once. Confirm expected categories and verify source files remain in input.

## Execute only after review

    docker compose run --rm cleaner /input --config /app/configs/default.conf --once

This performs real moves. Successfully processed files are removed from input. Use a backup and disposable test set.

## Persistent state
- Categorized files: output/
- Journal: state/move.log

The journal records paths and should be treated as operationally sensitive.

## Troubleshooting
- Permission denied: set PUID/PGID to a non-root identity that can write the test directories. Do not run as root to bypass permissions.
- No files found: check input mount and extension mappings in configs/default.conf.
- Watch mode: inotify can behave differently on Windows bind mounts and native Linux. Watch mode requires separate integration testing.

## Safety checklist
- [ ] Input is disposable or backed up.
- [ ] Output is not inside input.
- [ ] Dry-run output reviewed.
- [ ] Container runs non-root.
- [ ] No host root, Docker socket, or unrelated paths mounted.
- [ ] Journal directory is persistent.
