# ADR 0003: Unprivileged FCJ3 staging and isolated durable journal

- Status: Proposed for owner review before implementation
- Date: 2026-11-08
- Related: [FCJ3 schema](../../quality/FCJ3_JOURNAL_SCHEMA.md), [FCJ3 writer context](../../quality/FCJ3_WRITER_CONTEXT_DESIGN.md), [staging integration design](../../quality/FCJ3_STAGING_INTEGRATION_DESIGN.md), [threat model](../../security/THREAT_MODEL.md)

## Context

The existing FCJ3 design uses anonymous O_TMPFILE staging and proposes linking with `linkat(fd, "", dirfd, name, AT_EMPTY_PATH)`. Linux documents that AT_EMPTY_PATH requires CAP_DAC_READ_SEARCH. The application is intended to run non-root with least privilege; granting this capability is not acceptable as a default deployment requirement.

The current production journal is `journal/move.log`, written as FCJ2 and legacy records. Its recovery parser does not parse FCJ3. Appending FCJ3 bytes to that file before a unified recovery dispatcher exists would break the format boundary.

## Decision proposed

1. Use `linkat(AT_FDCWD, "/proc/self/fd/<fd>", dirfd, name, AT_SYMLINK_FOLLOW)` as the single supported unprivileged O_TMPFILE linking mechanism. Verify `/proc/self/fd` is procfs and the exact linked inode matches the still-open descriptor. No AT_EMPTY_PATH, capability grant, named-temp fallback, or silent alternative is allowed.
2. Probe the actual validated destination directory under the production UID/mount/security context. Probe O_TMPFILE creation, write/fsync, proc-fd linking, exact inode verification, unlink of the exact probe inode, and directory fsync. Do not cache capability across destinations.
3. Keep this staging/append integration test-only. Use disposable FCJ3 journal files in integration fixtures; do not create a production `journal/move.fcj3` or change the production mover until the separate recovery milestone implements coordinated parsing. At that later point, use a distinct `journal/move.fcj3`; keep `journal/move.log` FCJ2/legacy-only.
4. Use a dedicated append adapter with process mutex + exclusive flock, descriptor-only append, short-write/EINTR handling, fsync, and commit-after-fsync semantics. Any append/fsync failure poisons the context; no in-place repair/truncation.
5. Keep recovery, quarantine, staging cleanup, and production enablement out of this milestone until separately reviewed and tested.

## Alternatives

### Grant CAP_DAC_READ_SEARCH and use AT_EMPTY_PATH

Rejected as a default: it adds a privilege solely to make linking work and conflicts with least privilege.

### Silently fall back from AT_EMPTY_PATH to proc-fd linking

Rejected: runtime behavior should not switch between mechanisms based on failure. The proc-fd method is the selected contract, not a fallback.

### Use a visible named temporary file

Rejected for this protocol: a crash between creation and durable identity would leave an unowned entry that cannot be safely recovered.

### Append FCJ3 to move.log immediately

Rejected: current recovery reads FCJ2/legacy, not FCJ3. Mixed-format append is unsafe before recovery dispatch is implemented.

## Consequences

- Non-root deployments require procfs mounted and accessible for the linking step.
- Some filesystems/security environments will not support the full lifecycle; those destinations fail safely.
- The test phase does not create a production FCJ3 log. Later production enablement introduces a separate FCJ3 log; recovery must inspect both without confusing their formats.
- This phase creates durable records and named staging entries only in disposable test fixtures. Production remains FCJ2 until FCJ3 recovery is implemented. No cleanup is enabled.
- No additional Linux capability is required.

## References

- Linux `linkat(2)`: AT_EMPTY_PATH requires CAP_DAC_READ_SEARCH; proc-fd linking is documented as an alternative: https://man7.org/linux/man-pages/man2/linkat.2.html
- Linux `open(2)`: O_TMPFILE and proc-fd linking example: https://man7.org/linux/man-pages/man2/open.2.html
