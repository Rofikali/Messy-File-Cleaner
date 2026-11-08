# Engineering Playbook: HLD, LLD, Architecture, and Design Principles

## 1. Purpose and decision hierarchy

This playbook describes how to design and evolve Messy File Cleaner. It complements the mandatory rules in `AGENTS.md` and the practical workflow in `skills.md`.

Decision order:
1. Preserve user data and security invariants.
2. Maintain correctness, recoverability, and compatibility.
3. Keep module boundaries and contracts understandable.
4. Measure performance and resource use.
5. Optimize delivery speed and convenience.

The product is a Linux/POSIX **modular monolith written in pure C**. It is not a distributed system. C17 is the production baseline; C11 and C23 are independent validation targets.

## 2. HLD — high-level design

### 2.1 System responsibilities

The application:
1. Accepts and validates CLI/configuration input.
2. Establishes trusted root/output/journal directory handles.
3. Scans eligible filesystem entries.
4. Classifies files according to explicit rules.
5. Places bounded work items into a queue.
6. Uses workers to execute move transactions.
7. Records durable transaction progress in a journal.
8. Recovers interrupted transactions conservatively.
9. Reports outcomes and shuts down cleanly.

### 2.2 Logical components

| Component | Owns | Must not own |
|---|---|---|
| CLI/application | Process lifecycle, option parsing, orchestration | Filesystem mutation policy hidden in argument parsing |
| Configuration | Validated configuration values and rules | Arbitrary filesystem traversal |
| Scanner | Discovery and scan cancellation | Destination publication |
| Classifier | Deterministic file-to-category policy | Syscalls and journal writes |
| Queue/worker pool | Bounded dispatch, worker lifecycle, ownership transfer | Domain path policy |
| Mover | Copy, verification, publication, source retirement | Parsing CLI configuration |
| Journal | Durable record format, append, replay, transaction state | Guessing filesystem ownership |
| Recovery | Reconcile journal evidence with filesystem facts | Broad prefix-based cleanup |
| Watcher | Change notifications and reconciliation triggers | Treating notifications as authoritative truth |
| Platform adapters | Narrow OS-specific operations | Domain workflow policy |

The exact implementation may combine or split source files when cohesion and ownership justify it. Source-file count is not the architecture.

### 2.3 Main data flow

`CLI/config -> validated context -> scanner -> classifier -> bounded queue -> workers -> mover <-> journal -> result reporting`

The watcher is an optional source of scan/reconciliation triggers. Watcher notifications can be dropped or coalesced; reconciliation must be able to re-establish filesystem truth.

### 2.4 Runtime and deployment boundaries

- The development environment is Ubuntu 24.04 LTS through the repository Dev Container.
- The production container is a separate minimal runtime stage.
- Run as non-root, drop unnecessary capabilities, mount only required directories, and keep the root filesystem read-only where practical.
- Do not mount the Docker socket or host root.
- Filesystem behavior depends on kernel, mount, and filesystem implementation. State the supported environment and test the actual destination mount for optional capabilities.

## 3. LLD — low-level design requirements

Every module header and design note should define:
- Purpose and non-responsibilities.
- Public types and functions.
- Preconditions and postconditions.
- Ownership and lifetime for pointers, descriptors, and buffers.
- Error model and errno behavior.
- Thread-safety and reentrancy.
- Cancellation and shutdown behavior.
- Resource limits and complexity expectations.
- Platform assumptions and fallback policy.
- Test seams and fault-injection coverage.

### 3.1 C module shape

Prefer:
- One cohesive public header per module.
- Private implementation details in `.c`.
- Opaque context structs when state must be encapsulated.
- Explicit initialization/destruction pairs.
- Explicit status values and output parameters.
- Dependency injection through narrow function tables or context pointers only where it improves isolation.

Avoid:
- Global mutable state.
- Exposing internal structs unnecessarily.
- Header-level implementation duplication.
- Cyclic module dependencies.
- Generic frameworks with no second real use case.
- Hidden ownership or implicit cleanup.

### 3.2 Error handling

- Return explicit status; preserve the originating errno or domain error.
- Never convert an error into success because cleanup succeeded.
- Cleanup errors should be reported without erasing the primary failure.
- Define retryability: transient, permanent, or uncertain.
- Do not retry non-idempotent operations without a protocol that makes retries safe.
- Use bounded formatting and parsing and detect truncation.

