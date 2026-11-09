# FCJ3 Read-Only Filesystem Evidence Collector

## Status
Implementation proposal for the read-only evidence phase. FCJ3 remains test-only. This module is not linked into the production executable and cannot perform recovery actions.

## Context
PR #53/#54 established a pure-C classifier that consumes explicit filesystem observations. PR #55 hardened staged-content integrity checks. The next layer must gather those observations without resolving untrusted path strings itself or changing filesystem state.

The FCJ3 schema records destination-directory and staging inode identity, but does not record source inode or source-parent identity.

## Decision
Implement a narrow descriptor-relative adapter:

- The caller supplies already-open source-parent and destination-parent directory descriptors.
- The adapter validates the complete recorded paths lexically and extracts only their final components.
- The adapter verifies that the destination descriptor is a directory with the exact recorded st_dev/st_ino before looking up stage or destination entries.
- Entry lookup uses fstatat with AT_SYMLINK_NOFOLLOW, retries EINTR, and preserves ENOENT separately from all other errors.
- Stage and destination observations distinguish absent, matching regular inode, mismatching regular inode, symlink, special file, and unknown error.
- Source lookup is no-follow and reports only absent/present/unknown. It records the observed mode for diagnostics but never claims the entry is the original source.
- The module performs no openat, read, write, rename, link, unlink, chmod, truncate, fsync, journal append, or cleanup.

## Why caller-supplied descriptors
Resolving arbitrary absolute journal paths safely requires a separate path-resolution contract for roots, intermediate components, mount boundaries, symlinks, and directory replacement. Mixing that concern into evidence classification would create an unreviewed trust boundary. This first cut requires a separately reviewed resolver to supply the descriptors.

Destination parent identity is checked against the journal. Source parent identity cannot be checked with the current schema; source evidence is therefore advisory and must never authorize source retirement.

## Rejected alternatives
- stat on concatenated journal paths: follows symlinks and re-resolves attacker-controlled paths.
- open of journal paths inside the collector: silently expands the module into a path resolver.
- trusting the destination basename alone: a name is not inode ownership.
- treating every lookup error as absence: hides EACCES, EIO, resource exhaustion, and unsupported behavior.
- adding the collector to production sources: FCJ3 recovery is not ready for production integration.

## Security and failure semantics
- Invalid descriptors, malformed paths, traversal components, or invalid stage components return -1/EINVAL.
- Filesystem lookup failures are returned as explicit UNKNOWN_ERROR observations with diagnostic errno fields.
- A destination directory mismatch or unknown result prevents all child lookups.
- Symlinks are observed, never followed.
- No observed source inode identity is inferred.
- Any uncertainty is preserved for the classifier to map to MANUAL_REVIEW.
- No filesystem or journal mutation is possible through this API.

## Acceptance criteria
- Pure C; C11, C17, and C23 compile.
- Collector isolated from production FCJ2/legacy recovery and mover.
- Descriptor-relative fstat/fstatat only.
- Tests cover exact inode match, mismatch, missing entries, symlink, FIFO/special file, destination-directory mismatch, and unsafe path rejection.
- Tests prove the collector does not follow symlinks and does not mutate fixtures.
- make test and make validate pass in CI and the repository owner verifies the result.
- PR states that source parent identity and source inode identity remain unresolved schema limitations.
