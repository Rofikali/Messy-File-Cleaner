# FCJ3 Read-Only Recovery Reporting Design

## Status
Proposed design. Review and approval are required before implementation. This is a diagnostic-only milestone; FCJ3 remains isolated from production FCJ2/legacy recovery and the production mover.

## Objective
Provide an operator-invoked report for a complete FCJ3 journal. For each transaction, the report should show the last validated journal state, read-only filesystem observations, conservative classifier outcome, reason code, and relevant errno diagnostics.

The report must not execute recovery or change any filesystem/journal state.

## Scope
1. Open a caller-selected FCJ3 journal read-only and without following symlinks.
2. Read the entire journal as a bounded stream and replay every complete line through `fcj3_replay_add_line`.
3. Reject the entire report if any line is malformed, truncated, out of bounds, or violates transaction ordering. Never classify a valid prefix after a bad suffix.
4. Resolve source and destination parent directories using a descriptor-relative, no-symlink component walk.
5. Pass already-open parent directory descriptors to `fcj3_recovery_collect_at`.
6. Pass the collector's observations to `fcj3_recovery_classify`.
7. Emit a deterministic, machine-readable JSON report to stdout. Diagnostics go to stderr. Do not write report files.
8. Keep the executable separate from the production `file-cleaner` target and do not install it.

## Trust boundaries and path resolution
- The journal pathname supplied by the operator must be absolute. Walk from an already-open `/` descriptor using `openat` with `O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW` for each parent component. Open the final journal component with `O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK`, then verify with `fstat` that it is a regular file.
- Source and destination paths are journal data and remain untrusted. Require absolute paths and reject empty, `.`, `..`, nested-invalid, control-character, and overlong components. Resolve each parent component one at a time from `/` using `openat` with `O_DIRECTORY | O_NOFOLLOW`. Never use `stat` or `open` on concatenated journal paths.
- Keep the final parent descriptors open while collecting evidence. Directory descriptors pin the resolved objects across later renames; the collector verifies the destination directory's recorded device/inode before child lookups.
- Symlinked path components are not traversed. A path that requires a symlink is reported as unknown/manual review, not followed.
- Source parent identity is not stored in FCJ3. Even a successful no-symlink walk cannot prove that the source parent is the original parent. Source evidence remains advisory and can never authorize retirement.
- This walk does not impose a mount-boundary policy. Destination device/inode matching is the recorded authority; source is advisory. If product requirements later prohibit crossing mounts, that must be a separate explicit policy.

## Journal snapshot and replay integrity
- Open the journal as a regular file with no final-component symlink following.
- Enforce a documented maximum journal byte size and the parser's maximum transaction count/line size.
- Stream into a fixed-size line buffer; require every record to end with LF. An unterminated tail is an invalid journal, not a partial transaction.
- Retry interrupted reads. Treat zero progress, read errors, oversized lines, and parser errors as fatal to the whole report.
- Capture file identity, size, and high-resolution modification/change timestamps before and after reading. If any differ, reject the report as an unstable journal.
- This metadata check is a race detector, not an atomic snapshot or cryptographic authenticity guarantee. A same-privilege writer able to rewrite and restore metadata may evade it. The report must state this limitation; no automatic action is permitted regardless.
- Replay into a bounded `Fcj3Replay`. Do not emit partial JSON before full replay and all per-transaction observations complete.

## Evidence and classification
- Use the merged read-only evidence collector and existing pure classifier without modifying their semantics.
- Preserve ENOENT separately from EACCES, EIO, and other errors.
- A directory identity mismatch/unknown prevents child lookups (already enforced by the collector).
- Symlinks are observed without following them.
- The source is only absent/present/unknown; record source mode for diagnostics but never infer source inode identity.
- Every transaction gets exactly one existing classification and reason code. The report layer adds human-readable labels but does not add new recovery decisions.
- If parent resolution fails, synthesize UNKNOWN observations for that transaction and classify to MANUAL_REVIEW. Preserve the resolver errno in the report.
- If an invalid transaction path cannot be safely resolved, do not attempt a fallback path lookup.

## Output contract
JSON schema version 1:
- top-level: `schema_version`, `journal`, `transaction_count`, `transactions`
- journal: safely escaped path, device, inode, size, and stability-check result
- transaction: operation_id, sequence, state, source, destination, staging_component, directory/stage/destination/source observations, diagnostic errno values, classification, reason
- strings must be JSON-escaped; never interpolate raw untrusted path text into JSON.
- deterministic transaction order is replay insertion order (first-seen operation ID order).
- stdout contains JSON only. Errors and usage messages go to stderr with nonzero exit status.
- The report must label itself read-only and explicitly say that no filesystem or journal changes were made.

## Non-goals
- No FCJ3 production journal creation or append.
- No FCJ2/legacy parser or recovery changes.
- No recovery executor, RECOVERED transition, retry, cleanup, quarantine, unlink, rename, link, chmod, truncate, or overwrite.
- No source retirement or publication.
- No daemon, watcher, web server, database, event broker, or new runtime dependency.
- No path-resolution fallback through symlinks.
- No attempt to claim an atomic journal snapshot or power-loss durability.

## Security and correctness risks
| Risk | Control |
|---|---|
| Malformed suffix after valid records | Parse entire file before reporting; fail whole report |
| Symlink/path traversal | Component-by-component openat with O_NOFOLLOW; reject dot components |
| Journal final symlink/FIFO/device | O_NOFOLLOW + fstat regular-file check |
| Journal mutation during read | Before/after dev/ino/size/mtime/ctime check; disclose non-atomic limitation |
| Huge journal or line | Explicit byte, transaction, and line bounds |
| Source replacement at same path | Source evidence advisory only; no source identity claim |
| Destination directory replacement | Compare pinned descriptor's dev/ino to journal |
| JSON/log injection | JSON escaping; stdout JSON only; errors to stderr |
| Report mistaken for action | Diagnostic-only binary, no mutation APIs, explicit read-only label |

## Acceptance criteria
- Pure C only; C11, C17, and C23 builds.
- Report executable is separate, not installed, and not linked into production `file-cleaner`.
- Journal open is read-only, regular-file-only, and no-follow.
- Entire journal replay succeeds before any transaction is reported.
- Truncated/malformed suffix, duplicate/illegal transition, oversized line/file, read error, and unstable journal fail closed.
- Parent path resolution is descriptor-relative and rejects symlinks, `.`, `..`, controls, and invalid components.
- Collector and classifier are reused; no duplicated classification policy.
- Tests cover valid multi-transaction report, all classifier categories, resolver errors, symlinked path components, source symlink advisory semantics, destination identity mismatch, malformed journal suffix, and JSON escaping.
- Tests assert no fixture/journal bytes or directory entries change during reporting.
- `make test`, `make validate`, Docker/non-root workflow, and Ubuntu 24.04 workflow pass.
- Documentation clearly states source-identity and non-atomic journal snapshot limitations.

## Rollback
The feature is a separate test/diagnostic executable and isolated source set. Rollback consists of removing its target, source/header, tests, and this design document. The production executable, journal formats, and existing recovery behavior remain unchanged.
