# Input and output path boundaries

## Configuration destinations

Configured extension destinations must be one relative directory component. A single trailing slash is accepted for compatibility (for example, Jpeg/). Absolute paths, parent/current-directory components, nested paths, backslashes, control characters, and the reserved journal directory are rejected.

This intentionally keeps output directories directly beneath the process working directory. Nested output paths can be added later only with descriptor-relative traversal and explicit symlink policy.

## Input root

Before starting workers, the CLI:
- Rejects an input root that is itself a symlink.
- Resolves the root to a canonical absolute path and verifies it is a directory.
- Refuses dangerous system roots and the user's home directory/ancestors unless --force is supplied. Safe subdirectories beneath the home directory remain usable.
- Rejects overlap between the canonical input root and every configured output directory, the fallback Others directory, and the journal directory.
- Rejects existing output directories that are symlinks or non-directories.

The --force flag bypasses the dangerous-root guard only. It does not bypass symlink-root checks, output-directory checks, or input/output overlap checks.

## Why

If the input root contains the output directories, the scanner can discover files that the application is concurrently moving and process them again. A root that is an ancestor of the working directory also includes the output folders. Configured traversal paths could otherwise escape the intended output location or collide with the journal.

## Remaining limitation

This is canonical-path validation plus no-follow checks on the final root/output components. A future hardening milestone should pin root and output directories with directory file descriptors and use descriptor-relative operations throughout to close races involving writable parent directories.
