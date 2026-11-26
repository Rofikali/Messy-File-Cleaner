# C API, ABI, and Rust FFI Roadmap

**Upstream strategy:** [Master plan](../upstream/UPSTREAM_MASTER_PLAN.md) proposes static/shared libraries behind stable versioned C contracts. No exposed kernel internals, new ABI commitment or production Rust/FFI authorization; reconcile existing ABI roadmap first and review static/shared license obligations.

**Planning cross-reference (2026-11-22):** the [advanced feature roadmap](../ADVANCED_FEATURE_ROADMAP.md) puts a public C `.a` / `.so`, ABI stabilization and Rust FFI in a **future gated stage**; the [acceptance matrix](../ADVANCED_FEATURE_ACCEPTANCE.md) requires a real consumer, licensing, ABI/ownership tests, security review and explicit owner approval. This document is **DESIGN ONLY**, not permission for production FFI or SDK release. See the [live board](../../next.md) and [test-only Rust boundary](RUST_TEST_SUPPORT_BOUNDARY.md).

## Decision
Keep the current engine in C. Design a small public C API first, stabilize its ABI only after ownership rules and tests exist, and add Rust FFI only when a concrete Rust consumer exists.

## Terms
- API: source-level contract used by callers, typically public C headers and documented functions.
- ABI: compiled calling convention, symbol names, type layout, alignment, and binary contract.
- FFI: mechanism that lets one language call another. Rust calling a C ABI is Rust FFI.

A C header alone does not guarantee ABI stability. Struct layout, symbol changes, calling convention, and ownership also matter.

## Proposed layering

    CLI or future service adapter
              |
          Public C API
              |
        Versioned C ABI
              |
         Internal C modules
              |
        POSIX filesystem

A future Rust adapter calls the C ABI using extern C.

## Public API rules
- Use an opaque fc_engine_t handle; do not expose AppContext, queues, pthreads, FILE pointers, or internal hash maps.
- Use fixed-width integer types and explicit enum values.
- Pass strings as pointer plus length, or document strict NUL-terminated string validation.
- Add an ABI version function and negotiation.
- Extensible option structs include struct_size and abi_version.
- Define one owner for every allocation and matching release functions.
- Return explicit status codes.
- Document thread safety, callback lifetime, cancellation, and handle concurrency.
- Hide internal symbols; export only documented public functions.
- Never allow C++ exceptions, Rust panics, longjmp, or unwinding across the ABI.

## Suggested first API surface (direction only; not implemented)

    typedef struct fc_engine fc_engine_t;
    typedef int32_t fc_status_t;
    uint32_t fc_abi_version(void);
    fc_status_t fc_engine_create(const struct fc_options *options,
                                 fc_engine_t **out_engine);
    fc_status_t fc_engine_run_once(fc_engine_t *engine);
    void fc_engine_destroy(fc_engine_t *engine);

The actual interface must match the engine lifecycle and be tested before release.

## When Rust is justified
Use Rust when a real Rust consumer needs embedding, a new isolated security-sensitive module benefits from ownership types, or a measured replacement improves safety/performance. Do not rewrite scanner, mover, or queue merely to use Rust. FFI adds unsafe boundaries, ownership complexity, build complexity, and debugging cost.

## Phased delivery
1. Define module contracts and improve tests.
2. Move global state behind an engine context.
3. Extract a public C API around the stable lifecycle.
4. Add ABI versioning, symbol visibility, and C consumer tests.
5. Add a Rust wrapper for a real consumer; keep unsafe code small and audited.
6. Benchmark integration and maintain compatibility tests.

## References
- Rustonomicon FFI: https://doc.rust-lang.org/nomicon/ffi.html
- GNOME API stability guidance: https://developer.gnome.org/documentation/guidelines/maintainer/api-stability.html


## PUBLIC-API-01 — first design-only candidate