## 4. Architecture styles: when and how

### 4.1 Modular monolith — selected

One deployable application with strong internal module boundaries is the right default. It avoids network failure modes, service discovery, deployment coordination, distributed tracing, and operational overhead that do not currently create customer value.

Split into services only when independent deployment/scaling, security isolation, or organizational ownership demonstrably outweighs distributed-systems cost.

### 4.2 DDD — selective domain modeling

Use DDD to make the problem vocabulary precise:
- File candidate
- Classification rule
- Move operation
- Journal transaction
- Published destination
- Source retirement
- Recovery decision

Keep domain rules separate from low-level syscall details where practical. Do not invent repositories, aggregates, factories, or bounded contexts merely to imitate enterprise diagrams. The local journal and filesystem are persistence mechanisms, not automatically a complex domain model.

### 4.3 Hexagonal / ports and adapters — selective

Use ports where the application needs to depend on a capability rather than a concrete OS implementation:
- Filesystem operations.
- Journal persistence.
- Clock/time source.
- Random operation-ID generation.
- Logging/diagnostics.
- Test fault injection.

A port should express a stable application need. An adapter implements it. Do not wrap every libc call in a virtual interface; C function tables or context-based callbacks are sufficient when a real seam is needed.

### 4.4 Pipeline and producer-consumer

The scan/classify/queue/worker pipeline is appropriate because work items can be processed independently within explicit limits. Backpressure is a correctness and resource-control feature, not merely a performance detail.

Queue contracts must state producer/consumer counts, ordering guarantees, capacity, ownership transfer, cancellation, and shutdown. Preserve SPSC assumptions unless a separately reviewed implementation proves MPSC/MPMC correctness.

### 4.5 State machine and event-driven behavior

The move workflow is a durable state machine. Define:
- Transaction identity.
- Allowed transitions.
- Immutable transaction metadata.
- What is durable before each filesystem mutation.
- Crash points and recovery action.
- Retry/idempotency behavior.
- Terminal and uncertain outcomes.

Worker wakeups are event-like notifications. Journal records are durable transaction evidence. This is not full distributed Event-Driven Architecture: no broker, independent event consumers, or distributed eventual consistency currently exists. Introduce a broker only for a concrete multi-process/distributed use case with explicit delivery, ordering, deduplication, replay, and failure semantics.

### 4.6 Other patterns

Use a pattern only when it reduces real complexity:
- Strategy: interchangeable classification or policy rules with a genuine need.
- Adapter: isolate platform/filesystem differences.
- State: encode legal transaction transitions.
- Producer-consumer: bounded asynchronous processing.
- Builder: only if configuration construction is genuinely complex.
- Observer/pub-sub: only when multiple independent consumers need notifications.
- Singleton: generally avoid; explicit context/lifetime is safer.
- Factory: avoid when a simple constructor is clearer.

## 5. SOLID, DRY, KISS, YAGNI

| Principle | Practical interpretation in this C project | Warning |
|---|---|---|
| Single Responsibility | A module/function has one coherent reason to change | Do not split every line into a function |
| Open/Closed | Extend through stable policy seams when variation is real | Do not predict imaginary future plugins |
| Liskov Substitution | An adapter must honor the same documented contract | Do not hide weaker guarantees behind a common API |
| Interface Segregation | Small headers expose only what callers need | Do not create dozens of tiny meaningless headers |
| Dependency Inversion | High-level workflow depends on narrow capabilities | Do not wrap every syscall |
| DRY | Keep one authoritative rule/schema/constant for shared knowledge | Similar-looking code may have different safety semantics |
| KISS | Choose the simplest design that satisfies correctness and security | Simple code is not code that ignores failure modes |
| YAGNI | Defer features without a demonstrated requirement | Do not defer necessary recovery/security controls |

In C, SOLID is about module contracts and dependency direction, not classes or inheritance.

## 6. Security and data integrity by design

