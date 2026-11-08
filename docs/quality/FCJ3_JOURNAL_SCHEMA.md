# FCJ3 Journal Schema and Transaction Identity

## Status

Accepted protocol contract. The FCJ3 parser/replay implementation is being introduced as an isolated, read-only module and test target; it is not connected to the current FCJ2 writer or recovery path and does not authorize staging cleanup.

## 1. Why FCJ3 exists

The current recovery model groups operations by source and destination paths. That key is not a transaction identity: the same source/destination pair can be used by multiple operations over time. Adding a `STAGED` word to FCJ2 would therefore permit records from distinct operations to be conflated.

FCJ3 gives each move operation a unique identity and persists immutable staging ownership metadata in every transition. FCJ2 and legacy records remain readable under their existing conservative semantics.

## 2. Record grammar

A complete FCJ3 line is UTF-8/ASCII framing with a final LF and exactly 16 pipe-separated fields:

```text
FCJ3|timestamp_ns|operation_id|sequence|state|error_code|src_len|dest_len|dir_dev|dir_ino|stage_dev|stage_ino|stage_len|src_hex|dest_hex|stage_hex\n
```

| Index | Field | Contract |
|---:|---|---|
| 0 | Magic | Exact literal `FCJ3` |
| 1 | timestamp_ns | Decimal unsigned integer; informational ordering aid, not identity |
| 2 | operation_id | Exactly 32 lowercase hexadecimal characters (128-bit random ID) |
| 3 | sequence | Decimal unsigned integer; starts at 1 and increments by exactly 1 per operation |
| 4 | state | One of `STAGED`, `STARTED`, `PUBLISHED`, `COMPLETED`, `FAILED`, `RECOVERED` |
| 5 | error_code | Decimal integer in `[0, INT_MAX]`; state-specific rules below |
| 6 | src_len | Source path byte length before hex encoding |
| 7 | dest_len | Destination path byte length before hex encoding |
| 8 | dir_dev | Decimal unsigned `st_dev` of the validated destination directory |
| 9 | dir_ino | Decimal unsigned `st_ino` of the validated destination directory |
| 10 | stage_dev | Decimal unsigned `st_dev` of the staging inode |
| 11 | stage_ino | Decimal unsigned `st_ino` of the staging inode |
| 12 | stage_len | Staging component byte length |
| 13 | src_hex | Lowercase hexadecimal encoding of exactly `src_len` bytes |
| 14 | dest_hex | Lowercase hexadecimal encoding of exactly `dest_len` bytes |
| 15 | stage_hex | Lowercase hexadecimal encoding of exactly `stage_len` bytes |

All numeric parsing must reject empty strings, signs, whitespace, trailing characters, and overflow. Decimal values must use canonical form: `0` or a nonzero digit followed by digits. Leading zeros are rejected except for zero itself.

Hex fields must have exactly twice the declared byte length. Decoded paths must be nonempty, fit the project path bound, and contain no NUL byte. Hex input is canonical lowercase on write; the parser may accept uppercase only if compatibility policy explicitly permits it. The initial FCJ3 implementation should require lowercase to avoid multiple encodings of the same record.

The line buffer must be sized from explicit maxima and checked arithmetic. Reject a line that is too long, lacks LF, has CRLF if CRLF is not explicitly supported, or has any extra/missing field. Never truncate or repair the journal automatically.

## 3. Operation identity

- Generate 128 bits from the operating system CSPRNG (Linux `getrandom()`) before the first FCJ3 record.
- If secure random generation fails or returns an incomplete result that cannot be retried safely, fail the operation. Do not substitute timestamps, PIDs, counters, addresses, or `rand()`.
- Encode as 32 lowercase hex characters.
- The operation ID is the primary key. Source/destination path equality is not a key.
- An operation ID may appear in multiple records only as a sequence of transitions for the same operation.
- Reuse of an operation ID for different metadata is journal corruption and fails closed.
- A duplicate operation ID/sequence pair is corruption, even if the bytes happen to match. Do not silently deduplicate records.

