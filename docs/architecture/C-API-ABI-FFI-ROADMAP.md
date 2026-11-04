# C API, ABI, and Rust FFI Roadmap

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