Security requirements are defined in `docs/security/THREAT_MODEL.md`. Every design must consider:
- Untrusted path/configuration input.
- Symlinks, traversal, special files, hard links, and mount boundaries.
- TOCTOU and directory-entry replacement.
- File and directory permissions/ownership.
- Exclusive creation and no-overwrite publication.
- Resource exhaustion and denial of service.
- Integer overflow, truncation, and malformed persistent input.
- Concurrent workers and shutdown races.
- Crash consistency and ambiguous durability.
- Same-UID attacker limitations.

No cleanup/recovery feature may infer ownership from a filename prefix. Identity and directory ownership must be established by durable metadata and checked against live filesystem facts. If the safety boundary cannot be proven, preserve the entry and report an unresolved condition.

## 7. Journal and recovery design rules

- Treat the journal as a persistent protocol with versioned schemas.
- Never change a format without parser/serializer design, compatibility rules, migration strategy, and crash tests.
- Transaction identity must distinguish repeated operations on the same source/destination pair.
- Immutable ownership metadata must remain consistent across transitions.
- Validate state order, duplicate IDs, lengths, numeric fields, path/component constraints, and malformed/truncated tails.
- Legacy/FCJ2 records may be replayed under their existing semantics but cannot authorize FCJ3-only cleanup.
- Never truncate or rewrite ambiguous journal evidence automatically.
- Recovery must be idempotent and conservative; preserve unknown, replaced, mismatched, or special filesystem entries.
- Destructive recovery requires exact identity checks, a safe quarantine boundary, directory durability, and adversarial tests.

## 8. Performance and capacity engineering

Measure before optimizing. Record:
- Workload size and file-size distribution.
- Directory depth and directory count.
- Filesystem, mount options, storage, kernel, and cache state.
- CPU/memory/container limits.
- Compiler/version/flags and C standard.
- Wall time, files/sec, bytes/sec, memory, syscalls, and error rates.
- Repetition count, median, and tail percentiles.

Separate metadata-only rename, same-filesystem move, cross-filesystem copy, durable journal I/O, and cold-cache scanning. Do not claim nanosecond filesystem operations. Nanosecond units may be meaningful for isolated CPU-bound operations only when repeatable measurement supports the claim.

Use bounded queues, workers, memory, file descriptors, and journal parsing. Treat overload behavior as part of the design. Optimize algorithmic work and syscall count before considering assembly.

## 9. Testing and quality strategy

Required layers and security gates are in `skills.md`. High-risk changes require:
- Unit and integration tests.
- Deterministic fault injection.
- Process crash/restart tests at every durable boundary.
- Exact content and identity verification.
- Adversarial path and directory-entry replacement tests.
- C11/C17/C23 Debug and Release validation.
- Sanitizers, static analysis, Docker, and non-root execution.

A passing happy-path test is not proof of crash safety. A successful build is not proof of correctness.

## 10. Documentation and ADR policy

Maintain:
- HLD: component map, data flow, trust boundaries, deployment.
- LLD: module contracts, ownership, concurrency, errors, state transitions.
- ADRs: significant choices and rejected alternatives.
- Threat model: assets, attacker capabilities, controls, residual risks.
- Runbooks: build, dry-run, execution, recovery, troubleshooting.
- README: user-facing purpose, status, requirements, safe usage.

An ADR should include status, context, decision, alternatives, consequences, security impact, and validation plan. Mark designs as proposed until implemented and tested.

## 11. Business and management discipline

Evaluate every feature against:
- Customer/user problem solved.
- Data-loss and security risk.
- Reliability and support burden.
- Complexity and ongoing maintenance cost.
- Resource consumption and deployment cost.
- Measurable acceptance criteria.
- Reversibility and rollback.

Prefer a smaller reliable product over a large feature list. Do not introduce microservices, databases, brokers, GUI, cloud control planes, or speculative optimizations without customer evidence and a clear total-cost-of-ownership case.

## 12. Definition of done

A change is done only when:
1. Requirements and acceptance criteria are explicit.
2. Design and module contracts are updated.
3. Implementation is pure C and follows the supported standards.
4. Security and data-preservation invariants are maintained.
5. Focused tests and risk-appropriate full gates pass.
6. Documentation matches actual behavior.
7. CI results and limitations are recorded.
8. Owner review and repository governance are satisfied.
9. Merge is verified on `main` before dependent work begins.
