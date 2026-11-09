# FCJ4 Writer State Reconciliation and Cross-Record Validation Design

## Status

Implementation specification for the follow-up to merged PR #66. The implementation branch is being prepared against this design. The design remains the acceptance contract until code review and C11/C17/C23, sanitizer, Release, Makefile, Docker, and non-root validation are complete. It does not authorize journal I/O, file creation, append, fsync, production integration, or filesystem recovery.

## 1. Objective

Make the in-memory FCJ4 writer state a safe gate for future append work. Every record accepted by writer preparation must be consistent with the immutable BEGIN/SEALED evidence and existing ACTION_DEF relationships, not merely well-formed as an individual line. A writer state reconstructed from a valid complete parser replay must preserve enough information to validate the next record exactly as a fresh writer would.

The existing parser remains the authority for the complete wire history. Writer validation must be at least as strict for the next record so that a future append adapter cannot durably add a record that makes the journal unparsable.

## 2. Review findings from PR #66

PR #66 passed its focused test, C11/C17/C23 sanitizer matrix, Makefile workflow, Ubuntu 24.04 validation, and Docker smoke workflows, and is merged.

Code review identified these gaps before append work:

1. The writer state retains operation IDs, a SEALED flag, move state/sequence, and action IDs/state/sequence, but not the immutable BEGIN/SEALED evidence needed to validate future records.
2. The serializer checks ACTION_DEF field shape and some kind/role relationships, but does not check parent identity, entry identity, metadata, digest, or component binding against BEGIN/SEALED.
3. SEALED serialization does not check that stage device equals the destination-parent device or that stage size equals source size. The parser rejects either mismatch.
4. The parser exposes action sequence in Fcj4ActionView but does not expose operation move sequence in Fcj4OperationView. A replay-seeded writer therefore cannot resume exact MOVE sequence validation from the public views.
5. The writer did not track total record count, so its in-memory state could accept more records than the parser's FCJ4_MAX_RECORDS limit. The current implementation branch now rejects prepare when the committed-record revision has reached FCJ4_MAX_RECORDS; because revision advances only on successful in-memory commit, prepare/abort do not consume the limit. Replay-import accounting remains outstanding.
6. The current test proves one valid history round-trips. It does not prove rejection of records that are individually well-formed but inconsistent with the operation evidence or action relationships.

These are not evidence of production data damage: the module is isolated and not linked into the production executable. They are blockers for a future append adapter.

## 3. Required state model

The writer state must retain bounded, owned copies of the evidence needed for validation. Do not retain borrowed pointers into a replay object.

For each operation, preserve:

- operation ID;
- source basename and source parent device/inode;
- source device/inode, uid, gid, mode, size, and source diagnostic path as needed by the API;
- destination parent device/inode;
- quarantine parent device/inode;
- SEALED presence, stage component, stage device/inode, uid/mode/size, and SHA-256 digest;
- move state and move sequence;
- each action's ID, kind, target role, source component, optional target component, expected parent/entry identity and metadata, digest, action state, and action sequence.

All arrays remain bounded by the parser's existing limits: 4096 operations, 16 actions per operation, 100000 records, and 32 KiB per record. Check allocation-size arithmetic even when limits are compile-time constants.

## 4. Replay-seeded state reconstruction

Add an explicit API that imports a successfully parsed `Fcj4Replay` into a fresh, empty writer state.

- The replay must already have been produced by `fcj4_replay_parse`; do not accept raw untrusted serialized bytes as a shortcut.
- Import must be all-or-nothing. On failure, the destination state remains empty and usable or is destroyed with a documented status; it must never contain a partial reconstruction.
- Deep-copy every string and field needed after the replay is destroyed.
- Reconstruct each operation's move state and exact move sequence.
- Reconstruct each action's definition, state, and exact sequence.
- Initialize the writer's total record count from `replay.record_count`.
- Preserve interleaved operation histories; do not assume all records for one operation are contiguous.
- A state created from an empty journal and a state imported from an equivalent parsed history must make identical accept/reject decisions for the same proposed next record.
- Extend `Fcj4OperationView` with a read-only `move_sequence` field, or provide an equivalent parser accessor. Keep the parser API additive and document view lifetime.

## 5. Cross-record validation before prepare

The writer must reject the proposed record before it is returned as prepared if any parser-level invariant would fail after appending that record.

### 5.1 SEALED

In addition to current field validation:

- operation must have exactly one BEGIN and no prior SEALED;
- stage device must equal BEGIN destination-parent device;
- stage size must equal BEGIN source size;
- stage component must obey the existing parser's reserved prefix and component rules;
- stage inode must be nonzero and SHA-256 must be canonical lowercase hex.

