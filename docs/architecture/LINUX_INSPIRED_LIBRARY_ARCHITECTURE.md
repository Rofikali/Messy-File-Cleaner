# Linux-inspired user-space library architecture

Owner-approved direction, 2026-11-27. Normative design requirements for STATIC-LIB-01, SHARED-LIB-01 and RUST-FFI-EVAL-01, subject to the [live selector](../../next.md), [five-PR policy](../FEATURE_DELIVERY_AND_ACCEPTANCE_POLICY.md), [API/ABI roadmap](C-API-ABI-FFI-ROADMAP.md) and [upstream reuse policy](../upstream/UPSTREAM_REUSE_POLICY.md). This is not permission to import source, publish libraries, change production ABI, spend money, or bypass filesystem safety.

## Decision and alternatives

Prefer a Linux-inspired hybrid: own portable C user-space core, supported POSIX/Linux interfaces, compatible mature upstream components only when individually justified and cleared. Do not copy kernel internals wholesale or claim Linux kernel-grade certification. Linux [Kbuild](https://docs.kernel.org/kbuild/makefiles.html) builds subsystem objects and archives for kernel linking; CMake/Makefile builds our user-space artifacts. Archive packaging does not itself confer speed, security or correctness. Own-only avoids import complexity but duplicates maintenance; direct kernel copying adds environment, synchronization, license and maintenance risks; hybrid requires evidence and deliberate boundaries. C17 baseline, C11/C23 compatibility; optional SIMD/assembly only with capability detection, portable fallback and measured benefit. No kernel modules, kernel allocator, scheduler or workqueue imports; no root requirement.

## HLD: logical subsystems, not mandatory physical archives

| Logical boundary | Possible private target | Responsibility and dependencies |
|---|---|---|
| Memory | libfc_memory.a | Owned allocation, bounds and cleanup; libc/portable platform primitives |
| Queue | libfc_queue.a | Bounded concurrency, atomics; explicit producer/consumer and memory-order contracts |
| Hash map | libfc_hashmap.a | Bounded lookup; caller-owned storage |
| Classifier | libfc_classifier.a | Routing; depends on hash map, no filesystem effects |
| Scanner | libfc_scanner.a | Descriptor-relative discovery, cancellation; platform and owned memory |
| Watcher | libfc_watcher.a | Event acquisition/invalidation; platform and queue |
| I/O | libfc_io.a | Bounded transfer and error propagation; platform; destructive paths remain gated |
| Journal | libfc_journal.a | Record validation and durability; platform; no unauthorized recovery |
| Workers | libfc_workers.a | Lifecycle/orchestration; queue, classifier and approved I/O |
| Platform | libfc_platform.a | Isolate Linux/POSIX capabilities and fallbacks |

These are candidates, not existing artifacts or a requirement to create ten archives. Use internal targets/object libraries or ordinary source modules when simpler. No gratuitous fragmentation, cyclic dependencies, exposed private structures, or duplicate implementations. Layering: CLI/desktop/SDK or real Rust consumer -> versioned public C contract -> libfilecleaner.a or libfilecleaner.so (future) -> core engine orchestration -> private modules -> POSIX/Linux. Linker dependency direction must be justified and checked; archives are not merged merely by nesting them. Avoid direct global context exposure; no production extraction until lifecycle, cancellation, filesystem effects and ownership are proven.

## LLD and acceptance gates

- Document each subsystem API, invariants, owner/borrower lifetimes, bounds, status/error semantics, cancellation, concurrency, state transitions, and rollback. Keep internal and public headers separate, namespace exports; use opaque handles and fixed-width types where appropriate. Avoid unwinding across C ABI; no unsupported ABI guarantees.
- Static: deterministic member order and archive metadata; compare clean same-toolchain builds with pinned inputs, flags and hashes; record ar/nm/linker identity. Test independent consumers, unresolved dependencies and permitted symbols, production CLI isolation, installation absence until approved. Reproducibility across hosts/toolchains is a separate claim. Debug paths, timestamps and toolchain differences must be accounted for.
- Shared: only after concrete need and clearance; PIC, hidden-by-default visibility, explicit export allowlist/version policy, SONAME, loader/dependency tests, ABI layout/compatibility, independent consumers and staged packaging. Avoid accidental exports and RPATH surprises.
- FFI: real consumer and explicit approval, C ownership/status/length/version contract, Rust unsafe boundary minimized, panic containment, allocator pairing, cancellation/concurrency and cross-language tests. No Rust production dependency until gates pass.
- Security: strict warnings for scoped code, negative tests, sanitizers, fault injection, concurrency verification where applicable, threat model, resource ceilings, path confinement, fail-closed behavior. Do not weaken existing CI or retire source files until FS-01B trust/crash proof.
- Performance: establish reproducible baseline and same-workload candidate comparisons (latency/tails, throughput, CPU/cycles, RSS, allocation counts, I/O/syscalls, correctness); record environment and uncertainty. Kernel-style architecture is not a benchmark result; reject regressions or unproven complexity. No nanosecond claims without measurement.
- Supply chain: explicit license decision, component-level exact source provenance, notices/SPDX, static/shared obligations and legal review, update cadence, integrity and rollback before imports; registry remains locked. Existing source is used locally for research only; no release rights presumed.
- CI: GCC and Clang, C11/C17/C23, Debug/Release, Docker/non-root, exact-head checks, negative controls, reproducible artifact evidence; manual deployment and release separately gated. Preserve one active feature and five opened PR ceiling; same-PR fixes free.
- Review across L1–L7, security/CISSP, Linux/SRE, CCIE where relevant, CA/MBA/management/legal: measure TCO (integration, CI, maintenance, compliance, distribution, opportunity cost), compare alternatives, document accountable owner and acceptance. No new spend or certification claim.

## Live implementation mapping

PR #276 is a BUILD_TESTING-only private classifier/hashmap archive with a separate C consumer. It is own code, not Linux source, and not a production library. Initial exact head 1729cc22f2e298bbbc95d0f418451095eebfd7fe had Ubuntu Clang Release failure on missing terminal newlines in three files (job 140235055451); correct those without suppressing strict warnings. Same-PR candidate adds a repeat-build archive byte comparison and expected member order in the existing isolated Clang C17 Release step. No independent host reproducibility, release, ABI, or performance claim. New head CI pending. Future steps are acceptance criteria under existing feature IDs, not an automatic queue or permission to exhaust budgets. Production library changes, external imports, FS-01B and REL-01 remain blocked.


## STATIC-LIB-01 — second reference: symbol boundary

#276 merged at `696eff11165e9ed90049eabe81e32f6430650d2c`, tested head `bbbc2df9e4036f873c5f28c167fc3c5d8fd7ec31`, 14/14 returned jobs PASS; GCC C17 Release 71/71; Ubuntu job 140237565357 confirms identical independent same-environment Clang C17 Release archives, SHA-256 `52f7591b1d9e9a10b6d92847e4b461b26450dafed842627cfdb9261ad9adbf47` and two expected members. Formal independent review records not returned. #277 checks exactly five existing global text definitions using GNU-compatible nm POSIX output, retaining duplicate detection. Negative controls append a disposable extra definition and remove the hashmap member from separate copies. Both must fail with the expected diagnosis. This is a private archive definition boundary, not a stable public ABI, undefined-dependency allowlist, cross-platform proof or shared-library visibility test. Existing isolated Clang C17 Release workflow only; local NOT RUN and new-head CI pending. Production CLI, SDK installation, imports, license, FS-01B/REL-01 and ABI/FFI gates remain unchanged. Rollback: revert PR.


## STATIC-LIB-01 — third reference: dependency direction and instrumented consumer

#277 merged at `f8c54bfa1485a21a4b57e66d0c3e15fe2bff7972`, tested head `7e0ac2c97126ecc0739225fa4108c859d13370f5`. 13/13 returned jobs PASS; GCC C17 Release 71/71 (job 140238859938); Ubuntu job 140238863047 confirms archive repeatability, five defined global text symbols and extra/missing-definition negative controls. Independent review records were not returned. #278 inspects undefined symbol records per extracted archive member in the unsanitized Clang C17 Release reference: classifier requires hashmap_get and permits strrchr; hashmap permits only the listed libc string/memory functions and no reverse classifier edge. Disposable replacement objects check rejection of an unapproved call and reverse edge. Debug builds instrument both private archive objects and consumer with ASan/UBSan, fatal recovery disabled; hosted Clang reruns the consumer with leak detection. This is an observed binary dependency policy for a scoped toolchain, not a source-level proof, complete transitive dependency audit, ABI promise or thread-safety proof. Exact-head CI pending; no new dependency or production linkage. FS-01B/REL-01 and production library, license and FFI gates unchanged. Rollback: revert PR.
