# RUST-01 — Test-only FCJ4 C parser shim design (proposal)

**Maturity:** DESIGN_ONLY. **Delivery:** PR_OPEN pending review/CI. No shim, FFI, Cargo linkage, or production behavior is implemented by this document. Base: `main` `d9777d6a34d59339c8e0501cb565a5620189320a` (PR #94 merged). Do not infer acceptance from a documentation merge.

## Existing verified boundary

- `include/journal/fcj4_parser.h` exposes `fcj4_replay_init`, `fcj4_replay_parse(const void *, size_t, Fcj4Replay *)`, and `fcj4_replay_destroy`. The parser accepts arbitrary non-NUL-terminated bytes and returns 0 or -1 with `errno`. On failure an initialized empty replay remains empty.
- `tests/unit/test_fcj4_parser.c` exercises the parser through a C test binary. `tests/rust/fcj4-test-support/src/lib.rs` currently preserves arbitrary bytes without FFI and forbids unsafe Rust.
- `CMakeLists.txt` explicitly enumerates production sources; FCJ4 parser is **not** in the production `FC_SOURCES` list. The accepted Rust ADR authorizes test tooling only, not production FCJ4 or a public ABI.

## Proposed smallest interface — not yet approved for implementation

Create a test-only C translation unit under `tests/` that owns the complete `Fcj4Replay` lifetime inside one function. Proposed interface, illustrative and subject to independent review:

```c
/* test-only; no public header, installation or product symbol */
int fcj4_test_parse_bytes(const unsigned char *bytes, size_t length,
                          int *out_errno, size_t *out_operations,
                          size_t *out_records);
```

Return 0 for successful parse, -1 for parser rejection and a separate documented invalid-argument code (e.g. -2). Initialize outputs deterministically on every path. Reject `bytes == NULL && length != 0`; permit null with zero length only if verified safe against the actual C parser. Validate output pointers; never dereference a null pointer or fabricate an input byte. Snapshot `errno` immediately after parser failure **before** destroying the replay. Initialize and destroy the C replay exactly once; do not expose its internal pointers, `Fcj4OperationView`, ownership or allocator to Rust. All input remains borrowed for the call only; no callbacks, filesystem access, journaling or source retirement.

Rust's one minimal audited `unsafe` FFI module must justify pointer/length and output lifetime contracts; `#![forbid(unsafe_code)]` in the existing crate cannot simply be weakened globally. Prefer a separate tightly scoped crate/module with `deny(unsafe_op_in_unsafe_fn)`, documented safety invariants, and safe wrapper that accepts `&[u8]`. No Rust panic may cross the C boundary. C never invokes Rust, so unwinding cannot cross into C through callbacks. Map C return/errno to a stable test result without treating `errno` as a production API.

## Build isolation and negative acceptance

- Link only `src/journal/fcj4_parser.c` and the proposed test shim into a dedicated test binary/static test archive. **Do not** modify production `FC_SOURCES`, install headers, or ship a `.a`/`.so` as a product.
- Prove via link-input inspection and `nm`/dependency inspection that `file-cleaner` has no shim, FCJ4 parser or Rust runtime symbols. Keep production Docker image and executable unchanged.
- Add tests for empty bytes, embedded NUL, invalid UTF-8, malformed/truncated records, maximum journal bound and length beyond bound, null+nonzero length refusal, and repeated valid/invalid calls without stale state. Compare Rust shim outcomes against the existing C parser fixtures, not a second Rust grammar.
- Preserve the all-or-nothing replay property, no input mutation, deterministic outputs, no filesystem mutation. A green parser test does **not** establish journal authenticity, safe crash recovery, or FS-01B source-retirement safety.
- Run `cargo fmt --check`, `cargo clippy -- -D warnings`, `cargo test`, C11/C17/C23 Debug/Release tests, sanitizer tests and applicable Ubuntu 24.04/Docker PR CI on the **implementation** PR. This design-only PR has no newly executed local tests; required CI must be checked at its own exact head.
- Prefer deterministic tests before any expensive fuzz dependency. No new network, dependency, paid CI or runtime cost is authorized by this design.

## Decision and rollback

Before implementing the shim, review actual C parser dependencies, build/link strategy, `NULL,0` behavior, Rust unsafe-code exception, output contract and CI cost. This document is a **design proposal**, not approval of a production ABI or new filesystem semantics. Rollback of a future test shim is removal of the isolated test target and crate FFI adapter; existing production binaries and user data must remain unaffected.

**Safety gate unchanged:** FS-01B-FIX and REL-01 remain P0 blocked pending accepted trust/crash semantics and positive preservation regressions. FCJ3/FCJ4 and Rust stay test-only.
