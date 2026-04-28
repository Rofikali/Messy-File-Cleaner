# File Cleaner System — Low-Level Design (LLD) (Performance + DSA Integrated)

---

# 1. Overview

This document defines a **production-grade, performance-oriented LLD** for a file cleaner system.

Key upgrades in this version:

* Explicit **DSA integration**
* **IO-bound optimization focus**
* **Queue-based event pipeline**
* **Cache-friendly structures**
* **Concurrency-ready design**

---

# 2. Design Principles (Critical)

1. **IO-bound optimization over CPU tricks**
2. **Minimize syscalls**
3. **Avoid unnecessary memory allocations**
4. **Use cache-friendly data structures**
5. **Design for extension without modification**

---

# 3. Architecture at LLD Level

```text
[Producers]
   ├── Scanner (DFS)
   └── Watcher (inotify)

        ↓

   [Event Queue]  ← (DSA Core)

        ↓

   [Processing Pipeline]
   ├── Classifier
   ├── Resolver
   ├── Mover

        ↓

   [Observers / Logger]
```

---

# 4. Core Data Structures (DSA Focus)

---

## 4.1 File System Traversal → Graph (DFS)

### Choice:

* **Iterative DFS (stack-based)**

### Why:

* Avoid recursion stack overflow
* Better control over memory

```c
typedef struct {
    char* paths[MAX_STACK];
    int top;
} PathStack;
```

---

## 4.2 Event Queue → Circular Buffer (CRITICAL)

### Problem:

High-frequency file events (scanner + watcher)

### Solution:

**Lock-free circular queue (single producer, single consumer initially)**

```c
#define QUEUE_SIZE 1024

typedef struct {
    FileEvent buffer[QUEUE_SIZE];
    int head;
    int tail;
} EventQueue;
```

### Properties:

* O(1) enqueue/dequeue
* Cache-friendly
* No malloc

---

## 4.3 Config Lookup → Hash Table

### Problem:

Extension lookup must be fast

### Solution:

Open-addressing hash map

```c
typedef struct {
    char* key;      // ".jpg"
    char* value;    // "Jpeg/"
} HashEntry;

typedef struct {
    HashEntry* entries;
    int size;
} HashMap;
```

### Complexity:

* Average: O(1)
* Worst: O(n) (rare with good hashing)

---

## 4.4 Rule Chain → Linked List

```c
typedef struct Rule {
    int (*match)(const char*);
    const char* target;
    struct Rule* next;
} Rule;
```

### Why:

* Dynamic extensibility
* Ordered evaluation

---

## 4.5 String Handling → Fixed Buffers

### Policy:

* Avoid dynamic allocation
* Use stack buffers

```c
char path[PATH_MAX];
char filename[256];
```

---

# 5. Core Patterns (Refined with DSA Awareness)

---

## 5.1 Strategy Pattern → Classification

* Uses hash map internally (DSA)
* Constant-time lookup

---

## 5.2 Factory Pattern → Strategy Creation

* Builds hash map once
* Avoids runtime branching

---

## 5.3 Singleton → Config

* Single hash map instance
* Read-only after initialization

---

## 5.4 Producer–Consumer Pattern (NEW CORE)

### Producers:

* Scanner (batch)
* Watcher (event)

### Consumer:

* Processing pipeline

---

## 5.5 Chain of Responsibility → Rule Matching

* Lightweight linked list traversal

---

## 5.6 Command Pattern → File Operations

* Encapsulates move/copy logic

---

## 5.7 Observer Pattern → Logging

* Non-blocking logging pipeline

---

# 6. Event Model (New Core Abstraction)

```c
typedef enum {
    FILE_CREATED,
    FILE_MODIFIED
} EventType;

typedef struct {
    char path[PATH_MAX];
    EventType type;
} FileEvent;
```

---

# 7. Scanner (DFS Optimized)

---

## Algorithm:

* Iterative DFS using stack
* Avoid repeated syscalls

```c
void scan_directory(const char* root) {
    push(root);

    while (!empty()) {
        char* path = pop();

        // readdir loop
        // enqueue FileEvent
    }
}
```

---

## Optimization:

* Reuse buffers
* Avoid stat() unless needed

---

# 8. Watcher (inotify Integration)

---

## Flow:

```text
inotify → read() → parse → enqueue
```

---

## Key Handling:

* Coalesce duplicate events
* Delay processing (debounce)

---

# 9. Classification (O(1) Lookup)

---

## Flow:

1. Extract extension
2. Hash lookup

```c
const char* folder = hashmap_get(ext);
```

---

## Optimization:

* Avoid repeated parsing
* Pre-normalize extensions

---

# 10. Collision Handling (Atomic + Efficient)

---

## Strategy:

* Atomic file creation (O_EXCL)
* Incremental probing

---

## Optimization:

* Avoid string realloc
* Precompute base length

---

# 11. File Movement (IO-Critical Path)

---

## Step 1: Try rename()

* Fast path (no data copy)

---

## Step 2: Handle EXDEV

* Buffered copy (8KB blocks)
* fsync()
* atomic rename

---

## Optimization:

* Use large buffers (reduce syscalls)
* Avoid small writes

---

# 12. Rollback Strategy (Crash-Safe)

---

## Temp File Pattern

```text
file.tmp → fsync → rename → final
```

---

## Guarantees:

* No partial file exposure
* Safe recovery

---

# 13. Concurrency Model (Future-Ready)

---

## Upgrade Path:

```text
[Producers] → [Queue] → [Worker Threads]
```

---

## DSA Considerations:

* Lock-free queue (future)
* False sharing avoidance
* Cache alignment

---

# 14. Memory Management

---

## Rules:

* No heap allocation in hot path
* Preallocate buffers
* Reuse structures

---

## Why:

* Predictable performance
* No fragmentation

---

# 15. IO Optimization (MOST IMPORTANT)

---

## Key Insights:

* Disk IO dominates cost
* Reduce syscalls
* Batch operations where possible

---

## Techniques:

* Buffered reads/writes
* Avoid unnecessary stat()
* Prefer rename over copy

---

# 16. Failure Handling

---

## Strategy:

* Fail fast
* Cleanup aggressively
* Log everything

---

# 17. Symlink Handling

---

## Policy:

* Skip (default)

---

## Reason:

* Prevent cycles
* Avoid graph complexity

---

# 18. Performance Characteristics

---

| Component | Complexity   |
| --------- | ------------ |
| Traversal | O(N)         |
| Lookup    | O(1)         |
| Queue ops | O(1)         |
| Move      | O(file size) |

---

# 19. Bottleneck Analysis

---

## Real Bottleneck:

* Disk IO

## Not Bottleneck:

* CPU
* Algorithm complexity (mostly)

---

# 20. Future Optimizations

---

* Lock-free queue
* SIMD string parsing
* Zero-copy (sendfile)
* Parallel workers

---

# 21. Final Execution Flow

```text
Scanner/Watcher
    ↓
Event Queue (circular buffer)
    ↓
Classifier (hash map)
    ↓
Resolver (chain)
    ↓
Mover (rename/copy)
    ↓
Rollback/cleanup
    ↓
Observer (logging)
```

---

# 22. Summary

This design achieves:

* **DSA-driven efficiency**
* **Pattern-based extensibility**
* **IO-optimized execution**
* **Concurrency readiness**
* **Crash safety**

---

## Final Insight

> This system is not algorithm-heavy—it is **system-performance-heavy**.
> The real engineering lies in **minimizing IO cost, structuring data flow, and ensuring correctness under failure**.

---

**End of Document**
