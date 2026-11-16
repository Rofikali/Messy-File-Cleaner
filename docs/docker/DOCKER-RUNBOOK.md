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

The default `compose.yaml` mounts `/input` **read-only**. Merely removing `--dry-run` from the normal command does **not** enable real moves. Do not try to work around that safeguard by modifying the default Compose file.

For explicit execution, first review the dry-run results, back up the input and preferably use disposable fixtures. Opt into the separate writable-source configuration:

    docker compose -f compose.yaml -f compose.execute.yaml config
    docker compose -f compose.yaml -f compose.execute.yaml run --rm cleaner

**Warning: the second command performs real moves and removes successfully processed files from `./input`.** The override deliberately changes `/input` to read/write and drops `--dry-run`, but preserves the non-root identity, network isolation, read-only root filesystem, capabilities, and resource limits inherited from `compose.yaml`. Do not use `compose.execute.yaml` for an ordinary preview. It is an explicit opt-in configuration, not a transactional safety guarantee or backup replacement.

Before running real moves, also confirm that the input path and destination/state paths are not overlapping and that the input contains only files you're willing to modify.

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
- [ ] Dry-run output reviewed; default `/input` is read-only.
- [ ] For execution only: explicit `compose.execute.yaml` selected, backup verified, and writable-source operation understood.
- [ ] Container runs non-root.
- [ ] No host root, Docker socket, or unrelated paths mounted.
- [ ] Journal directory is persistent.
