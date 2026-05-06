# File Cleaner System — High-Level Design (HLD) (Architecture + Evolution Ready)

---

# 1. Overview

This document defines a **production-grade High-Level Design (HLD)** for a file cleaner system, upgraded with:

* **Architecture evolution strategy**
* **Event-driven extensibility**
* **Scalability pathways**
* **IO-bound system optimization**

The system starts as a **local CLI utility** and evolves into a **hybrid event-driven processing system**.

---

# 2. Core Design Philosophy

> “Start simple, design for extension, evolve without rewriting.”

Key principles:

* Prefer **monolith first**
* Introduce **abstractions at boundaries**
* Separate **producers from consumers**
* Optimize for **IO, not CPU**

---

# 3. Architecture Style

## ✅ Primary: **Layered + Pipeline Architecture**

## ✅ Evolution: **Event-Driven Hybrid Architecture**

---

# 4. System Evolution Stages (CRITICAL)

---

## 🔹 Stage 1 — Batch Processing (Initial System)

```text
[CLI] → [Scanner] → [Processor Pipeline] → [File System]
```

### Characteristics:

* Pull-based
* Synchronous
* Simple and deterministic

---

## 🔹 Stage 2 — Decoupled Pipeline (Foundation for Scaling)

```text
[Scanner] → [Event Queue] → [Processor]
```

### Improvements:

* Decoupling via queue
* Enables concurrency
* Prepares for event-driven model

---

## 🔹 Stage 3 — Event-Driven Hybrid (Target Architecture)

```text
        [Scanner] (Batch)
              ↓
        [Event Queue] → [Processor Pipeline]
              ↑
        [Watcher] (Real-time)
```

### Key Insight:

* Multiple producers
* Single unified pipeline

---

## 🔹 Stage 4 — Concurrent Processing

```text
[Producers] → [Queue] → [Worker Pool] → [File System]
```

---

## 🔹 Stage 5 — Distributed (Future Optional)

```text
[Nodes] → [Distributed Queue] → [Workers]
```

---

# 5. High-Level Components

---

## 5.1 CLI Interface

### Responsibilities:

* Accept root directory
* Load configuration
* Initialize system

---

## 5.2 Configuration Service

### Responsibilities:

* Load extension mappings
* Provide immutable runtime config

---

## 5.3 Event Producers

---

### 🔹 Scanner (Batch Producer)

* Traverses directory (DFS)
* Emits file events

---

### 🔹 Watcher (Real-Time Producer)

* Uses Linux `inotify`
* Emits file system events

---

## 5.4 Event Queue (Architectural Core)

### Role:

* Decouples producers and consumers

### Properties:

* High throughput
* Non-blocking (future)
* Backpressure handling

---

## 5.5 Processing Pipeline

---

### Sub-components:

#### 1. Classifier

* Determines file category

#### 2. Resolver

* Resolves destination path

#### 3. Executor

* Performs file movement

---

## 5.6 File System Interface

### Responsibilities:

* Rename
* Copy
* Delete

---

## 5.7 Observability Layer

### Responsibilities:

* Logging
* Metrics (future)
* Error tracking

---

# 6. Data Flow (Unified)

```text
Input → Producers → Event Queue → Pipeline → File System → Logs
```

---

# 7. Key Architectural Decisions

---

## 7.1 Monolithic vs Distributed

### Decision:

✅ Monolithic (initial)

### Reason:

* Local tool
* No network overhead
* Simpler failure model

---

## 7.2 Pipeline Design

### Decision:

✅ Sequential processing stages

### Reason:

* Clear responsibility separation
* Easier debugging

---

## 7.3 Event-Driven Integration

### Decision:

✅ Hybrid model

### Reason:

* Combines batch + real-time
* No rewrite required

---

## 7.4 Queue-Based Decoupling

### Decision:

✅ Introduced early (Stage 2)

### Reason:

