# C API, ABI, and Rust FFI Roadmap

**Owner-approved Linux-inspired library architecture (2026-11-27):** [Engineering requirements](LINUX_INSPIRED_LIBRARY_ARCHITECTURE.md). This owner-approved architecture governs static/shared/FFI acceptance; logical module boundaries do not require separate physical archives. Initial #276 Clang Release failure (job 140235055451) corrected by EOF newline-only changes; independent same-environment C17 Release builds will compare archive bytes and member order, CI pending.

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


## STATIC-LIB-01 — first private archive reference

PUBLIC-API-01 scoped design acceptance DONE 5/5: #275 merged at `f000517ef7ad81934cfed141e05489831833c82a`, tested head `1aa97d59198a4eb2105b5fef7b2c40692129fa1a`, 13/13 returned PR-head jobs PASS; Ubuntu image job 139957790194 logs three production artifact checks and two negative controls per standard PASS. Formal independent review records not returned. No actual public engine API implemented; no sixth PR.

#276 selects the distinct, test-only static-link feasibility milestone from the upstream master plan, not a production SDK or a continuation of the mock API. Existing classifier and hashmap implementations are unchanged; a private CMake STATIC target `fc_classifier_reference` exists only under BUILD_TESTING and is linked into a separate [internal reference consumer](../../tests/reference/static_classifier_consumer.c). Eight checks cover independent caller-owned contexts, basename and hidden-file handling, invalid input, and isolated updates. The map and strings remain caller-owned/borrowed internal types; this is not a stable ABI or a proof of thread safety, entire engine lifecycle or external consumer demand. Existing CMake matrix supplies compiler-mode execution; local NOT RUN, exact-head CI pending. No install/export target, production CLI relink, new dependency, external import, license decision, paid service, ABI/FFI or production library approval. Future work must preserve non-install boundary and require real consumer, lifecycle/ownership review and license clearance before production library changes. FS-01B/REL-01 blocked. Rollback: revert PR.


## STATIC-LIB-01 — second reference: symbol boundary

#276 merged at `696eff11165e9ed90049eabe81e32f6430650d2c`, tested head `bbbc2df9e4036f873c5f28c167fc3c5d8fd7ec31`, 14/14 returned jobs PASS; GCC C17 Release 71/71; Ubuntu job 140237565357 confirms identical independent same-environment Clang C17 Release archives, SHA-256 `52f7591b1d9e9a10b6d92847e4b461b26450dafed842627cfdb9261ad9adbf47` and two expected members. Formal independent review records not returned. #277 checks exactly five existing global text definitions using GNU-compatible nm POSIX output, retaining duplicate detection. Negative controls append a disposable extra definition and remove the hashmap member from separate copies. Both must fail with the expected diagnosis. This is a private archive definition boundary, not a stable public ABI, undefined-dependency allowlist, cross-platform proof or shared-library visibility test. Existing isolated Clang C17 Release workflow only; local NOT RUN and new-head CI pending. Production CLI, SDK installation, imports, license, FS-01B/REL-01 and ABI/FFI gates remain unchanged. Rollback: revert PR.


## STATIC-LIB-01 — third reference: dependency direction and instrumented consumer

#277 merged at `f8c54bfa1485a21a4b57e66d0c3e15fe2bff7972`, tested head `7e0ac2c97126ecc0739225fa4108c859d13370f5`. 13/13 returned jobs PASS; GCC C17 Release 71/71 (job 140238859938); Ubuntu job 140238863047 confirms archive repeatability, five defined global text symbols and extra/missing-definition negative controls. Independent review records were not returned. #278 inspects undefined symbol records per extracted archive member in the unsanitized Clang C17 Release reference: classifier requires hashmap_get and permits strrchr; hashmap permits only the listed libc string/memory functions and no reverse classifier edge. Disposable replacement objects check rejection of an unapproved call and reverse edge. Debug builds instrument both private archive objects and consumer with ASan/UBSan, fatal recovery disabled; hosted Clang reruns the consumer with leak detection. This is an observed binary dependency policy for a scoped toolchain, not a source-level proof, complete transitive dependency audit, ABI promise or thread-safety proof. Exact-head CI pending; no new dependency or production linkage. FS-01B/REL-01 and production library, license and FFI gates unchanged. Rollback: revert PR.


