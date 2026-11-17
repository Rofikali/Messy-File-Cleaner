# RUST-01A — FCJ4 C parser test seam (isolated, no Rust FFI)

**Implementation:** C-only test-support milestone branched from `main` `3a978a882765cb772681b6f595b469a513e39ef5` after design [PR #95](https://github.com/Rofikali/Messy-File-Cleaner/pull/95) merged. **Status: PR candidate; do not claim test results before exact-head CI.** This is the first implementation slice of [RUST-01 design proposal](../architecture/RUST_01_TEST_SHIM_DESIGN.md), not approval of Rust unsafe-code, production FFI or journal adoption.

## Contract

- `tests/support/fcj4_parser_test_shim.h` is a private (not installed) C test-support interface. `tests/support/fcj4_parser_test_shim.c` accepts a borrowed pointer + length; its only downstream dependency is the existing **pure-memory** `src/journal/fcj4_parser.c`.
- `fcj4_test_parse_bytes` deterministically zeroes non-NULL outputs before validation; invalid output pointers or a null buffer with nonzero length yield **-2 / EINVAL**. `NULL,0` is passed to the actual parser and rejected **-1 / EBADMSG**. Do not make up an empty success result.
- Valid data yields **0**, captures operation/record counts, and sets error output to zero. Malformed/truncated/too-large data yields **-1**, immediately saves the C parser's `errno` before replay destruction and returns zero counts. The replay is initialized and destroyed **once per call**. The shim never exposes the replay's heap-allocated C pointers, views, allocators, journal ownership or on-disk action APIs.
- The input is never mutated or converted to UTF-8. No filesystem access, mutating recovery, plugins, network, installed C API or Rust runtime.

## Tests

- `tests/unit/test_fcj4_parser_shim.c` repeats a known-valid `BEGIN` record, compares its untouched input buffer, alternates valid/invalid calls to expose state leakage, and checks malformed record kinds, truncation, embedded NUL, invalid UTF-8, and parser max-journal refusal. `NULL,0`, `NULL,nonzero` and missing output pointer cases are verified.
- `CMakeLists.txt` declares `test_fcj4_parser_shim`/`fcj4_parser_test_shim_unit` **inside BUILD_TESTING only**. Explicit source file list is only the new test, private shim and existing FCJ4 parser.
- Required exact-head evidence: C11/C17/C23 Debug+Release matrix, CTest new name, Docker, Makefile, Ubuntu where triggered; confirm via CMake `FC_SOURCES` that production `file-cleaner` includes **neither** the shim **nor** `fcj4_parser.c`. Local tests not run by ChatGPT in this milestone.

## Boundaries and next milestone

**RUST-01A is not a Rust FFI implementation.** The existing Rust `fcj4-test-support` crate retains `#![forbid(unsafe_code)]` and is unchanged. A separately reviewed **RUST-01B** must decide whether and how to introduce an isolated, audited `extern "C"` Rust boundary without weakening that original crate or linking Rust into production.

**Critical:** RUST-01A does not fix the known same-UID source-retirement vulnerability; FS-01B-FIX and REL-01 remain P0 blocked. A successful parser test is not proof of journal authenticity, crash durability or file mutation safety. Rollback is reverting the test-only files and CMake target; no user-data migration is involved.