## 4. Immutable metadata

These fields must be byte-for-byte/value-for-value identical in every record for one operation:
- operation_id
- source path bytes
- destination path bytes
- destination directory device and inode
- staging component bytes
- staging inode device and inode

The staging component is one filename component, not a path. It must:
- start with the exact reserved prefix `.fc-tmp-` and have a nonempty suffix;
- be shorter than `NAME_MAX`;
- not equal `.` or `..`;
- contain no slash, NUL, control byte, or path separator.

The `.fc-tmp-` prefix is a namespace check, not ownership proof. Recovery must still verify the recorded parent directory and exact staging inode identity, and must use the private quarantine boundary before any unlink.

The component name alone never proves ownership. Cleanup requires the operation ID's durable record plus exact parent-directory and staging inode identity checks. No prefix sweep is permitted.

The four device/inode values are decimal unsigned integers parsed without overflow into the platform representation. The implementation must verify round-trip representability before comparing them with `struct stat`. Do not assume `dev_t` or `ino_t` is always 64-bit.

## 5. State machine

The initial FCJ3 transition graph is:

```text
                 +----------+
                 |  STAGED  |
                 +----------+
                   |      |
             STARTED      FAILED
                   |
              +----------+
              |  STARTED |
              +----------+
                |      |
           PUBLISHED  FAILED
                |
           +------------+
           | PUBLISHED  |
           +------------+
             |        |
        COMPLETED   RECOVERED
```

Additional recovery from `STAGED` or `STARTED` may append `RECOVERED` after conservative reconciliation. A `PUBLISHED` operation may append `COMPLETED` or `RECOVERED`, never `FAILED` as a way to erase publication evidence.

Legal transitions:
- first record: `STAGED`, sequence 1;
- `STAGED -> STARTED`, sequence 2;
- `STAGED -> FAILED`, sequence 2;
- `STAGED -> RECOVERED`, sequence 2 (recovery before visible staging publication);
- `STARTED -> PUBLISHED`, sequence 3;
- `STARTED -> FAILED`, sequence 3;
- `STARTED -> RECOVERED`, sequence 3;
- `PUBLISHED -> COMPLETED`, sequence 4;
- `PUBLISHED -> RECOVERED`, sequence 4.

No records may follow `COMPLETED`, `FAILED`, or `RECOVERED`. A repeated terminal record is corruption, not an idempotent append. Recovery idempotency means it observes the terminal state and appends nothing.

Error-code rules:
- `STAGED`, `STARTED`, `PUBLISHED`, and `COMPLETED` require error code 0.
- `FAILED` requires a nonzero error code.
- `RECOVERED` may use 0 for clean reconciliation or a nonzero code to record a conservative unresolved outcome; the semantics must be documented at the call site.

Sequence values are tied to the legal transition graph, not wall-clock timestamps. A gap, duplicate, decrease, or unexpected next state fails closed.

## 6. Mixing FCJ3 with FCJ2/legacy

A journal may contain older FCJ2/legacy operations and new FCJ3 operations appended later. The parser must maintain two independent replay models:
- FCJ2/legacy: preserve existing source/destination grouping and behavior exactly.
- FCJ3: group exclusively by operation ID and validate sequence/immutable metadata.

Never merge an FCJ3 operation with an FCJ2/legacy record because their paths match. An old-format record has no staging ownership identity and cannot authorize FCJ3 cleanup. Unknown magic/version is corruption; do not guess its format.

## 7. Replay and corruption policy

The parser must reject:
- malformed/truncated lines and oversized records;
- unknown magic, wrong field count, bad state;
- invalid or noncanonical numbers and numeric overflow;
- invalid operation ID or hex;
- length/hex mismatch, NUL bytes, empty required paths;
- invalid staging component;
- values not representable by the target `dev_t`/`ino_t`;
- operation ID metadata mismatch;
- duplicate ID/sequence;
- sequence gaps or illegal state transitions;
- records after terminal states;
- capacity exhaustion beyond the explicit maximum transaction count.

