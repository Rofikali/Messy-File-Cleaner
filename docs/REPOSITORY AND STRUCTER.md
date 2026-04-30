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
├── Makefile
└── README.md
```

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
│   └── queue.c
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
├── collision/              # Naming logic
│   ├── collision.c
│   └── collision.h
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
└── utils/
```

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

---

# 9. Build System

---

## Makefile Layout

```make
srcs = $(wildcard src/**/*.c)
objs = $(srcs:.c=.o)

all:
    gcc -O2 -Wall $(objs) -o bin/file-cleaner
```

---

## Build Targets

* `make` → build
* `make clean`
* `make test`

---

# 10. CI/CD Pipeline (Recommended)

---

## Steps:

1. Build
2. Run unit tests
3. Run integration tests
4. Static analysis (clang-tidy)
5. Lint check

---

# 11. Coding Standards

---

## Rules:

* No global mutable state (except config singleton)
* Explicit error handling
* No silent failures
* Consistent naming

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

---

# 13. Release Strategy

---

## Steps:

1. Merge to main
2. Tag version
3. Build binary
4. Publish

---

# 14. Future Extensions (Repo Level)

---

## Planned Additions:

* `perf/` → benchmarking tools
* `bench/` → latency tests
* `docs/architecture/` → deep dives

---

# 15. Summary

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