## STATIC-LIB-01 — fourth reference: matrix coverage and ownership

#278 merged at `c79157c217bb171507ee4e27f9bd7ca8029e9697`, tested head `dded04eaa3b8fc32395c6267aff031f54fac2176`; 10/10 returned jobs across CMake, Docker, Makefile and FCJ4 TSan passed; C17 Debug and Release 71/71. Ubuntu workflow and independent review were not returned. Targeted local Debian 13 x86-64 reconstruction matched all six relevant Git blob hashes; Clang 17 C17 dependency oracle, unauthorized/reverse negative controls and original eight consumer checks with ASan/UBSan/leak detection passed. GCC and Clang C11/C17/C2x Release dependency probes also passed (six modes). This does not establish hosted Ubuntu acceptance. #279 adds the existing positive dependency oracle to Release CTest and extends the independent consumer with caller-allocated two-entry heap storage, mutable insertion inputs, borrowed fallback identity, capacity rejection with existing entries preserved, successful full-map update and unchanged original contexts. All consumers cease use before free; no deliberate dangling-pointer exercise, production lifecycle or concurrent use proof. Existing Debug instrumentation retained. Exact-head CI pending. No production changes, installs, dependencies, imports, ABI/FFI approval or release. FS-01B/REL-01 blocked. Rollback: revert PR.


## STATIC-LIB-01 — final reference: artifact isolation

#279 merged at `45d4898aed6383b84863f34358562f909bb53249`, tested head `f5ce5075ff62596cf51fede05807d14ca40f4af8`; 10/10 returned jobs passed; GCC C17 Release 72/72 (job 140263254799), Debug 71/71 (job 140263254793). Extended consumer and Release dependency test PASS. Ubuntu workflow and formal independent review records not returned. #280 adds Release CTest artifact gate: five-symbol oracle, fresh BUILD_TESTING=OFF CLI build, exact missing-target diagnostics for private archive and consumer, private staged install containing only byte-identical CLI. Reject leaked archive and altered staged CLI. Local miniature Ninja/Make fixture confirmed missing-target diagnostics; full repository test not run locally. Exact-head CI pending. This is the fifth and final opened PR; reconcile scoped DONE/PARTIAL/BLOCKED using actual evidence, no sixth PR. This does not establish full cross-host reproducibility, actual engine lifecycle, installed SDK, public ABI, real external consumer, production performance or kernel-grade certification. Shared-library feasibility remains subject to separate stable-feature and approval gates; no production library/FFI, license decision, imports or release approval. FS-01B/REL-01 remain blocked. Rollback: revert PR.


## SHARED-LIB-01 — first private PIC reference

STATIC-LIB-01 closes PARTIAL 5/5, no sixth PR: #280 merged at `7173dadf41fd624baff4eb89fc871e2f7f3521ad`, tested head `7884cd87ca57b4c4fece98a4db5b0eb38f553184`, 10/10 returned jobs PASS, GCC C17 Release 73/73 (job 140264485442), Debug 71/71 (job 140264485356), final artifact isolation PASS. Earlier Clang same-environment repeatability and symbol tests passed; final Ubuntu-specific workflow and formal independent review records not returned. This is not production static-library acceptance.

#281 selects a distinct, private shared-link feasibility experiment, not a production SDK or bypass of the static-library gates. BUILD_TESTING-only target fc_classifier_shared_reference compiles unchanged classifier/hashmap and test-local adapter as a PIC shared object, hidden-by-default C visibility and one explicitly default-visible fc_shared_reference_route. The linker rejects unresolved link-time references (normal resolved libc imports remain valid). The adapter uses a fixed two-route fixture with caller-supplied NUL-terminated string or NULL, stack-local map, no pointer retention and scalar status; eight independent consumer scenarios exercise repeated routing and invalid input. No install/export, production relink, stable ABI, public interface, real external consumer or production FFI. Exact-head CI pending; full local tests NOT RUN. Later scoped acceptance should inspect actual dynamic exports, SONAME/loader/dependency behavior, sanitizer and isolation, within existing five-PR ceiling. License/import, production library, FS-01B/REL-01 gates remain blocked. Rollback: revert PR.
