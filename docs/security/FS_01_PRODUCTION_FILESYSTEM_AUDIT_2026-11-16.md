# FS-01A — Production filesystem and legacy journal safety audit

**Status:** Source-first assessment only. Reviewed `main` at `9dfb2ac30a3a539529e165d2ab168e0e332a0471` on 2026-11-16. No code changes, real-file operations, reproduction, power-loss experiment or local test execution. **FS-01 P0 release gate remains OPEN.**

## Source and evidence scope

- [Production mover](../../src/mover/mover.c), [FCJ2/legacy journal and recovery](../../src/journal/journal.c), [CLI input/output admission](../../src/main.c), [scanner](../../src/scanner/scanner.c), [filesystem helper](../../src/utils/fs_utils.c), [CMake tests](../../CMakeLists.txt).
- [Mover integration tests](../../tests/integration/test_mover.c), [process-crash integration tests](../../tests/integration/test_process_crash_recovery.c), [mover security contract](MOVER-SECURITY.md), [path boundaries](OUTPUT-PATH-BOUNDARIES.md), [threat model](THREAT_MODEL.md), [crash runbook](../quality/PROCESS-CRASH-RECOVERY.md).
- These are code/design observations, not claims of reproduced security exploits or universal filesystem durability. FCJ3/FCJ4 remain test-only, as does Rust tooling.

## Implemented safeguards observed

| Surface | Source-backed control | Residual boundary |
| --- | --- | --- |
| Source read | Mover opens via `openat` with `O_NOFOLLOW`, verifies regular-file type with `fstat` | A scan pathname can change before the mover opens it; later retirement is a separate pathname operation |
| Publication | Exclusive temporary file; copy from opened source fd; source metadata comparison; temp `fsync`; Linux `renameat2(RENAME_NOREPLACE)` | Defends against ordinary overwrite but is not arbitrary-filesystem certification |
| Retirement | No-replace rename to `.fc-retire-*`, `fstatat(..., AT_SYMLINK_NOFOLLOW)` dev/inode/type check, then `unlinkat` and source-directory sync | Check and unlink are separate calls on a name in a potentially writable source directory |
| Journal | Bounded legacy/FCJ2 parser, advisory lock, sync states; recovery preserves ambiguous candidates rather than deleting them | `journal_recover` evaluates pathname existence/type with `lstat`, not authenticated original inode or payload |
| CLI admission | Canonical root validation, dangerous-root guard, input/output overlap refusal, destination-component validation | Concurrent writable parent substitution and filesystem-specific effects still need tests |
| Process crash | SIGKILL children and repeat recovery tests on disposable directories | SIGKILL is not sudden machine power loss or proof on other mount types |

## Prioritized risks and bounded next steps

| ID | Priority | Review observation | Proposed proof and next gate |
| --- | --- | --- | --- |
| **FS-01B** | **P0** | In `retire_source` (`src/mover/mover.c` approx. lines 428–487), quarantine name lives in the source directory; `fstatat` identity check precedes pathname `unlinkat`. The existing threat model section 6.8 explicitly warns the earlier check is not sufficient to guarantee race-free unlink. No exploit was reproduced in this audit. | First test-only, disposable, controlled source-entry substitution matrix with inode/payload survival and error assertions. Review whether a private, descriptor-pinned quarantine with appropriate permissions is required. Any production fix requires separate approval and a bounded PR. |
| **FS-01C** | **P0 for release** | `journal_recover` (`src/journal/journal.c` approx. lines 451–688) groups by source/dest path, uses `lstat` and appends `RECOVERED` without proving object identity. It does **not** perform file deletion. | Add fixture-only path-replacement/reporting tests; distinguish presence of a regular file from authenticated journal ownership. No legacy destructive cleanup. |
| **FS-01D** | **P1** | The [crash test runbook](../quality/PROCESS-CRASH-RECOVERY.md) records orphan `.fc-tmp-*` entries after a STARTED crash; legacy journal does not record temp identity. Tests clean only their disposable fixtures. | Preserve orphans by default; design documented manual handling rather than name-prefix deletion. |
| **FS-01E** | **P1** | Canonical admission checks occur before later descriptor/path operations. No-follow traversal helps but does not eliminate hostile concurrent directory replacement. | Targeted writable-parent/symlink/race fixture tests and source-preservation assertions. |
| **FS-01F** | **P0 release gate** | Existing `fsync` and SIGKILL tests do not establish power-loss durability or ext4/XFS/btrfs/overlay/NFS behavior. | Explicit supported-filesystem and kernel/mount matrix, fault evidence, backup and operator release procedure. Do not infer PASS on untested hosts. |
| **FS-01G** | **P2** | Byte copy and basic permissions are implemented; complete xattr/ACL/owner/hardlink/sparse/timestamp preservation is not claimed by current source review. | Agree on intended metadata contract before treating differences as bugs; measure and test within disposable fixtures. |

## Delivery sequence

1. **FS-01A (this document):** read-only source review and priority register. Its merge **does not complete** FS-01 and does not authorize source deletion.
2. **FS-01B next:** after FS-01A review/merge, design a deterministic, private-`mkdtemp`, test-only regression for retirement identity/race evidence. Start with existing mover test baseline and fail closed. If an exact race cannot be safely or reliably reproduced, document the limitation rather than fabricate results.
3. **FS-01C:** read-only/append-only legacy recovery reporting and path-identity tests.
4. **FS-01D/E/F:** orphan policy, writable-parent model, supported filesystem, crash/power and release requirements.

## Verification contract

- Existing `CMakeLists.txt` declares `mover_security`, `journal_recovery_security`, `process_crash_recovery`, `config_destination_security`, `scanner_security` and other tests; their existence is not new run evidence.
- Future implementation PRs: targeted sanitized Debug tests, then `make test`, C11/C17/C23 `make validate`, appropriate Release/non-root Docker and exact PR-head CI, plus fail/errno and preservation of bytes, identity and unrelated entries. Never test with personal/customer files.
- Previous ROADMAP-02 PR #87 head `df48832870250faba9d55d8575b5431d7705a6c5` had all four triggered PR CI workflows pass and merged to `9dfb2ac30a3a539529e165d2ab168e0e332a0471`. This is **historical** evidence and not a test result for FS-01A.
- No production Rust FFI, public `.a`/`.so` SDK, FCJ4 production recovery, automatic temporary-file sweeper, paid infrastructure or new native file mutations are approved by this audit.

**Decision:** prioritize source preservation. The next job is chosen from the live `next.md` board; plain **go ahead** authorizes the next bounded eligible *review/test work*, not PR merging, live mutation, deployment or removing security gates.