Baseline 70817c9a606b22907be675b6ac74df7f7ce57f87; UPSTREAM-COMPAT-01 closed scoped 5/5. Reconcile provisional roadmap API-01 and PUBLIC-API-01: this is the public interface design stage of the former, not a second independent budget for the same work. Static/shared delivery and ABI/FFI remain separately gated as documented. Observed `src/main.c`: global AppContext ctx, static HashEntry buffer[64], process signal handlers, config and canonical-root validation, optional recovery/open journal, worker start/stop, scanner callback without user context, daemon watcher thread, metrics/stdout and cleanup. `include/core/context.h` exposes queues, eventfds, atomics, config, classifier, MoveJournal and metrics internally. Directly exporting these, invoking main, or claiming multi-instance/reentrant behavior would couple consumers to global lifecycle, filesystem effects and platform internals. Do not modify production here.

First proposed consumer is a local read-only batch preview caller, analogous to existing aggregate JSON CLI; a hypothetical design fixture, not a confirmed real external consumer. [Header](../../tests/reference/public_api_candidate.h) and [compile-only caller](../../tests/reference/public_api_candidate.c) are test fixtures outside include/install and FC_SOURCES; no implementation, exported symbol or link test. Candidate names deliberately distinct from final ABI. Opaque handle; fixed-width status/counter types; size/version prefixes; explicit byte lengths; summary aggregate only. Proposed future contract (not implemented): create rejects NULL out pointer, initializes output to NULL before other validation, validates size/version, lengths and NUL/embedded NUL/empty input before filesystem access, owns copied strings only after validated allocation, no caller-buffer retention. Preview is synchronous read-only: no file moves, journal creation, recovery or daemon; validate output size/version before access, report complete=1 only on successful full run, incomplete/partial counts explicitly documented, reserved zero. Caller owns summary storage; library owns handle and copied resources; destroy(NULL) no-op, only valid owned handle once; no concurrent or reentrant calls, signal handler installation, callbacks or cross-language unwinding in first design. No execution until actual root/config containment and cancellation/error/cleanup contracts are proven. No permission escalation or paths in aggregate result. Pointer width and padding not fixed by field widths; no struct-layout portability or ABI stability claim. C17 baseline, C11/C2x syntax compatibility only; six GCC/Clang compile-only probes in existing isolated development image, no linking or running fixture, no new dependencies/runner. Local tests NOT RUN; exact-head CI pending. Future slots require concrete consumer and ownership/error-state evidence; stop at design if owner/architecture gates unmet. Production API/ABI/.a/.so/FFI implementation, licensing/distribution and release require separate explicit approval. No imports, spend, production changes, FS-01B/REL-01 change. Rollback fixture and CI revert.


## PUBLIC-API-01 — proposed internal CLI consumer boundary (second candidate)

#271 merged at `0083d7ff25edecc85ecd6d8fc718b8e0c2213b92`, tested head `1dbf969b2bf64601c7d13050b67e5b67a164bf5e`, 13/13 returned PR-head jobs PASS. #272 (2/5) maps the concrete existing `src/main.c` batch `--dry-run --once --report-json` consumer target: it currently emits aggregate counts and complete based on scan, cancellation, worker shutdown and worker error status. The existing CLI does NOT call the candidate API; no real external consumer or production refactor is approved. The compile-only caller accepts proposed input pointer/length pairs and stages the summary locally, rejects null output, version mismatch, success with null handle, failed preview, mismatched size/version, incomplete or nonzero reserved result. Only a successful complete compatible result is copied to caller storage; the valid acquired handle is destroyed after preview even on error. It is not safe to destroy an unexpected pointer on create failure without ownership; create must leave it null by contract. The existing CLI can emit incomplete aggregate diagnostics; this example is a success-only adapter and does not establish parity for error diagnostics. No filesystem action, linking, runtime tests or ABI proof; six existing syntax probes on GCC/Clang C11/C17/C2x only, pending CI. No production changes, dependencies or costs added. Before any implementation, resolve actual lifecycle extraction, ownership/error-state runtime evidence, concrete consumer approval, license, cancellation and containment. FS-01B/REL-01 remain blocked. Rollback: revert candidate PR.


## PUBLIC-API-01 — isolated mock caller oracle (third candidate)

