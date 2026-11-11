# ADR: Controlled Rust Adoption Strategy

## Status

Accepted — staged rollout authorized by owner merge of PR #81. This authorizes only the test-tooling stages described below; it does not authorize production Rust, production FFI, or a runtime dependency.

## Context

Messy-File-Cleaner is an established C systems project with data-preservation and filesystem-safety requirements. Its current engineering contract is:

- C17 is the production baseline.
- C11 and C23 are independently validated compatibility targets.
- Linux/POSIX is the current platform target.
- The production executable and filesystem mutation paths must remain small, auditable, and explicitly tested.
- FCJ4 parser, serializer, writer state, and append adapter have strict isolation gates. The FCJ4 append adapter is currently test-only.
- Existing C APIs, persistent journal formats, and filesystem behavior must not change as an incidental consequence of introducing another language.

The owner accepted this ADR by merging PR #81. That decision authorizes a narrowly scoped Rust test-tooling pilot; it does not change the C17 production baseline or authorize Rust in the product core.

## Decision proposal

Adopt Rust incrementally, beginning with **test and verification tooling only**. Keep C17 as the production implementation language during the pilot.

The first pilot should be a Rust-based, test-only fuzz/property-test harness that exercises the existing C FCJ4 parser through a narrow test FFI boundary. Rust must not become a second implementation of the FCJ4 grammar in this phase. The C parser remains the single source of protocol truth.

Why this pilot:

- Rust's ownership and type system can make the harness itself safer while generating and managing hostile byte sequences.
- Fuzzing can explore parser input space beyond hand-written fixtures.
- The harness does not add Rust to the production executable or create a new production ABI.
- Existing C11/C17/C23 production compatibility remains unchanged.
- A test-only harness can be removed without changing user data, the CLI, or the runtime product.

This is an evaluation, not a commitment to rewrite the product in Rust.

## Proposed pilot boundaries

Approved staged rollout scope:

- A separate Cargo workspace or isolated Rust test package under a clearly named test-support directory.
- A small C test shim exposing only the minimum parser entry points needed by the harness.
- Rust-generated arbitrary byte buffers and structured valid/invalid record sequences.
- Assertions for parser outcomes, all-or-nothing replay behavior, deterministic replay, resource-limit handling, and absence of input mutation.
- Reproducible seeds and minimized regression cases promoted into the existing C test fixtures when useful.
- CI job isolated from production build artifacts.

Explicitly excluded:

- Rust production executable, daemon, CLI, or library linked into the shipped binary.
- Rewriting the C parser, serializer, writer state, append adapter, mover, filesystem layer, or recovery logic.
- Changing FCJ2, FCJ3, or FCJ4 on-disk formats.
- Enabling FCJ4 production journal paths, startup integration, recovery execution, planner/executor, or filesystem mutation.
- Changing the C17 production baseline or dropping C11/C23 validation.
- Introducing Tokio, async runtime, plugin frameworks, databases, or broad dependency trees without a separately justified need.
- Exposing Rust-owned strings, vectors, structs, trait objects, panics, or allocator ownership across a C ABI.
- Treating Rust memory safety as proof of filesystem correctness, journal authenticity, or crash durability.

## FFI safety contract for any future boundary

If a later, separately approved milestone requires Rust/C interoperability:

1. Keep the ABI C-compatible and versioned; use opaque handles and fixed-width scalar types.
2. Pass byte buffers as pointer-plus-length; never rely on NUL termination for arbitrary data.
3. Define allocator ownership explicitly. Memory allocated on one side must be released by that same side through an exported matching function.
4. No Rust references, slices, `String`, `Vec`, enums with unspecified representation, trait objects, or Rust layout structs cross the ABI.
5. Catch unwinding at every exported Rust boundary; panics must never unwind through C. Prefer panic-abort only where the product's failure policy explicitly accepts process termination.
6. Validate null pointers, lengths, alignment, integer conversions, aliasing, and lifetime requirements before constructing Rust references or slices.
7. No callback may outlive its context; document reentrancy, thread safety, blocking, cancellation, and destruction rules.
8. Keep unsafe Rust in small audited modules, with a written safety invariant for each unsafe block.
9. Build and test with pinned toolchain/dependency versions and a reviewed supply-chain policy.
10. ABI changes require compatibility tests and a separate design review; no ABI is introduced by the pilot.

