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

---

# 14. Symlink Policy

---

## Default:

* Ignore symlinks

---

## Reason:

* Avoid cycles
* Prevent unintended access

---

# 15. Observability

---

## Logging:

* Success/failure
* Errors

---

## Future:

* Metrics (files/sec)
* Queue depth

---

# 16. Performance Characteristics

---

## Nature:

* IO-bound system

---

## Key Optimization Areas:

* Disk operations
* Syscall reduction
* Queue throughput

---

# 17. Bottlenecks

---

| Component | Bottleneck |
| --------- | ---------- |
| Disk IO   | Primary    |
| Queue     | Secondary  |
| CPU       | Minimal    |

---

# 18. Trade-offs

---

| Decision     | Benefit    | Cost                     |
| ------------ | ---------- | ------------------------ |
| Monolith     | Simplicity | Limited distribution     |
| Queue        | Decoupling | Complexity               |
| Event-driven | Real-time  | Event consistency issues |

---

# 19. Future Extensions

---

## 19.1 Event Sources

* Cloud storage events
* Network file systems

---

## 19.2 Advanced Features

* Duplicate detection
* Content-based classification

---

## 19.3 Distributed System

* Kafka-based queue
* Multi-node workers

---

# 20. System Diagram (Final Form)

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

# 21. Summary

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
