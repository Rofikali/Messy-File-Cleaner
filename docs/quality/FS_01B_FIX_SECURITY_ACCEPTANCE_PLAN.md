# FS-01B-FIX — Positive Source-Retirement Security Acceptance Plan

**Planning status:** test matrix proposed, **NOT RUN** by this docs PR. The [FS-01B known-risk test](FS_01B_RETIREMENT_RACE_CHARACTERIZATION.md) reproduces unsafe unlink and is not an acceptable security pass. Read the [proposed quarantine/fail-closed ADR](../architecture/ADR_FS_01B_SOURCE_RETIREMENT_BOUNDARY.md) and [source audit](../security/FS_01_PRODUCTION_FILESYSTEM_AUDIT_2026-11-16.md).

## Test rules

1. Every test must create its own restrictive disposable `mkdtemp` root and avoid actual user/customer paths. No wildcard cleanup, no environmental `/tmp` sweep, no privilege escalation, no production journal mutation.
2. Verify bytes and file identity before and after each failure with explicit content checks, `st_dev`/`st_ino`, link counts, paths and journal bytes. A test that deletes the last hard link of an unrelated payload **fails the test safety design itself**.
3. **Convert** `fs01b_known_race_characterization` into a **positive security regression** only after a production fix: it must assert that an unauthorized replacement stays intact, the mover does not falsely report successful retirement, and the destination/source state is safely recoverable. Do not keep an assertion that the unsafe unlink remains expected.
4. A same-UID callback holding a protected directory FD trivially bypasses a private-directory boundary and is not a valid *other-UID* attacker simulation. Distinguish attacker capabilities explicitly and test them without implying extra isolation. A same-UID unrestricted writer remains an explicit unresolved threat unless an actual privilege boundary is introduced.

## Required acceptance cases

| Case | Fixture action | Expected safe observable result |
| --- | --- | --- |
| A1 — ordinary success | A private owned source/destination; unchanged original source | Published bytes match original; only authorized original retired; no unexpected file removed; truthful COMPLETED |
| A2 — source entry replaced **before** protected transfer | Controlled swap after source fd open, before quarantine rename | Substitute not unlinked; opened original/destination evidence preserved; report failure/uncertainty rather than success |
| A3 — candidate substituted after identity check | Adapt the FS-01B deterministic test to the approved attacker boundary | No unrelated entry is unlinked. If same-UID attacker is in scope, an effective isolation mechanism must make this true; otherwise fail closed under that model and document limits |
| A4 — source disappears or becomes symlink/special | Rename/remove/replace only fixture source path | Never follow/retire unrelated symlink target; preserve copied complete destination on error |
| A5 — destination collisions | Pre-existing destination and suffixed variants | No overwrite, source remains until safe no-replace publication |
| A6 — permissions/ownership mismatch | Writable shared source, other UID/ACL model where safely testable | If authority cannot be proven: preserve source and published destination; explicit error and accurate journal; no fallback unlink |
| A7 — directory identity substitution | Race/rename parent or private quarantine **name** | Pinned fd continues to designate only original directory; no deletion of unrelated replacement directory; no recursive cleanup on uncertain identity |
| A8 — probe error, ENOSPC, EIO, EINTR | Inject at mkdir/open/rename/fstat/fsync/unlink/close checkpoints where controlled | No unexpected unlink; explicit error code; any published destination preserved; owned temp/orphan state described |
| A9 — crash boundaries | SIGKILL after publish, after quarantine move, before unlink, after unlink, before sync | Every surviving file has known ownership; no automatic deletion of unproven orphan; recovery idempotent and never claims false completeness |
| A10 — hard links and metadata | Original hardlink siblings and sparse/xattr/ACL fixtures where contract requires | Surviving bytes and link count consistent with documented move contract; report unsupported semantics rather than invent fidelity |
| A11 — concurrency/stress | Two movers and a non-cooperating writer limited to disposable fixtures | No unowned unlink, no overwrite, bounded resources and repeatable counters; no arbitrary timing-only proof |
| A12 — platform/filesystem | Selected Ubuntu 24.04 kernel, overlay/ext4 and other **actually provisioned** mounts | Record exact kernel, UID, mount/fs, flags, unsupported syscalls; never claim other filesystems PASS |

## CI and developer commands (future implementation PR)

- CMake C11/C17/C23 Debug and Release; targeted `ctest -R 'mover_security|fs01b|journal_recovery_security|process_crash_recovery'` on a freshly built **test-only** fixture. Test against exact implementation head.
- `make test` and `make validate`, sanitizer checks, Docker non-root smoke, Ubuntu 24.04 validation, plus relevant TSan where a specific shared-state race is being tested. The FCJ4 append TSan job **does not** automatically test this mover boundary.
- Record exact PR SHA, run IDs, number of cases, fixture isolation, input/output hash and unexpected mutations. Preserve complete logs in CI and report bounded failed excerpts.
- Confirm production object uses no `FC_TESTING` callback/symbol; C17 native program only and no FCJ3/FCJ4 production dependency.

## Rollback and operational consequence

- A fix that fails any safety case must be withheld or reverted, **not** papered over with a second pre-unlink stat or retry that widens the race.
- Retain data-preserving behavior on error: leave source or a recoverable private quarantine entry and published destination; record uncertainty clearly. Document manual operator instructions before new orphan locations are allowed.
- Merging a design/test PR is not a production release. Owner must approve the trust boundary, crash persistence and activation; REL-01 and overall FS-01 remain blocked until that evidence exists.

**No tests or production edits occurred as part of this planning document.**
