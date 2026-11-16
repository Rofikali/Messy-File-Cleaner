# FS-01B — Source-retirement check/use race characterization

**Status:** **MERGED test-only known-risk reproduction** via [PR #89](https://github.com/Rofikali/Messy-File-Cleaner/pull/89), merge `fc09ae2df7d29d8c82419819e820455f58db7f4f`. All five triggered PR-head workflows passed at `4d030275d30efe0a1e8b6ab7ad65647f6b72922d` (CMake, Docker, Makefile, Ubuntu 24.04, FCJ4 TSan); C17 Debug ran 29/29 tests. **PASS reproduced the unsafe unlink; no security fix or release certificate.** Applies to the [FS-01A source audit](../security/FS_01_PRODUCTION_FILESYSTEM_AUDIT_2026-11-16.md). Next: [proposed P0 fix ADR](../architecture/ADR_FS_01B_SOURCE_RETIREMENT_BOUNDARY.md) and [positive test plan](FS_01B_FIX_SECURITY_ACCEPTANCE_PLAN.md); neither is yet accepted or implemented.

## What is observed and why it matters

`src/mover/mover.c:retire_source()` uses `renameat2(RENAME_NOREPLACE)` to place a source entry under a temporary quarantine **name in the same source directory**, calls `fstatat(..., AT_SYMLINK_NOFOLLOW)` to compare the quarantined inode to the opened source, then calls `unlinkat` on that name. In a source directory writable by a concurrent actor, the entry can be substituted between the identity check and unlink. That check does **not** pin the checked directory entry for the subsequent unlink operation.

The existing [threat model](../security/THREAT_MODEL.md) §6.8 explicitly warns that an earlier `fstatat` cannot prove a later pathname unlink is race-free. This test narrows the abstract risk to a deterministic observed **check/use gap** in the actual mover implementation.

## Deterministic test boundary

- `FC_TESTING` builds only expose a single-threaded `fc_test_mover_set_retire_probe` callback. This callback is called **after the positive quarantine `fstatat` check and immediately before `unlinkat`**. The macro is absent from all production CMake targets. This hook only schedules a test fixture; it does not enforce a real production safety rule.
- `tests/integration/test_fs01b_retirement_race.c` creates a private `mkdtemp` root and 0700 `source/` and `output/` directories. It writes an original payload and an unrelated replacement payload. It creates a **second hard link to the replacement** before any attempted substitution, ensuring there is always a surviving copy of its bytes.
- The probe renames the checked original quarantine entry to `retained-original.bin` (preserving those bytes) and renames the unrelated replacement entry into the vacated quarantine name **before the mover resumes**. No sleeps, OS scheduler luck, external attacker, customer files or real directories are involved.
- **Expected current insecure behavior (characterization only):** mover returns success, published destination and retained original both contain original payload, replacement's backup hard link retains unrelated payload, but the moved replacement entry is unlinked and the backup's link count decreases from two to one. The journal would otherwise be able to regard the move as complete despite retirement of an unowned directory entry.
- The test cleans up only its exact created fixture entries. It never sweeps files by name prefix. All data still has a surviving hard-link or copied version before cleanup.

## Interpretation — read before claiming PASS

**This test passing means the risk was reproduced, not that the file mover is safe.** It intentionally asserts the **current known-unsafe result** to make the source-level concern measurable without deleting the only copy of the replacement payload. A future fix must **change this test to assert preservation of the unrelated replacement and fail-closed/error reporting**, then retain it permanently as a positive regression. Do not normalize the unsafe behavior as a product requirement.

It does **not** prove an arbitrary user directory can be exploited in every deployment. A concurrent writer needs suitable permissions and timing; the injected schedule point removes timing uncertainty only for the test.

## Acceptance and release consequences

- Build and run the focused test `ctest --test-dir build/debug -R fs01b_known_race_characterization --output-on-failure` after a test CMake build; run C11/C17/C23 `make validate` and applicable PR CI. Keep exact SHA and job outcomes. This is a test-only C code and CMake change; no release binary or journal format is edited.
- Verify in code review that the probe is under `#ifdef FC_TESTING` and absent from the production binary; test-only CMake links `mover.c` with `FC_TESTING`.
- **P0 release gate remains BLOCKED.** The next task (**FS-01B-FIX**, separately approved) is a threat-modelled source-retirement design eliminating attacker-writable check/use name substitution, with negative race/identity/permission cases and failure/crash durability review. Avoid merely adding a second `fstatat` before `unlinkat`: it repeats the same race.
- No production C changes, automatic cleanup, FCJ3/FCJ4 deployment, Rust FFI or public SDK is approved by this characterization.