### 5.2 ACTION_DEF

All current parser relationships must be enforced before prepare:

- operation must have BEGIN and SEALED;
- action ID must be unique within the operation and action count must remain within 16;
- kind/target-role mapping must exactly match the parser;
- QUARANTINE_STAGE: expected parent equals destination parent; source component equals the SEALED stage component; expected entry device/inode, uid, mode, size, and digest equal SEALED evidence;
- QUARANTINE_SOURCE: expected parent equals source parent; source component equals the basename of the BEGIN source path; expected entry device/inode, uid, mode, size, and digest equal BEGIN source evidence (digest must equal the SEALED digest as required by the parser);
- RETIRE_QUARANTINED_SOURCE: expected parent equals quarantine parent; role is NONE; source component equals the target component of a previously defined QUARANTINE_SOURCE action; expected entry metadata and digest equal BEGIN source evidence;
- REMOVE_QUARANTINED_STAGE: expected parent equals quarantine parent; role is NONE; source component equals the target component of a previously defined QUARANTINE_STAGE action; expected entry metadata and digest equal SEALED evidence;
- target component is present only for QUARANTINE role; NONE uses the exact schema sentinel pair (length `0`, hex field `-`).

The writer may adopt a stricter definition order for quarantine follow-up actions: the corresponding QUARANTINE_SOURCE or QUARANTINE_STAGE definition must already exist. This avoids committing a journal prefix that the current whole-buffer parser would reject due to an unresolved quarantine-action link. If a different ordering is needed, it must be explicitly designed and tested without accepting invalid complete histories.

### 5.3 MOVE and ACTION

- Enforce the exact parser transition table, error-code relation, and monotonically increasing sequence.
- Reject records after terminal states.
- Keep move and action lifecycles independent.
- Do not infer action completion from move completion.

## 6. Record and resource bounds

- Reject prepare when the next committed record would exceed `FCJ4_MAX_RECORDS`.
- The state must not advance on prepare, abort, serializer failure, or failed commit-byte comparison.
- A successful in-memory commit increments record count and revision exactly once.
- A failed or stale token cannot mutate evidence, operation/action state, record count, or revision.
- Keep one pending serialized record only; do not retain an unbounded journal history.
- The future append adapter remains responsible for the aggregate 64 MiB journal byte limit and exact file offset checks.

## 7. Required tests

Add tests that specifically target parser/writer divergence:

- SEALED stage device mismatch and stage-size mismatch are rejected at prepare.
- ACTION_DEF with wrong parent device/inode is rejected for each kind.
- ACTION_DEF with wrong entry device/inode, uid, mode, size, or digest is rejected.
- Wrong source component, wrong target role, malformed NONE sentinel, and invalid quarantine-link component are rejected.
- Valid quarantine action and its follow-up definition are accepted in the defined order.
- MOVE and ACTION sequence/state/error boundaries match parser behavior.
- Maximum operation/action/record bounds are enforced.
- Interleaved operations remain independently valid.
- Replay import followed by destruction of the source replay still supports valid and invalid next-record decisions.
- A fresh state and replay-imported state accept/reject identical test vectors.
- Failed import is all-or-nothing.
- Every prepared output in positive tests is accepted by the existing parser when combined with a valid complete history.
- Negative tests demonstrate that records rejected by the writer would be rejected by the parser if manually appended.
- C11/C17/C23 Debug+ASan/UBSan, Release, Makefile, Docker/non-root CI remain green.

## 8. Explicit non-goals

- No append adapter, file descriptor, lock, file creation, write, fsync, close, or journal path.
- No FCJ4 production source-list integration or CLI exposure.
- No FCJ2/legacy or FCJ3 changes.
- No filesystem evidence capture, planner, executor, quarantine, publication, source retirement, deletion, or recovery mutation.
- No automatic tail repair, truncation, or prefix acceptance.

## 9. Implementation status and exit criteria

The parser operation view now exposes the exact last MOVE sequence as an additive field. The writer now enforces FCJ4_MAX_RECORDS against its committed-record revision before prepare; this is a bounded partial implementation, not completion of record accounting because replay-import seeding is not implemented. The remaining writer-state implementation, replay import, deep ownership, cross-record validation, and regression coverage are still required before this milestone can be considered complete.

The implementation PR must pass all tests above and be reviewed against the existing parser source line-by-line. Record exact commands and observed outcomes; do not infer Codespaces or CI results. Only after this milestone is reviewed, merged by the repository owner, and verified on main may a separate append-adapter design be considered.
