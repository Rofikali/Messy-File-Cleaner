# Rust Test-Support Boundary and Native Artifact Policy

## Status

Approved implementation stage: toolchain and isolated test-support scaffold only.

This document supplements `docs/architecture/ADR_RUST_ADOPTION_STRATEGY.md`. It does not authorize a production Rust component, a public C ABI, a shared library, or changes to the production executable.

## Current artifact decision

The product remains a C17 command-line application. Its production deliverable is the `file-cleaner` executable.

The Rust test-support crate is built and run only by Cargo's test workflow. Cargo may create temporary build artifacts (including an internal Rust library artifact and a test executable) under its build directory. Those artifacts are not installed, packaged, linked into `file-cleaner`, or exposed as product libraries.

There is no current requirement for `libfilecleaner.so`, `libfilecleaner.a`, a Rust `.so`, or a Rust production executable. Do not add one merely because the toolchain can produce it.

## What the common artifact types mean

| Artifact | Meaning | Appropriate use |
|---|---|---|
| Executable | A runnable program | Current product: `file-cleaner` |
| Static library (`.a` on Linux) | Object code linked into another program at build time | A future embedded/reusable core when consumers can relink |
| Shared library (`.so` on Linux) | Dynamically loaded code with a runtime ABI/deployment contract | A future independently deployed library with real external consumers |
| Rust `rlib` | Cargo/Rust compiler library artifact | Internal Rust crate composition; not a stable C ABI |
| Rust `staticlib` | Rust code packaged for linking by non-Rust consumers | Possible future Rust component linked into a native executable |
| Rust `cdylib` | Rust dynamic library intended for foreign-language consumers | Possible future C-ABI library when dynamic loading is a real requirement |

Platform names differ: Linux commonly uses `.so`, macOS uses `.dylib`, and Windows uses `.dll`. File extensions alone do not create a safe or stable interface.

## Decision rules for a future library

1. **No external consumer, no library.** Keep the executable as the product boundary.
2. **One executable embedding reusable code:** evaluate a static library first. It avoids a runtime shared-library deployment dependency, although consumers must relink to receive updates.
3. **Independent consumers or independently updated components:** evaluate a shared library only after defining ABI versioning, symbol visibility, compatibility policy, loader behavior, packaging, and security updates.
4. **Rust code linked into a C executable:** choose Rust `staticlib` or `cdylib` only after a separate production-component ADR and C-ABI review. Do not expose Rust-native types.
5. **Rust test harness calling C:** prefer linking the existing C parser objects into a test-only harness through a narrow shim. A production `.so` is not required for that test boundary.

A shared library introduces runtime loader paths, symbol collision/visibility concerns, ABI compatibility obligations, deployment and patching coordination, and a larger test matrix. A static library reduces runtime deployment complexity but does not provide a stable binary ABI by itself.

## Test-support crate restrictions

The current Rust crate:

- Is located under `tests/rust/` and is test tooling only.
- Uses the pinned toolchain in the repository root.
- Has no third-party dependencies.
- Forbids unsafe Rust.
- Handles raw byte inputs only; it does not parse or define FCJ4 records.
- Does not call C, use FFI, perform filesystem operations, or link into production targets.
- Must not change the C17 production baseline or C11/C17/C23 validation.

The first helper deliberately treats parser input as arbitrary bytes, including embedded NUL and invalid UTF-8. It must not convert hostile input to text as a prerequisite for testing.

## Gates before any C/Rust FFI

A later test-shim milestone must separately document and verify:

- Exact C symbols and function signatures exposed to tests.
- Pointer/length and null-pointer rules.
- Ownership and lifetime of all buffers and handles.
- Error mapping and integer conversion rules.
- No panic/unwind crossing the C boundary.
- The shim and all Rust objects are absent from production link inputs and installed artifacts.
- Adversarial tests for empty, oversized, malformed, and arbitrary binary input.

A production FFI/library proposal requires a new ADR, threat-model update, ABI compatibility tests, benchmark or safety justification, release/packaging design, and rollback plan.

## Rollback

Remove `rust-toolchain.toml`, `tests/rust/fcj4-test-support/`, and the isolated Rust workflow. No production binary, user data, journal format, or runtime behavior depends on them.
