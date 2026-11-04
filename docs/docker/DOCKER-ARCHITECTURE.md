# Docker Runtime Architecture

## Goal
Provide a reproducible Linux runtime for the POSIX C CLI without changing filesystem behavior or introducing a network service.

## Runtime layout

| Container path | Purpose | Host mount |
| --- | --- | --- |
| /input | Files to classify and move | ./input |
| /output | Categorized folders | ./output |
| /output/journal | Persistent journal and recovery state | ./state |
| /app/configs | Configuration | Included in image |
| /usr/local/bin/file-cleaner | Executable | Included in image |

The working directory is /output. The current config uses relative category destinations and the journal path is journal/move.log. This layout keeps output and journal persistent while giving the journal a separate host directory.

## Security controls
- Multi-stage build; compiler tools are absent from runtime.
- Non-root runtime identity.
- Read-only container root filesystem.
- Only input, output, and journal mounts are writable.
- All Linux capabilities dropped; no-new-privileges enabled.
- Network disabled for this local filesystem workload.
- PID and memory limits applied.
- Compose defaults to dry-run. Remove that option only after reviewing the plan.

The input mount must be writable for real moves. Never mount valuable files until the dry-run has been reviewed and a backup exists.

## Why .devcontainer remains
The devcontainer defines the developer workspace. Dockerfile and Compose define the application runtime. They solve different problems.

## Known limitations
- The program currently hard-codes a relative journal path and the Others destination. This Compose layout accommodates them; a future API milestone should make state and destination roots explicit.
- The CLI has no dedicated execute flag. Compose defaults to dry-run; execution is an explicit command override.
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

    docker compose run --rm cleaner /input --config /app/configs/default.conf --once

This removes the default dry-run argument and performs real moves. Use disposable files only.

## Permissions
The image defaults to UID/GID 10001. On Linux, make the three directories writable by that identity or set PUID and PGID to an appropriate non-root identity:

    PUID=$(id -u) PGID=$(id -g) docker compose run --rm cleaner

On Docker Desktop, verify bind-mount permissions with disposable files first.
