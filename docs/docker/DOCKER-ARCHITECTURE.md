# Docker Runtime Architecture

## Goal
Provide a reproducible Linux runtime for the POSIX C CLI without changing filesystem behavior or introducing a network service.

## Runtime layout

| Container path | Purpose | Host mount |
| --- | --- | --- |
| /input | Files to classify and move | ./input (read-only by default; explicit execution override only) |
| /output | Categorized folders | ./output |
| /output/journal | Persistent journal and recovery state | ./state |
| /app/configs | Configuration | Included in image |
| /usr/local/bin/file-cleaner | Executable | Included in image |

The working directory is /output. The current config uses relative category destinations and the journal path is journal/move.log. This layout keeps output and journal persistent while giving the journal a separate host directory.

## Security controls
- Multi-stage build; compiler tools are absent from runtime.
- Non-root runtime identity.
- Read-only container root filesystem.
- Default dry-run: `/input` is **read-only**; `/output` and `/output/journal` are writable mounts. The separate `compose.execute.yaml` intentionally allows writing to `/input` only for explicit real-move operation.
- All Linux capabilities dropped; no-new-privileges enabled.
- Network disabled for this local filesystem workload.
- PID and memory limits applied.
- Compose defaults to dry-run **and read-only `/input`**. Real execution requires explicitly using the separate writable-input execution overlay after reviewing the plan.

Source retirement needs a writable input mount **only for a real move**. Default Compose runs a dry-run with the input mounted read-only. The explicit `compose.execute.yaml` overlay replaces that source mount with read/write and removes `--dry-run`. Never use the execution overlay for preview; back up input and inspect the dry-run before any real moves.

## Why .devcontainer remains
The devcontainer defines the developer workspace. Dockerfile and Compose define the application runtime. They solve different problems.

## Known limitations
- The program currently hard-codes a relative journal path and the Others destination. This Compose layout accommodates them; a future API milestone should make state and destination roots explicit.
- The CLI has no dedicated execute flag. Default Compose is dry-run plus read-only input; execution requires deliberately selecting the separately named writable-input overlay. Direct Docker/CLI invocations remain a separate trust boundary.
- Watch mode and inotify on Docker Desktop bind mounts are not covered by this first milestone.
- Container isolation does not replace path validation inside the application.
- This is not an HTTP API service and exposes no ports.

## Build and dry-run
Create input, output, and state directories, put disposable files in input, then run:

    docker compose build --no-cache
    docker compose run --rm cleaner

Review planned destinations. Dry-run must not change input files.

## Explicit execution
After backing up test input and reviewing the dry-run:

    docker compose -f compose.yaml -f compose.execute.yaml config
    docker compose -f compose.yaml -f compose.execute.yaml run --rm cleaner

This deliberately changes the input mount from read-only to writable and removes `--dry-run`. It performs real moves, potentially removing input files. Use disposable fixtures, verified backups and reviewed configuration only.

## Permissions
The image defaults to UID/GID 10001. On Linux, make the three directories writable by that identity or set PUID and PGID to an appropriate non-root identity:

    PUID=$(id -u) PGID=$(id -g) docker compose run --rm cleaner

On Docker Desktop, verify bind-mount permissions with disposable files first.