## Build and supply-chain policy

- Pin the Rust toolchain using `rust-toolchain.toml`.
- Commit `Cargo.lock` for the test package.
- Start with the standard library and the smallest practical dependency set.
- Require `cargo fmt --check`, `cargo clippy -- -D warnings`, and `cargo test` for the Rust pilot.
- Run Rust tests and the existing C suites independently; a green Rust test does not replace C11/C17/C23, ASan/UBSan, TSan, or Docker checks.
- Review every dependency for maintenance, license, transitive dependencies, unsafe code, and build-script behavior.
- Do not download or execute unpinned code during production builds.
- Keep the Rust toolchain out of the production runtime image unless a later approved production component requires it.

## Acceptance criteria for the pilot

The pilot is acceptable only when:

- A clean Ubuntu 24.04 environment can run the documented Rust test workflow reproducibly.
- The harness links only against test-support C objects, never the production executable.
- No Rust symbols or runtime dependencies appear in the production binary.
- The harness tests parser properties without duplicating the FCJ4 grammar.
- Fuzz findings are reproducible and minimized; confirmed bugs become ordinary C regression tests.
- Existing C11/C17/C23 validation, sanitizers, Makefile workflows, Docker checks, and production link-isolation checks remain green.
- CI clearly separates Rust test tooling from production compilation and packaging.
- Toolchain/dependency updates are explicit and reviewable.

## Rollout stages

1. **ADR review:** agree on the pilot scope, ownership rules, toolchain policy, and CI cost.
2. **Toolchain-only PR:** add pinned Rust toolchain and a minimal Cargo package that builds without linking to production targets. No parser FFI yet.
3. **Test-shim PR:** add the smallest test-only C shim and verify it is excluded from production link inputs.
4. **Fuzz/property-test PR:** exercise the existing C parser; preserve minimized regression cases in C tests.
5. **Evaluation:** measure bug-finding value, CI time, maintenance cost, and dependency exposure.
6. **Separate production proposal (optional):** only if the pilot demonstrates concrete value, evaluate one isolated production component with a new ADR, ABI design, threat model, benchmarks, and rollback plan.

Each stage is a separate reviewable milestone. Do not start a dependent stage before the preceding stage is merged and its checks are verified.

## Consequences

Positive:
- Introduces Rust skills and tooling without destabilizing the established C product.
- Adds a memory-safe environment for hostile-input generation and test orchestration.
- Creates evidence before making a production language decision.

Costs and risks:
- Adds another compiler/toolchain and CI maintenance surface.
- Rust/C FFI can introduce undefined behavior if ownership and pointer contracts are weak.
- A duplicated parser implementation would create consistency risk; therefore it is excluded from the pilot.
- Fuzzing does not prove absence of bugs and does not simulate real power loss or all filesystem behaviors.

## Pilot evaluation questions

- Is a test-only Rust fuzz/property-test harness an acceptable first pilot?
- Should the project use `cargo-fuzz`/libFuzzer integration, or begin with deterministic property tests and add coverage-guided fuzzing after CI/resource limits are understood?
- Which CI time budget and fuzz duration are acceptable?
- Does the owner want to keep C17 as the production baseline after the pilot, or consider a future component-by-component language review based on evidence?

## Current safety statement

The accepted ADR authorizes isolated Rust test tooling only. Each later stage still requires its own reviewed and merged milestone. The production application remains C17; no production Rust code, C/Rust FFI, shared/static Rust library, or Rust runtime dependency is authorized.