On any such error, recovery returns an error and leaves journal bytes and user filesystem entries unchanged. It must not append `RECOVERED`, truncate, reorder, or rewrite records.

## 8. Resource bounds

The first parser implementation fixes these bounds explicitly:
- source path: 1 through `PATH_MAX - 1` bytes;
- destination path: 1 through `PATH_MAX - 1` bytes;
- staging component: 1 through `NAME_MAX - 1` bytes;
- decimal field: at most 20 digits (the canonical unsigned 64-bit range);
- operation ID: exactly 32 lowercase hexadecimal characters;
- replay transaction count: at most 4,096 distinct operation IDs;
- transitions per operation: at most 4, because the legal state graph is strictly sequential and terminal;
- maximum line bytes: `261 + 4 * (PATH_MAX - 1) + 2 * (NAME_MAX - 1)`, including the final LF. On Linux systems with `PATH_MAX=4096` and `NAME_MAX=255`, this is 17,149 bytes.

The replay table grows dynamically and never exceeds 4,096 entries. On a typical LP64 Linux build, a fully populated table is approximately 34 MiB; the exact allocation is `FCJ3_MAX_TRANSACTIONS * sizeof(Fcj3Transaction)` and is platform/compiler dependent. The parser does not allocate a transaction table merely to parse one record.

The parser reports malformed or illegal records as `EBADMSG`, configured line/transaction limits as `E2BIG`, and allocation failure as `ENOMEM`. Every allocation-size multiplication and line-length calculation is bounded before use. It does not allocate based on untrusted numeric fields beyond these fixed limits.

## 9. Writer/API migration

Do not change existing `journal_record_started/published/completed/failed/recovered` signatures to pretend they can supply FCJ3 identity. Introduce a separate operation context/API that is initialized with:
- secure operation ID;
- validated source and destination path bytes;
- validated destination directory identity;
- staging component;
- staging inode identity.

The context owns immutable metadata for the operation lifetime. Transition APIs accept the context and validate the legal next state. The mover must not independently reconstruct or mutate these fields.

FCJ3 writing is enabled only when the O_TMPFILE staging lifecycle can provide all required metadata in the required order. Until then, existing FCJ2 writing and recovery remain unchanged.

## 10. Implementation phases

1. **Schema contract:** accepted in PR #42.
2. **Pure parser/model:** implemented as `include/journal/fcj3_parser.h`, `src/journal/fcj3_parser.c`, and `tests/unit/test_fcj3_parser.c`. The parser is currently compiled only into its dedicated test target; it is not linked into the production executable or called by `journal_recover()`. It has no file I/O and no recovery side effects.
3. **Writer/context:** implemented in `include/journal/fcj3_writer.h`, `src/journal/fcj3_writer.c`, and `tests/unit/test_fcj3_writer.c`. This module generates IDs with Linux `getrandom()`, captures immutable metadata from caller-provided stat snapshots, prepares canonical transition lines, and advances state only through an explicit commit call. It is compiled only into a dedicated test target; no production append or mover integration is enabled. See [FCJ3 Writer Context Design](FCJ3_WRITER_CONTEXT_DESIGN.md) and [ADR 0002](../architecture/adr/0002-fcj3-operation-context.md).
4. **Staging integration:** destination-specific O_TMPFILE probe, staging/link order, durable journal adapter, and fault injection.
5. **Recovery:** exact identity checks, private quarantine and crash/adversarial tests.
6. **Enablement:** only after non-root, supported-filesystem, C11/C17/C23, Debug/Release, sanitizer, static-analysis, Docker, and crash test gates pass.

## 11. Acceptance criteria

The schema is accepted only after review confirms:
- exact field order and canonical encoding;
- 128-bit CSPRNG operation ID policy;
- legal transition graph and error semantics;
- compatibility separation for FCJ2/legacy;
- strict duplicate/sequence/metadata validation;
- bounded parser memory and line size;
- no cleanup authorization in parser-only scope.

No production FCJ3 record writer or staging cleanup is enabled by this schema document alone.