#272 merged at `1071b60baf6be4768e70f45932d133429068a8ab`, tested head `39e7143d3799f9db513eaf0b04701ed02a970ff0`, 13/13 returned PR-head jobs PASS; formal independent review records not returned. #273 (3/5) links the unchanged proposed caller ONLY with a test-local synthetic stub in [mock test](../../tests/reference/public_api_candidate_mock_test.c), never with real engine code. Ten scenarios: success, null output, version mismatch, create failure, unexpected null handle, preview failure after partial writes, malformed size, malformed version, incomplete result, nonzero reserved field. Explicit field-by-field output sentinel checks, status, call counts and mock argument/handle checks; successful acquired handle destroyed exactly once on modeled preview paths. No padding comparison or allocator/filesystem access; no actual engine lifecycle/ownership assurance. Existing isolated Ubuntu development image executes six builds (GCC/Clang C11/C17/C2x) with strict warnings; no new runner or dependency. Local NOT RUN, exact-head CI pending. The earlier compile-only restriction remains for the actual candidate engine (no implementation, installation or ABI); synthetic caller-only execution is not engine runtime proof or a real consumer approval. Preserve license, security and architecture gates; production ABI/FFI and FS-01B/REL-01 blocked. Rollback: revert PR.


## PUBLIC-API-01 — synthetic allocated ownership and recovery (fourth candidate)

#273 merged at `0e72e1d60e5032563548f453f2ea00fefc6d08e1`, tested head `fc8f596b694a6882927fdb67611d63d404ff85e6`, 13/13 returned PR-head jobs PASS; Ubuntu image job 139949119557 logs six compiler/standard executions with ten synthetic cases each PASS. Formal independent review records not returned. #274 replaces static mock handle with test-local heap allocation and copies the one-byte fixture inputs into owned mock storage; checks live-handle state and allocation/release deltas across every scenario followed by a positive retry, without resetting ownership. Expect 20 calls and 16 matching allocations/releases per build. Existing six isolated compiler modes retained; additionally execute same synthetic mock with existing hosted Clang 18 ASan/UBSan, fatal errors and leak detection. No actual engine allocation/cleanup or input ownership proof, no filesystem operations, no dependency or new runner. Local NOT RUN; CI pending. One final slot remains; do not treat test-only ownership as production API readiness. FS-01B/REL-01, imports/license, external consumer and production ABI/FFI gates unchanged. Rollback: revert this PR.


## PUBLIC-API-01 — final candidate: production isolation and handoff

#274 merged at `84ea67e73cd93ad2fe3e5b5595d7f60072b78ce0`, tested head `d1a89864f51708372d164b5240b1f85e5bfbec8f`, 13/13 returned PR-head jobs PASS. Ubuntu image job 139954896454 logs seven synthetic builds with 20 calls each, 112 matched allocations/releases in aggregate and hosted Clang ASan/UBSan leak checks PASS. Formal independent review records not returned. #275 uses the existing isolated Clang production-only C11/C17/C23 builds and a test-local positive mock symbol fixture; [isolation gate](../../scripts/tests/check_public_api_isolation.sh) requires readable ELF symbol tables, production main, absence of fc_candidate_ symbols, and a private staged install containing exactly bin/file-cleaner with identical binary bytes. Negative controls substitute the mock executable as production and insert a synthetic unexpected installed file; each must be rejected for the expected reason. These checks apply only to the three observed artifacts; no proof against all possible source leakage or stripped binaries, no ABI proof or installable SDK. No new packages or runners; no production source changes. Local NOT RUN; exact-head CI pending. This is the fifth opened PR; no sixth. Close scoped design acceptance only after verified gates (otherwise partial/blocked). Subsequent STATIC-LIB-01 requires separate reconciliation with existing roadmap and evidence for real lifecycle extraction, actual consumer and license/ownership decisions before any production library change. A safe non-import maintenance or design-only milestone may be selected under the existing policy if implementation is blocked; do not claim approval to expose production ABI, FFI, release or destructive behavior. FS-01B/REL-01 unchanged. Rollback: revert this test and CI PR.
