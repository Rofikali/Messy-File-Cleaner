# RUST-FFI-EVAL-01 — private classifier consumer feasibility

Status: design-only proposal; implementation BLOCKED until explicit owner scope approval and staged ADR review. This proposal does not amend the accepted [Rust ADR](ADR_RUST_ADOPTION_STRATEGY.md) or [test support boundary](RUST_TEST_SUPPORT_BOUNDARY.md). The existing FCJ4 test-support crate remains unchanged and forbids unsafe Rust. C17 production and C11/C23 compatibility remain unchanged.

## Evidence and concrete consumer

Shared reference #285 merged at 2c5942cf5a7db41cc185197bc58c93a403892d0f, exact tested head 51a53665b886ff464a5fac2cf43dc85c83d89a63. Returned jobs 10/10 PASS; GCC C17 Release 77/77 (140294002440), Debug 73/73 (140294002520), final artifact isolation PASS. Shared feature PARTIAL 5/5: Ubuntu/Clang final evidence and independent formal review not returned. Static feature likewise PARTIAL 5/5. Do not infer stable ABI, distribution clearance or production linkage.

Proposed concrete **internal** consumer: a separate test-only Rust executable verifies classification of arbitrary filename bytes through the existing C classifier/hashmap. It must not reuse or weaken the existing FCJ4 crate, expose public engine API, access a filesystem, allocate across the FFI boundary, or imply actual external customer demand. No production Rust or public SDK.

## Alternatives and decision

- Retain C-only tests (current approved baseline): least toolchain and ABI complexity; no foreign-language evidence.
- Proposed static C test shim + Rust test executable: no runtime loader deployment, small explicit unsafe boundary, easy isolation. Prefer for first cross-language experiment **only after approval**.
- Dynamic Rust consumer of existing private DSO: later option, needs ABI/loader policy, dependency/path tests and separate scope acceptance. Do not link directly to current NUL-terminated adapter with arbitrary byte slices.
- Rust rewrite, cdylib/staticlib shipped to users, or C calling Rust: rejected for this scope; no concrete production benefit or authorization.

## Proposed contract (not implemented)

Candidate signature: `int32_t fc_ffi_reference_route_v1(const uint8_t *bytes, size_t len);` Rust equivalent uses `*const u8`, `usize` and `i32` on matched Linux/POSIX target. Unique private test symbol, no public header/install. Version suffix is local experiment identity, not ABI stability.

Status 1 = jpg route; 0 = fallback; -1 = invalid input; -2 = fixture setup failure. Scope case-sensitive byte matching using the existing C classifier, not a parallel classifier in Rust. Length 1..4096 inclusive; reject null, zero length, oversized length before reading, and embedded NUL. Non-UTF-8 is valid if no embedded NUL. Caller must supply readable initialized storage of at least len bytes for duration of call, and must not concurrently mutate it. A null check cannot prove pointer provenance or allocation extent; forged/dangling/undersized pointers are not valid inputs. Do not exercise UB by inventing invalid non-null pointers.

C copies validated input to a bounded local 4097-byte buffer, appends NUL, invokes test-local classifier context with fixed route mapping and returns a scalar only. No caller pointer retained, allocation handoff, returned borrowed pointer, callbacks, global classifier use, filesystem effects or C-to-Rust calls. Rust wrapper accepts a borrowed slice, checks length and interior NUL, makes one audited unsafe extern-C call with live slice pointer, converts statuses to a Rust result, treats unexpected status as error; no panic/unwinding across C. Do not use Rust native layouts at boundary. No concurrency guarantee claimed; test sequentially first. No allocation cross-release.

## Proposed staged work and acceptance (not authorized implementation)

1. This design PR: reconcile existing Rust authorization, concrete consumer, ownership contract, negative cases, costs and explicit approval. It consumes one of at most five opened PRs for RUST-FFI-EVAL-01; remaining slots are ceilings, not required work.
2. After owner approval and reviewed merge: implement private C shim, validate C-only contract first, no new production target. Verify size limits, byte copying, classifier parity, scalar error mapping and exact symbol set.
3. After preceding verified merge: isolated dependency-free Rust test consumer under tests/rust/ with pinned existing toolchain and Cargo.lock, minimal scoped unsafe module and written invariant; leave FCJ4 crate's forbid(unsafe_code) intact. Use static test archive compiled from approved existing C modules plus shim; no network downloads of dependencies, no auto-install of toolchain by product build.
4. If justified: adversarial differential tests using deterministic seeds and matching C oracle, ASan/UBSan C-side instrumentation plus appropriate Rust checks where supported; record gaps in cross-language sanitizer support, no claims from uninstrumented code. Test 0,1,4095,4096,4097 lengths; NUL at first/middle/last position; high-bit bytes; trailing dot, hidden file, directory dot, jpg/txt, missing extension, repeated mutable inputs, positive retries, unexpected status mapping. Do not call C on invalid non-null pointers.
5. If needed: artifact isolation, measured CI time, dependency/maintenance and rollback evaluation. CMake BUILD_TESTING=OFF, normal Make/Docker, staged CLI-only install and symbol/dependency scans must remain independent of Rust. Verify exact head and preserve existing safety gates.

CI acceptance after approval: pinned compiler identity, cargo fmt --check, clippy -D warnings, offline locked cargo tests for the isolated package, C11/C17/C23 GCC and Clang modes and existing Docker/Makefile/sanitizer jobs; distinguish missing workflow from pass. No new paid runner, dependency or external source import. Compare matched C-only baseline and candidate CI runtime, artifact size, complexity and defect-finding value; no production speed claim. Independent review must cover security, ownership, ABI, build isolation, supply chain and financial/maintenance cost.

## Approval gate and rollback

Owner decision needed: authorize **only** a private classifier test shim and isolated Rust consumer with the pointer/length contract above, minimal unsafe module, existing pinned toolchain, no third-party crates, no production linkage/install/public ABI and no paid services. Review the staged ADR and exact scope before any implementation. A general go-ahead or roadmap listing is not blanket FFI approval. No new Rust code, C shim, build target or FFI connection is introduced by this proposal. If owner declines or evidence is inadequate, close PARTIAL/BLOCKED and select independent safe work; do not exhaust slots by default. Production FFI/SDK, license, external import, FS-01B and REL-01 stay blocked.

Rollback: revert this design PR (preserve actual PR history and consumed budget when reconciling ledger); no runtime effect. Subsequent implementation rollback must remove only isolated test artifacts and keep C production unchanged.


## Reconciliation after proposal merge

#286 merged at `15e720b63f897709b31623c9cbea2cc081602c52` (tested head `1eee3583dd10fb42b7d021dc27825324a044238a`), 9/9 returned exact-head jobs PASS; formal review not returned. Proposal merge is not implementation scope approval. RUST-FFI-EVAL-01 is BLOCKED at 1/5 pending explicit owner scope approval and staged ADR review. No new Rust FFI, crate, C shim or unsafe code. Preserve the stable ID and consumed slot if later resumed. Independent upstream maintenance can proceed without weakening this gate.
