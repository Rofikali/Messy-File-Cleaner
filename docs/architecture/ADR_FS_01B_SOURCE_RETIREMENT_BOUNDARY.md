# ADR FS-01B-FIX — Source-Retirement Authority and Private Quarantine

**Status: PROPOSED / NOT APPROVED FOR PRODUCTION.** This P0 design **document** was merged in [PR #90](https://github.com/Rofikali/Messy-File-Cleaner/pull/90) at `54d70b95207191f2a7f22715a43a2fb39b18a132` (2026-11-16), but the PR did **not** accept a final source-retirement trust boundary, authorize code activation, or close release gates. Based on earlier [PR #89](https://github.com/Rofikali/Messy-File-Cleaner/pull/89) known-risk evidence. A narrow, separate [FS-01B-GUARD](../security/FS_01B_SHARED_SOURCE_GUARD.md) containment proposal refuses other-writable source directories without implementing the private-quarantine design. Reconcile live [next.md](../../next.md) before acting.

## 1. Evidence and security objective

- [The source audit](../security/FS_01_PRODUCTION_FILESYSTEM_AUDIT_2026-11-16.md) identified a check/use window: `retire_source` renames a source file to a `.fc-retire-*` name in the original source directory; `fstatat` confirms its inode and type; a later `unlinkat` removes whatever entry occupies that name.
- [PR #89](https://github.com/Rofikali/Messy-File-Cleaner/pull/89) merged a **known-unsafe characterization** under `FC_TESTING`. On its tested head `4d030275d30efe0a1e8b6ab7ad65647f6b72922d`, the deterministic test swapped that entry after the check. The mover returned success and unlinked the substituted entry. The fixture kept copies through retained links; **PASS = risk reproduced, not fixed**.
- The production mover already provides `O_NOFOLLOW` source opens, exclusive temp creation, source fd metadata comparison, `RENAME_NOREPLACE` publication and file/directory `fsync`. Keep all these controls. Existing FCJ2 recovery is conservative and pathname-based. FCJ3/FCJ4 remain **test-only**.

**Required invariant:** the mover must never unlink a directory entry it cannot establish as the source object it opened and copied. If authority, entry identity or durability is uncertain, **preserve file bytes, return an explicit failure, and retain the published complete destination**. A duplicate is safer than removing somebody else's file.

## 2. Root cause and threat boundary

On Linux, `fstatat` followed by `unlinkat(name)` is a check/use pair, not an atomic conditional unlink of a particular inode. A second check does not close the race. An open file descriptor pins the inode for reading but a conventional POSIX `unlinkat` still acts on a *name*.

A directory entry in a source directory writable by a different actor is not a trustworthy retirement authority. `RENAME_NOREPLACE` makes the staging rename no-overwrite; it does **not** prevent changes to the staged name after it succeeds.

**Security nuance:** mode `0700` and an owned directory can restrict *other-UID* writers, but cannot isolate against a fully capable process with the **same UID**. Same-UID access, parent-directory rename rights, mount namespaces, idmapped mounts, ACLs, sticky bits, and privileged root attackers must be explicitly included or excluded. Do not promise a universal race-free unlink or call a random/private-looking name an authorization boundary.

## 3. Alternative analysis — do not code until owner decision

| Option | Safety upside | Tradeoff / non-goal | Verdict |
| --- | --- | --- | --- |
| A — repeat `fstatat` before unlink | Minimal code | Same check/use race remains | **REJECT** |
| B — unlink the existing `.fc-retire-*` name in source parent | Preserves current crash semantics | Confirmed post-check substitution vulnerability | **REJECT** as a general source-retirement boundary |
| C — never unlink: publish a copy and preserve source on untrusted paths | Strong immediate data-preservation posture; no unowned unlink | Changes move semantics; duplicates, disk and journal/result handling | **APPROVE IN PRINCIPLE as fail-closed fallback**, requires clear explicit behavior and tests |
| D — no-replace move to a fresh private, mode-0700 quarantine directory, pinned by `openat` fd; verify then unlink **relative to pinned fd** | Constrains other-UID substitution if exclusive access to quarantine object is actually proved | Same-UID attacker, parent-name substitution and crash-orphan lifecycle remain; cross-filesystem and filesystem feature support must be checked | **PREFERRED design candidate**, conditional on trust-boundary and crash gates below |
| E — public reusable C API or FCJ4 production recovery as part of fix | None needed for this bug | Expands risky scope and deploy surfaces | **DEFER** |

## 4. Proposed bounded implementation contract (not yet accepted)

### 4.1 Admission and fail-closed mode

1. Use an already validated **source directory fd**. Before source retirement, decide whether the process has an exclusive-enough source/quarantine authority for the declared adversary model. Inspect `fstat` owner/mode, effective UID, source parent namespace and relevant ACL/mount constraints; document what cannot be proven.
2. If no reliable private boundary exists, **do not retire or unlink**. Preserve the source and published destination, report a distinct failure/unsupported authority outcome (e.g. `EPERM`/`EBUSY` with documented mapping), and record truthful journal error state. Do not silently call a copy a completed move.
3. Do not escalate privileges, chmod arbitrary user directories, remove symlinks, or downgrade to the vulnerable legacy name-unlink path for convenience. A `--force` argument is **not** permission to bypass the retirement identity invariant.

### 4.2 Candidate pinned quarantine protocol

4. Create a **new** unpredictable-named directory under the same source mount via descriptor-relative `mkdirat(mode 0700)` with strict collision handling. Open it via `openat(O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC)`. Verify `fstat`: directory, expected owner, restricted mode and identity of the newly created object. Prefer a valid fd over a re-resolved path. Do not trust name prefix or the mere presence of a directory.
5. Determine whether group/other or same-UID actors can rename/access that directory or its contents. Source parent controlled by another UID is not inherently fatal if a pinned private directory is protected from those actors, but the parent may rename the *directory itself*. A process that can access the private directory (notably same UID) can still defeat ordinary pathname unlink. Fail closed when the stated guarantee cannot be established.
6. Move source name into the pinned private directory **with no replace**, using a fixed bounded internal entry name that could not previously exist. Verify `fstatat(..., AT_SYMLINK_NOFOLLOW)` matches the open source fd identity and regular-file type; also consider concurrent source content mutation, extra hard links, and source path disappearance before/after transfer. On mismatch, do **not** delete. Try no-replace restoration only when safe; otherwise leave a named, operator-recoverable orphan and return failure.
7. Only if all authority and identity checks succeed, permit retirement of that internal entry **relative to the trusted private fd**. The relevant security boundary is exclusive control of that directory, not an extra `fstatat`. Sync the private directory and source parent in the reviewed order; preserve published destination on any error.
8. Empty private-directory removal via `unlinkat(parent_fd, quarantine_name, AT_REMOVEDIR)` is *also* name-based. Do not remove it if unrelated substitution is possible: either prove parent name ownership at the action boundary under the stated threat model, or retain an empty orphan for separately approved operator handling. Never sweep `.fc-*` directories by prefix.

### 4.3 Crash, journal and resource contract

- Preserve the existing ordering: temp file sync, no-replace destination publish, destination-directory sync, durable PUBLISHED transition, then any source retirement. Avoid a new call to unjournaled destructive recovery.
- A crash after moving into a private quarantine but before retirement may leave the original source name absent and the verified content inside a quarantine directory. The legacy journal does not record the private quarantine identity. **This is a new recovery/visibility consequence** and must be explicitly addressed before selecting option D for production.
- Consider a bounded non-destructive restoration path, a durable ownership record or adopting **copy-only fail-closed mode** until durable crash identity exists. Do not graft test-only FCJ4 records into production without a separate approved compatibility migration.
- Bound directory-name generation, mkdir collisions, file descriptors, loops and journaling. Document worst-case extra disk metadata operations and measured latency; no unsupported nanosecond assertion.

## 5. Implementation gates and decision questions

**Human design decision required:** choose (1) acceptable adversary model (different UID, same UID, writable source parent, root), (2) supported filesystem(s) and directory ownership modes, (3) what to do if source retirement authority is not provable (recommended: preserve both copies and report failure), and (4) how to handle crash-orphaned source files before any production activation.

Then implement in **a separate small code PR** only after approval, including positive preservation tests. Any attempt to bypass a blocked release gate is out of scope. Do not merge this ADR as though it were an accepted runtime design.

## 6. Test and acceptance references

See [FS-01B-FIX positive security test and rollback plan](../quality/FS_01B_FIX_SECURITY_ACCEPTANCE_PLAN.md) for deterministic negative fixtures, tested failure states, exact-head CI, crash/rollback checks, isolation and required source-code review.

## 7. Rollback and non-goals

- This ADR only proposes alternatives. Revert it and the live roadmap change if rejected; production binary is unchanged.
- No live file operations, root privileges, user-file deletion, production FCJ3/FCJ4, Rust runtime, C ABI/SDK, GUI/SaaS, deployment, public release or paid resources authorized.
- Merging a documentation design **does not mean the owner accepted option D or approved a production fix**. Keep FS-01 P0 and REL-01 blocked until actual positive evidence exists.