* Enables scaling
* Supports multiple producers

---

# 8. Scalability Strategy

---

## 8.1 Vertical Scaling

* Efficient IO usage
* Memory reuse

---

## 8.2 Horizontal Scaling (Future)

* Multiple worker threads
* Partitioned queues

---

## 8.3 Throughput Optimization

* Batch event processing
* Reduce syscalls

---

# 9. Reliability & Fault Tolerance

---

## 9.1 Failure Domains

| Component | Failure Impact |
| --------- | -------------- |
| Scanner   | Missed files   |
| Watcher   | Missed events  |
| Queue     | Backlog        |
| Processor | Partial work   |

---

## 9.2 Recovery Strategy

* Retry failed operations
* Periodic full scan (safety net)

---

## 9.3 Crash Safety

* Temp file pattern
* Atomic rename

---

# 10. Backpressure Handling

---

## Problem:

Queue overflow during event bursts

---

## Solutions:

* Drop low-priority events (optional)
* Slow producers
* Expand queue size

---

# 11. Event Consistency Model

---

## Guarantees:

* At-least-once processing
* Eventual consistency

---

## Trade-off:

* Possible duplicate processing (handled via idempotency)

---

# 12. Idempotency Design

---

## Strategy:

* Safe reprocessing
* File existence checks

---

# 13. Security Considerations

---

* Path validation
* Permission checks
* Safe file handling

## Safety Boundaries:

The system must be conservative because it mutates user files.

Default behavior:

* Dry-run mode available for all commands
* Refuse dangerous roots like `/`, `/bin`, `/usr`, `/etc`, `/var`
* Refuse home directory root unless `--force` is passed
* Skip symlinks by default
* Skip special files such as sockets, device files, and FIFOs
* Never delete source before durable destination is confirmed
* Never follow paths outside the configured root after normalization

---

# 14. CLI and Operating Modes

---

The CLI is the user-facing control plane.

## Required Modes:

```text
file-cleaner --root <path> --config <file> --dry-run
file-cleaner --root <path> --config <file> --once
file-cleaner --root <path> --config <file> --watch
file-cleaner --root <path> --config <file> --workers <n>
file-cleaner --root <path> --restore <journal>
```

## Mode Semantics:

| Mode | Behavior |
| ---- | -------- |
| `--dry-run` | Print planned operations without moving files |
| `--once` | Run scanner once and exit |
| `--watch` | Run scanner, then watch for new events |
| `--workers` | Configure processing concurrency |
| `--restore` | Use journal to inspect or reverse operations |
| `--force` | Allow risky roots after explicit user intent |

Default mode for early releases should be `--dry-run` or require explicit confirmation before mutation.

---

# 15. Symlink Policy

---

## Default:

* Ignore symlinks

---

## Reason:

* Avoid cycles
* Prevent unintended access

---

# 16. Observability

---

## Logging:

* Success/failure
* Errors

---

## Metrics:

* files/sec
* bytes/sec
* Queue depth
* queue overflow count
* event duplicates
* move failures by `errno`
* watcher debounce count
* crash recovery count

## Logs Must Include:

* operation id
* source path
* destination path
* event type
* result
* `errno` where applicable
* elapsed time

---

# 17. Recovery and Undo Strategy

---

The system should maintain a move journal for every mutating filesystem operation.

## Journal Responsibilities:

* Record operation before mutation
* Mark operation complete after durable destination is confirmed
* Mark operation failed with error code on failure
* Support startup recovery for incomplete temp files
* Support user-facing restore or audit flow

## Recovery Guarantees:

* No partial file should appear as final output
* Source should be preserved on copy failure
* Incomplete temp files should be cleaned on startup
* Duplicate or stale events should not create data loss

---

# 18. Queue and Consistency Model

---

## Queue Contract:

The queue is bounded. Overflow behavior must be explicit.

Default policies:

