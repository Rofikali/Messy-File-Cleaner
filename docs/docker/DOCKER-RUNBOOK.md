# Docker Setup and Safe Operation

## Verification scope as of 2026-11-21

GitHub-hosted Ubuntu 24.04 validation and Docker build/runtime smoke workflows passed on main `3d7e14ed16040d6918f5d88330c510db77be0b96`. This is evidence for those **configured CI jobs**, not proof of an owner-run local Docker Desktop/Ubuntu Compose session, different host mount permissions, malicious same-UID interference, or real-file recovery. The five-PR [delivery policy](../FEATURE_DELIVERY_AND_ACCEPTANCE_POLICY.md) does not override the FS-01B P0 gate. Use only disposable fixture previews and do not run the execution override as an acceptance shortcut.

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

For explicit execution, first review the dry-run results, back up the input and preferably use disposable fixtures. Opt into the separate writable-source configuration.

**FS-01B-GUARD permission contract (review candidate):** real source retirement will reject an input parent directory not owned by the container's effective UID, or writable by group/other, with `EPERM`. This intentional fail-closed change may reject previously working shared-writable mounts. Check the input directory's owner and mode first; never use root or change permissions on customer-owned trees just to bypass it. For disposable, **already-owned** fixture directories, matching your host UID/GID in Compose and using a private input directory supports the positive smoke:

    stat -c '%u %a %n' input output state
    id -u
    id -g
    # For private, owned test directories only, if you elect to restrict them:
    # chmod 0700 input output state

    PUID="$(id -u)" PGID="$(id -g)" docker compose -f compose.yaml -f compose.execute.yaml config
    PUID="$(id -u)" PGID="$(id -g)" docker compose -f compose.yaml -f compose.execute.yaml run --rm cleaner

This is not protection against a **same-UID** process that can mutate source directory entries, nor a substitute for backup/recovery validation. See [source-retirement guard and unresolved risks](../security/FS_01B_SHARED_SOURCE_GUARD.md).

**Warning: the second command performs real moves and removes successfully processed files from `./input`.** The override deliberately changes `/input` to read/write and drops `--dry-run`, but preserves the non-root identity, network isolation, read-only root filesystem, capabilities, and resource limits inherited from `compose.yaml`. Do not use `compose.execute.yaml` for an ordinary preview. It is an explicit opt-in configuration, not a transactional safety guarantee or backup replacement.

Before running real moves, also confirm that the input path and destination/state paths are not overlapping and that the input contains only files you're willing to modify.

## Persistent state
- Categorized files: output/
- Journal: state/move.log

The journal records paths and should be treated as operationally sensitive.

## Troubleshooting
- Permission denied: first check whether the input directory is owned by the container effective UID and has no group/other write bits. For **disposable, privately owned fixtures**, match the host's UID/GID with PUID/PGID and restrict directory modes; do not broaden permissions or run as root to bypass the guard. If authority cannot be established, **do not move files**.
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
