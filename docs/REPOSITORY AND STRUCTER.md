# File Cleaner System — Branching Strategy & Repository Structure

---

# 1. Overview

This document defines:

* **Branching strategy** (how development flows)
* **Repository structure** (how code is organized)
* **Workflow rules** (how engineers collaborate)

Designed at **production-grade / L6 level** with:

* Stability
* Scalability
* Maintainability

---

# 2. Branching Strategy

## ✅ Chosen Model: **Trunk-Based Development (Simplified GitFlow Hybrid)**

---

## 🔷 Core Branches

```text
main        → Production-ready (stable)
develop     → Integration branch (optional for teams)
feature/*   → New features
bugfix/*    → Bug fixes
hotfix/*    → Critical production fixes
```

---

## 🔹 Branch Roles

### 1. `main`

* Always **stable**
* Tagged releases
* No direct commits

---

### 2. `develop` (optional for small team → can skip)

* Integration branch
* All features merged here before main

---

### 3. `feature/*`

```text
feature/event-queue
feature/inotify-watcher
feature/hashmap-config
```

* Short-lived
* Single responsibility
* Merged via PR

---

### 4. `bugfix/*`

```text
bugfix/collision-logic
bugfix/cross-fs-copy
```

---

### 5. `hotfix/*`

```text
hotfix/data-loss-fix
```

* Created from `main`
* Merged back to both `main` and `develop`

---

## 🔁 Workflow

```text
feature → develop → main
```

OR (simpler, preferred):

```text
feature → main (via PR, CI enforced)
```

---

## ✅ Rules

* PR required for all merges
* CI must pass (build + tests)
* No direct push to `main`
* Small, atomic commits

---

## ⚡ L6 Insight

> Prefer **trunk-based with feature flags** over long-lived branches to avoid merge hell.

---

# 3. Versioning Strategy

## ✅ Semantic Versioning

```text
vMAJOR.MINOR.PATCH
```

Examples:

* `v1.0.0` → initial release
* `v1.1.0` → new feature (watcher)
* `v1.1.1` → bug fix

---

# 4. Repository Structure

---

## 🔷 Root Layout

```text
file-cleaner/
│
├── bin/                # Compiled binaries
├── build/              # Build artifacts
├── configs/            # Config files
├── docs/               # HLD, LLD, design docs
├── include/            # Header files (.h)
├── src/                # Source code (.c)
├── tests/              # Test cases
├── scripts/            # Utility scripts
├── CMakeLists.txt
└── README.md
```

## Current Repository Notes:

The current project uses **CMake**, not Makefile-first builds. If a `Makefile` is added later, it should only wrap CMake commands.

Recommended generated directories:

```text
bin/        # Runtime output
build/      # CMake build tree
lib/        # Library output, if needed
perf/       # perf.data, flamegraphs, profiling notes
```

Do not commit large generated binaries or profiling outputs unless they are intentionally stored as benchmark artifacts.

---

# 5. Source Code Structure (C-Oriented)

---

## 🔷 `src/` Breakdown

```text
src/
│
├── main.c                  # Entry point
│
├── core/                   # Core pipeline
│   ├── pipeline.c
│   ├── event.c
│   ├── queue.c
│   ├── lf_queue.c
│   └── thread_pool.c
│
├── scanner/                # DFS traversal
│   ├── scanner.c
│   └── scanner.h
│
├── watcher/                # inotify integration
│   ├── watcher.c
│   └── watcher.h
│
├── classifier/             # Strategy pattern
│   ├── classifier.c
│   └── classifier.h
│
├── resolver/               # Rule engine
│   ├── resolver.c
│   └── resolver.h
│
├── mover/                  # File operations
│   ├── mover.c
│   └── mover.h
│
├── config/                 # Config loader
│   ├── config.c
│   └── config.h
│
├── utils/                  # Shared utilities
│   ├── hashmap.c
│   ├── string.c
│   └── fs_utils.c
│
└── logging/                # Observer/logging
    ├── logger.c
    └── logger.h
```

---

# 6. Include Directory

```text
include/
│
├── core/
├── scanner/
├── watcher/
├── classifier/
├── resolver/
├── mover/
├── config/
├── logging/
└── utils/
```

## Header Ownership Rule:

Public module contracts live under `include/`. Private helpers stay inside the matching `src/` module as `static` functions.

Each public header should document:

* ownership of pointers
* return values
* thread-safety
* error behavior
* whether the function mutates filesystem state

---

# 7. Config Structure

```text
configs/
│
├── default.conf
├── dev.conf
└── prod.conf
```

---

## Example Config

```text
.jpg=Jpeg/
.jpeg=Jpeg/
.png=Png/
.txt=Text/
default=Others/
```

## Config Rules:

* Config is loaded once at startup
* Runtime config is immutable after initialization
* Extensions should be normalized to lowercase
* Destination paths must be relative to the configured output root
* Config parse errors fail startup
* Unknown extensions use `default`

---

# 8. Tests Structure

```text
tests/
│
├── unit/
│   ├── test_hashmap.c
│   ├── test_queue.c
│
├── integration/
│   ├── test_pipeline.c
│
└── system/
    ├── test_full_run.sh
```

## Required Test Matrix:

| Area | Required Cases |
| ---- | -------------- |
| Queue | empty, full, wraparound, overflow, shutdown |
| Hash map | collisions, missing key, resize/load factor if supported |
| Scanner | nested dirs, hidden files, long paths, permission denied |
| Watcher | duplicate events, burst events, file still being written |
| Classifier | case normalization, unknown extension, default mapping |
| Mover | rename fast path, cross-filesystem copy, collision naming |
| Safety | symlink skip, special file skip, dangerous root refusal |
| Recovery | temp file cleanup, incomplete journal, failed copy |

System tests must use temporary directories and must never run against real user roots.

---

# 9. Build System

---

## CMake Layout

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

## Build Types:

| Build Type | Purpose |
| ---------- | ------- |
| `Debug` | ASan/UBSan, symbols, local debugging |
| `Release` | Optimized binary |
| `Profile` | Profiling-friendly build |

## Optional Makefile Wrapper:

If a Makefile is added, it should call CMake targets:

```make
build:
	cmake --build build

test:
	ctest --test-dir build --output-on-failure
```

---

# 10. CI/CD Pipeline (Recommended)

---

## Steps:

1. Build
2. Run unit tests
3. Run integration tests
4. Static analysis (clang-tidy)
5. Lint check

## Required Quality Gates:

* Debug build with ASan/UBSan passes
* Release build passes
* Unit and integration tests pass
* System tests pass in temporary directory
* No compiler warnings in touched modules
* No committed generated binaries unless approved
* Documentation updated for public behavior changes

## Recommended Tooling:

* `clang-tidy`
* `clang-format`
* `cppcheck`
* `valgrind` where sanitizer coverage is insufficient
* `strace -c` for syscall profile
* `perf stat` / `perf record` for IO and CPU profile

---

# 11. Coding Standards

---

## Rules:

* No global mutable state (except config singleton)
* Explicit error handling
* No silent failures
* Consistent naming
* Public functions return status values
* Filesystem mutations must be journaled
* Hot-path allocations require justification
* Queue operations must define full/empty behavior
* Threaded code must document ownership and shutdown

---

## Naming Convention

```text
snake_case (C style)
```

---

# 12. Dependency Rules

---

## Allowed:

* Standard C library
* POSIX APIs

## Avoid:

* Heavy external dependencies
* Hidden global state
* Ad hoc path parsing when POSIX helpers are available
* Recursive directory traversal for unbounded trees

---

# 13. Documentation Ownership

---

## Document Map:

| Document | Owns |
| -------- | ---- |
| `HLD.md` | Architecture, evolution stages, user-visible guarantees |
| `LLD.md` | Data structures, algorithms, contracts, failure handling |
| `designs.md` | Main flow sketches and design notes |
| `REPOSITORY AND STRUCTER.md` | Repo layout, branching, build, CI, standards |

## Update Rule:

Any PR that changes public behavior must update the matching document.

Examples:

* Queue behavior change → update LLD and tests
* CLI flag change → update HLD and README
* Directory structure change → update repository structure doc
* File move semantics change → update LLD, HLD, and system tests

---

# 14. Runtime Artifacts

---

Runtime and generated artifacts should be separated from source.

```text
bin/                 # built executable
build/               # CMake files
logs/                # runtime logs, usually gitignored
journal/             # move ledger, usually gitignored
perf/                # local profiling output, usually gitignored
generated_files/     # local stress-test data, gitignored
```

The move journal is runtime state, not source code. It should be configurable and excluded from normal commits.

---

# 15. Release Strategy

---

## Steps:

1. Merge to main
2. Tag version
3. Build binary
4. Publish

---

# 16. Future Extensions (Repo Level)

---

## Planned Additions:

* `perf/` → benchmarking tools
* `bench/` → latency tests
* `docs/architecture/` → deep dives
* `docs/runbooks/` → recovery and debugging guides
* `.github/workflows/` → CI automation
* `.clang-format` → formatting rules
* `.clang-tidy` → static analysis rules

---

# 17. Summary

This setup ensures:

* Clean separation of modules
* Scalable development workflow
* Safe collaboration via branching
* Production-ready structure

---

## Final Insight

> A strong system is not just about code—it’s about how code evolves safely over time.
> Branching strategy and structure define long-term success.

---

**End of Document**