* Scanner producer blocks when queue is full
* Watcher producer coalesces duplicate events
* If watcher bursts exceed capacity, mark root dirty and schedule full scan
* Workers must drain before shutdown

## Consistency Guarantees:

* At-least-once event processing
* Eventual consistency after full scan
* Duplicate events are allowed
* Missing files are treated as stale events
* File operations must be idempotent

---

# 19. Test and Validation Strategy

---

Testing must cover filesystem edge cases, not only pure data structures.

## Required Test Matrix:

| Area | Cases |
| ---- | ----- |
| Queue | empty, full, wraparound, overflow, shutdown |
| Scanner | nested dirs, long paths, permissions, hidden files |
| Watcher | duplicate events, burst events, file still being written |
| Classifier | unknown extension, case normalization, default mapping |
| Collision | existing names, high collision count, atomic create |
| Mover | rename fast path, EXDEV copy, ENOSPC, EACCES, EINTR |
| Recovery | crash during copy, stale temp files, incomplete journal |
| Safety | symlink skip, special file skip, dangerous root refusal |

System tests should run in temporary directories only.

---

# 20. Performance Budget

---

The system is IO-bound, so performance work should be driven by measurements.

## Initial Budget:

| Metric | Target Direction |
| ------ | ---------------- |
| Same-filesystem move | Prefer `rename()` only |
| Cross-filesystem move | Sequential buffered copy |
| Queue operation | O(1) bounded memory |
| Classification | Average O(1) |
| Memory | Preallocated hot path |
| Watch latency | Debounce-window bounded |

## Profiling Tools:

* `perf stat`
* `perf record`
* `strace -c`
* ASan / UBSan debug builds
* stress tests with generated file trees

---

# 21. Performance Characteristics

---

## Nature:

* IO-bound system

---

## Key Optimization Areas:

* Disk operations
* Syscall reduction
* Queue throughput

---

# 22. Bottlenecks

---

| Component | Bottleneck |
| --------- | ---------- |
| Disk IO   | Primary    |
| Queue     | Secondary  |
| CPU       | Minimal    |

---

# 23. Trade-offs

---

| Decision     | Benefit    | Cost                     |
| ------------ | ---------- | ------------------------ |
| Monolith     | Simplicity | Limited distribution     |
| Queue        | Decoupling | Complexity               |
| Event-driven | Real-time  | Event consistency issues |
| Dry-run first | User safety | Slower first-run workflow |
| Journal | Recovery and undo | Extra writes |
| Bounded queue | Predictable memory | Backpressure complexity |

---

# 24. Future Extensions

---

## 24.1 Event Sources

* Cloud storage events
* Network file systems

---

## 24.2 Advanced Features

* Duplicate detection
* Content-based classification
* Undo command
* Ignore rules
* File age and size rules
* Quarantine mode before final move

---

## 24.3 Distributed System

* Kafka-based queue
* Multi-node workers

---

# 25. System Diagram (Final Form)

```text
          +------------------+
          |       CLI        |
          +------------------+
                    ↓
          +------------------+
          |     Config       |
          +------------------+
                    ↓
     +-----------------------------+
     |        Event Producers      |
     |  [Scanner]   [Watcher]      |
     +-----------------------------+
                    ↓
          +------------------+
          |   Event Queue    |
          +------------------+
                    ↓
          +------------------+
          | Processing Layer |
          | Classify/Move    |
          +------------------+
                    ↓
          +------------------+
          |   File System    |
          +------------------+
                    ↓
          +------------------+
          |      Logs        |
          +------------------+
```

---

# 26. Summary

This architecture provides:

* Clean separation of concerns
* Seamless evolution to event-driven model
* High reliability and safety
* Strong scalability foundation

---

## Final Insight

> The system evolves from a simple batch processor into a **hybrid event-driven architecture** by introducing a queue and abstracting event sources—without rewriting core logic.

---

**End of Document**
