✅ Cleaner Architecture (Engineering-Grade Version)
1. Overview

Cleaner is a layered, testable, platform-isolated file organization tool.

The architecture enforces strict dependency direction and isolation of platform-specific behavior.

Dependency flow:

CLI
 ↓
Core
 ↓
System (Infrastructure)
 ↓
Platform

Higher layers never depend on lower implementation details.

Platform-specific code is fully isolated.

2. Architectural Principles
1. UTF-8 Core

All internal string handling is UTF-8.

Wide-character conversion (wchar_t) exists only inside the platform layer.

2. Dependency Direction

Dependencies always point downward.
Lower layers must never depend on higher ones.

3. Separation of Concerns

Business logic is isolated from OS details.

Concurrency is isolated from processing logic.

I/O is isolated from organization logic.

4. Testability

Core logic must be testable without OS interaction.

3. Layer Definitions
CLI Layer (src/cli/)
Responsibilities

Argument parsing

Configuration construction

Input validation

User interaction

Triggering application workflow

Must NOT

Implement file organization logic

Call OS APIs directly

Perform renaming logic

Contain business rules

CLI is a thin orchestration layer only.

Core Layer (src/core/)

This is the domain layer.

Responsibilities

File scanning orchestration

Organization strategy

Rename rules

Processing pipeline

Exclusion logic

Folder naming logic

Properties

Uses UTF-8 exclusively

Contains no Windows headers

No windows.h

No HANDLE

No SYSTEMTIME

No WinAPI calls

No console printing

May Depend On

System abstractions (interfaces only)

Core defines what should happen — not how the OS does it.

System Layer (src/system/)

This is infrastructure coordination.

Responsibilities

Thread pool orchestration

Metrics collection

Logging interface

Coordination primitives

Properties

May depend on platform abstractions

Must not depend on CLI

Must not embed business logic

System layer coordinates execution but does not define organization rules.

Platform Layer (src/platform/)

Lowest layer.

Responsibilities

Filesystem operations

File time retrieval

Directory creation

File move operations

Atomic operations

Thread primitives

High resolution timing

UTF-8 ↔ UTF-16 conversion (Windows only)

Properties

Only layer allowed to include <windows.h>

All OS-specific APIs live here

Provides C-level abstraction functions

Replaceable for POSIX implementation

Platform layer is the only place touching the OS.

4. Library Design

cleaner_core is a static library.

It exposes a stable public API.

Public modules:

scanner

processor

organizer

renamer

threadpool (system-level abstraction)

metrics interface

CLI links against this library.

Platform implementation is compiled into the core library but hidden behind abstraction boundaries.

5. Data Flow Model

High-level execution:

CLI builds config
 → Core scanner walks filesystem (via platform API)
 → Threadpool schedules jobs
 → Processor applies organization rules
 → Platform performs file operations
 → Metrics collect results

Business logic never directly calls WinAPI.

6. Encoding Model

All public APIs accept and return UTF-8.

Core operates purely on UTF-8 strings.

Windows implementation converts to UTF-16 internally.

No wide strings leak upward.

This prevents encoding bugs and ensures portability.

7. Concurrency Model

Threadpool is infrastructure.

Core submits work units.

Worker threads execute processing logic.

Synchronization primitives are abstracted via platform layer.

Threadpool must not depend on Windows directly once refactor is complete.

8. Testing Strategy
Unit Tests

Scanner behavior

Rename rules

Exclusion logic

Folder naming rules

Processor dry-run behavior

Tests link against cleaner_core.

Principles

Do not test CLI parsing.

Do not test OS APIs directly.

Mock or isolate platform layer where needed.

Focus on behavior, not implementation.

CTest orchestrates execution.

9. Build System

CMake provides:

Static library separation

Platform-specific source selection

Testing integration

Warning configuration

Future install/packaging support

Build targets:

cleaner_core (static lib)

cleaner (CLI executable)

test_* (unit tests)

10. Long-Term Direction

Future improvements:

Fully remove <windows.h> from core and system

Replace direct WinAPI calls with platform abstractions

Eliminate global state

Introduce interface headers for platform contracts

Add install() rules

Add packaging

Add CI with static analysis

🔥 What Changed vs Your Original

You originally wrote:

“Windows-native file organization tool”

Now it becomes:

Platform-isolated architecture with Windows implementation.

That’s a huge maturity shift.

~~~ system/
        threadpool.h        ← owns Job + ThreadPool
        threadpool.c        ← implements logic

~~~ platform/
        threads.h           ← defines cleaner_thread abstraction ONLY
        threads_win.c       ← Windows implementation