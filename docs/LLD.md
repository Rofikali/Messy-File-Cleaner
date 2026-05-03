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

## 4.1 File System Traversal → Graph

### Choice:

* **Iterative DFS (stack-based)**
* BFS can be added later behind configuration if directory-depth behavior requires it

### Why:

* Avoid recursion stack overflow
* Better control over memory
* Predictable memory growth under deep directory trees

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

**Bounded circular queue**

Initial implementation should use a mutex-protected queue for correctness. Lock-free SPSC queues are a future optimization and must only be used when the producer/consumer model is explicitly SPSC.

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

## Error Contract:

Every module must return explicit status codes. No silent failure is allowed.

```c
typedef enum {
    FC_OK = 0,
    FC_ERR_INVALID_PATH,
    FC_ERR_PERMISSION,
    FC_ERR_QUEUE_FULL,
    FC_ERR_IO,
    FC_ERR_COLLISION_LIMIT,
    FC_ERR_UNSUPPORTED
} FcStatus;
```

### Rules:

* Preserve `errno` for filesystem failures
* Log source path, destination path, operation, and error
* Treat missing source file as a stale event, not a fatal system failure
* Treat destination collision as expected behavior
* Treat partial copy as a hard failure and cleanup temp files

---

# 17. File System Correctness Contract

---

This is the most important correctness section because the system moves user data.

## Guarantees:

* Source file is deleted only after destination is fully created
* Partial destination files are never exposed as final files
* Cross-filesystem moves use copy + fsync + rename
* Temporary files are cleaned after failure
* Destination collision resolution is atomic
* Crash recovery can identify incomplete operations

## Move Algorithm:

```text
1. Validate source path
2. Validate destination directory
3. Create destination placeholder with O_CREAT | O_EXCL
4. Try rename(src, final)
5. If EXDEV:
   a. copy src -> final.tmp
   b. fsync final.tmp
   c. rename final.tmp -> final
   d. fsync destination directory
   e. unlink source
6. Write operation result to journal
```

## Required `errno` Handling:

| Error | Behavior |
| ----- | -------- |
| `EXDEV` | Use copy + atomic rename |
| `EEXIST` | Generate new destination name |
| `ENOENT` | Stale event; skip safely |
| `EACCES` / `EPERM` | Log and skip |
| `ENOSPC` | Stop mover, preserve source |
| `EINTR` | Retry syscall when safe |
| `ENAMETOOLONG` | Log and skip |

## Startup Recovery:

At startup, scan for leftover temp files and journal entries marked `STARTED`.

Policy:

* If source still exists, delete incomplete temp destination
* If final destination exists and source exists, keep both and log conflict
* If final destination exists and source is gone, mark operation complete

---

# 18. Queue Contract

---

The queue is a correctness boundary, not only a DSA component.

## Initial Contract:

* Stage 1/2: mutex-protected bounded circular queue
* Future Stage 3/4: lock-free SPSC or sharded SPSC queues
* MPSC requires a different design; do not use SPSC atomics with multiple producers

## Required Semantics:

```c
int enqueue(EventQueue* q, const FileEvent* e);
int dequeue(EventQueue* q, FileEvent* out);
```

Return values:

* `1` = success
* `0` = queue empty/full
* `-1` = invalid state

## Queue Rules:

* Never overwrite unread events
* Always unlock mutex before return
* Define full condition as `next_tail == head`
* On overflow, apply configured backpressure policy
* Shutdown must wake blocked consumers

## Backpressure Policies:

| Policy | Behavior |
| ------ | -------- |
| `block` | Producer waits for capacity |
| `drop_duplicate` | Drop event if same path already pending |
| `rescan` | Mark root dirty and schedule full scan |
| `fail_fast` | Stop processing and return error |

Default policy should be `block` for scanner and `rescan` for watcher bursts.

---

# 19. Idempotency and Duplicate Events

---

Filesystem events are at-least-once, unordered, and sometimes stale.

## Idempotency Rules:

* Moving an already moved file must not crash the pipeline
* Missing source means event is stale
* Destination folders may already exist
* Repeated collision checks must remain atomic
* Duplicate events for the same path should be coalesced where possible

## Event Coalescing:

Use a small fixed-size hash/set or ring cache for recent paths.

```text
key = normalized_absolute_path
value = last_event_time + event_type
```

If the same path appears within debounce window, merge events.

---

# 20. Safety Policy

---

The cleaner must protect users from destructive mistakes.

## Default Restrictions:

* Refuse to run on `/`
* Refuse system directories like `/bin`, `/usr`, `/etc`, `/var`
* Refuse home directory root unless `--force`
* Skip symlinks by default
* Skip device files, sockets, FIFOs, and special files
* Do not follow hardlinks unless explicitly configured
* Do not move files that are still being written

## Write-Stability Check:

Before moving a file from watcher mode:

```text
stat file
wait debounce interval
stat file again
move only if size and mtime are stable
```

---

# 21. Recovery Journal / Move Ledger

---

Every mutating operation should be recorded.

## Journal Record:

```c
typedef enum {
    OP_STARTED,
    OP_COMPLETED,
    OP_FAILED
} OperationState;

typedef struct {
    char source[PATH_MAX];
    char destination[PATH_MAX];
    EventType event_type;
    OperationState state;
    int error_code;
    long timestamp_ns;
} MoveJournalRecord;
```

## Journal Uses:

* Crash recovery
* Undo support
* Debugging user reports
* Auditing data movement

Write the journal before mutating the filesystem and update it after completion.

---

# 22. Symlink Handling

---

## Policy:

* Skip (default)

---

## Reason:

* Prevent cycles
* Avoid graph complexity

---

# 23. Performance Characteristics

---

| Component | Complexity   |
| --------- | ------------ |
| Traversal | O(N)         |
| Lookup    | O(1)         |
| Queue ops | O(1)         |
| Move      | O(file size) |

---

# 24. Bottleneck Analysis

---

## Real Bottleneck:

* Disk IO

## Not Bottleneck:

* CPU
* Algorithm complexity (mostly)

---

# 25. Performance Budget

---

Performance claims should be measured, not guessed.

## Initial Targets:

| Metric | Target |
| ------ | ------ |
| Memory usage | O(queue size + config size) |
| Queue operation | O(1) |
| Classification | Average O(1) |
| Same-filesystem move | One `rename()` fast path |
| Cross-filesystem copy | Buffered sequential IO |
| Watcher latency | Configurable debounce window |

## Metrics to Capture:

* files processed per second
* bytes copied per second
* queue depth
* queue overflow count
* move failures by `errno`
* duplicate event count
* average processing latency

---

# 26. Future Optimizations

---

* Lock-free queue
* SIMD string parsing
* Zero-copy (sendfile)
* Parallel workers

---

# 27. Final Execution Flow

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

# 28. Summary

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
