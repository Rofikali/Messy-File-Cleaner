# FS-01B-GUARD — Fail-closed source-parent permissions (interim mitigation)

**Status:** narrow production C17 mitigation **in review**, not the final FS-01B-FIX. Source branch based on `main` `54d70b95207191f2a7f22715a43a2fb39b18a132` (PR #90 merged). The [full private-quarantine ADR](../architecture/ADR_FS_01B_SOURCE_RETIREMENT_BOUNDARY.md) remains **PROPOSED / NOT APPROVED**. No user-file deployments are authorized.

## Why this is necessary

[FS-01B PR #89](https://github.com/Rofikali/Messy-File-Cleaner/pull/89) reproduced a check/use race in `retire_source`: an attacker who can change the source directory entry between `fstatat` and `unlinkat` can substitute a different entry. The existing [FS-01B characterization](../quality/FS_01B_RETIREMENT_RACE_CHARACTERIZATION.md) intentionally **still passes when that known-unsafe action occurs**. It is not a release gate.

## Narrow policy implemented in this proposal

At two points, `src/mover/mover.c` now calls `source_parent_retirement_guard` on the **open source parent directory descriptor**: (1) before opening/creating the destination; (2) immediately before source retirement. The check uses `fstat`, requires a directory owned by the **effective UID**, and rejects directories writable by group or other (`S_IWGRP | S_IWOTH`) with `EPERM`.

- **Shared-writable input**: fail closed **before any copy or destination creation**; original file remains at its initial name with original bytes, no new destination or journal state. This intentionally changes behavior for group/other-writable trees; it may reject previously supported configurations.
- **Source directory becomes unsuitable after initial admission**: the second check refuses retirement, returns an error, and retains any already published complete destination rather than rolling it back. Real operator reporting must make the duplicate/uncertainty explicit.
- **Private owned source**: existing copy, publish, journal and retirement path remains unchanged. The known check/use race remains reachable by a capable same-UID actor; this mitigation **does not** resolve that threat.
- Pathname replacement via a privileged actor or a capability outside ordinary POSIX DAC remains possible. ACL, nonlocal filesystems, mount/namespace/UID mapping, pre-opened descriptors, source content mutation, cross-filesystem durability, process crash and power loss are **not certified** by this change.

## Explicit safety decision

**This is containment, not complete correction.** To eliminate the identified same-UID/privileged check/use gap, there must be a stronger trust-boundary or no-unlink fail-closed policy. A new private quarantine under the source tree may create a crash orphan that existing FCJ2 journal cannot identify and the recursive scanner could encounter. The ADR therefore remains proposed until operator recovery, scanner exclusion and privilege model are accepted. Never silently enable it here.

## Verification requirements

- `tests/integration/test_fs01b_source_permissions.c`: independent CTest with **no `FC_TESTING` definition**; uses private `mkdtemp` fixture. It verifies group and world writable source directories fail with `EPERM`, byte/inode preservation, no destination, then after restoring 0700 the ordinary move succeeds. Cleans exact created fixture paths only.
- Existing `fs01b_known_race_characterization` stays as known-risk *evidence*, **not** positive security proof. Both tests must run in each supported C11/C17/C23 lane.
- Review run `CMake` C11/C17/C23 Debug/Release, focused CTest, CI Docker/Makefile/Ubuntu as triggered; record exact tested commit and failures. Confirm shipped binary has no test hook. Do not interpret passing CI as removal of residual P0 risk.

## Decision, rollback and next work

- If the stricter source admission unexpectedly rejects legitimate users, document and retain safe `EPERM` refusal rather than reverting to unguarded destructive retirement; the existing default Docker dry-run remains available.
- If this branch's tests fail, fix within this PR, without weakening data-preservation assertions. Rollback consists of reverting the PR before distribution; do not attempt automatic cleanup of existing files.
- **FS-01B-FIX remains OPEN / BLOCKED on an accepted end-to-end authority and crash-safe recovery design.** After review, prioritize source-retirement staging isolation or explicitly copy-only no-unlink policy, with positive preservation and crash tests, as a separate PR.
- No production FCJ3/FCJ4, Rust FFI, `.a`/`.so` SDK, broad API, deployment, pricing or privileged operation is authorized.

**Do not run on valuable source directories without an independent verified backup.**
