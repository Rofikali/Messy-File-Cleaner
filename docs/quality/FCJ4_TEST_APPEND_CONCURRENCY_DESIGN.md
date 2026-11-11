# FCJ4 Test Append Adapter — Cross-Thread Concurrency Design

## Status

**Proposed design only.** This document authorizes no implementation by itself. It must be reviewed and merged before a separate test-only implementation PR begins.

The FCJ4 append adapter remains a Linux-only test harness under `tests/support`, used only with caller-supplied descriptors for disposable private temporary files. It is not a production journal facility. PR #76 merged the per-context allocation-failure tests; deliberately concurrent calls from separate threads remain an open acceptance gap in the broader adapter fault harness.

## 1. Objective

Prove, with deterministic pthread tests, that the adapter's atomic try-guard rejects overlapping operations on one live context without allowing two threads to mutate its state or use its descriptor concurrently.

The tests must cover the append, acknowledgement, poison, and destroy entry points, with particular attention to:

- a second append while the first append is paused inside an injected I/O callback;
- destroy while an append is active;
- acknowledgement or poison while another operation owns the context;
- independent contexts used by independent threads;
- correct caller-managed context lifetime and thread shutdown.

The purpose is to verify the existing single-owner/try-guard contract under real cross-thread scheduling. This is not a proposal to make the adapter a general-purpose concurrent journal writer.

## 2. Scope and non-goals

### In scope

- A Linux-only integration-test extension to `tests/integration/test_fcj4_test_append.c`.
- Pthread-based deterministic coordination using mutexes and condition variables (or barriers where they simplify a specific handshake).
- Explicit linking of the Linux-only test executable with the existing CMake `Threads::Threads` target.
- Test-only synchronization callbacks and state, isolated per test/context.
- A separate ThreadSanitizer (TSan) validation configuration or CI job, kept separate from ASan/UBSan.
- Narrow documentation updates to the adapter contract, threat model, and CI/test instructions.

### Explicit non-goals

- No production source-list, production executable, CLI, mover, startup, journal path, or runtime behavior changes.
- No FCJ2/legacy or FCJ3 changes.
- No production journal writer, multi-writer support, lock redesign, planner, executor, recovery, or filesystem mutation.
- No global mutex, global fault state, global allocator, or process-wide test hook.
- No promise that arbitrary calls racing with or following successful destruction are safe.
- No changes to the adapter's existing append/durability semantics.
- No claim that TSan proves absence of all concurrency bugs or that a clean test run covers every possible schedule.

## 3. Existing contract and concurrency boundary

The current context contains an `atomic_bool busy` try-guard. Public operations attempt to acquire it with an atomic exchange. If another operation already owns the guard:

- `fcj4_test_append_record()` returns `-1` with `errno == EBUSY`;
- `fcj4_test_append_ack_commit()` returns `-1` with `errno == EBUSY`;
- `fcj4_test_append_destroy()` returns `-1` with `errno == EBUSY` and must not unlock, close, or free the context;
- `fcj4_test_append_poison()` returns without changing context state because its current API is `void`.

A successful guard acquisition grants exclusive access to the non-atomic context fields and descriptor for that operation. The guard must be released on every return path after acquisition. A failed try-acquire must not read or write context fields other than the atomic guard itself.

This guard is a fail-fast exclusion mechanism, not a blocking mutex and not a queue. The API does not promise fairness or eventual success for a rejected caller.

### 3.1 Context lifetime is externally owned

An atomic field inside an allocated context cannot make dereferencing a freed context safe. Therefore:

- The owner must prevent new calls from starting once destruction begins.
- The owner must stop and join every thread that may hold or attempt to use the context before freeing it.
- A call that is already active may overlap a destroy attempt while the context is still live; destroy must observe the busy guard and return `EBUSY` without cleanup.
- After that active call exits, the owner may retry destroy only after it has established that no other thread can start or still be using the context.
- No test may call any API through a context pointer after successful destroy.
- No test may treat `EBUSY` from destroy as permission to discard the pointer or assume cleanup occurred.

This is a caller-side lifetime rule. The implementation must not claim that the atomic guard solves use-after-free caused by a caller racing a new API entry against completed destruction.

## 4. Deterministic synchronization harness

Use per-test synchronization objects initialized before worker creation and destroyed only after all participating threads have joined.

Preferred mechanism:

- `pthread_mutex_t` protects the test gate's flags and counters.
- `pthread_cond_t` signals that a callback has been entered and allows the main test thread to release it.
- Worker callbacks signal “entered” and wait in a predicate loop until the main thread sets “release”.
- The main thread waits in a predicate loop for the expected callback-entry condition before issuing the competing API call.
- Every pthread return code is checked and reported. Do not assume pthread APIs set `errno`; use their returned error code.
- Every created thread is joined on every test path before its stack data, synchronization objects, mock state, descriptors, or context are destroyed.
- Cleanup paths must release a blocked callback before joining, even when an earlier assertion fails; use a test helper that records failure and performs safe teardown rather than exiting while a worker is blocked.

Do not use sleeps, polling loops with wall-clock timing, scheduler assumptions, or “run this many times and hope” as the synchronization mechanism. A CTest timeout is a final hang guard, not the synchronization strategy.

The callback gate should be narrowly injected through the existing per-context I/O operations table (for example, a test `pwrite_fn` wrapper). It must not add a production hook or a global pause flag.

## 5. Required concurrency scenarios

### 5.1 Append versus append

1. Initialize one valid adapter context over a disposable private file.
2. Start worker A, which calls append.
3. Its injected `pwrite_fn` signals that it has entered and blocks on the condition variable while worker A still owns the context guard.
4. After observing the callback-entry signal, the main thread calls append on the same live context.
5. Assert the second call returns `EBUSY`; assert it did not invoke `pwrite_fn`, `fsync_fn`, or mutate the expected file length.
6. Release the callback, join worker A, and assert the first append result matches the normal durable path.
7. Acknowledge the exact committed bytes, then destroy only after all users have stopped.

### 5.2 Append versus destroy

1. Pause worker A inside injected `pwrite_fn` while it owns the guard.
2. Call destroy from the main thread while the context is still live and worker A remains blocked.
3. Assert destroy returns `EBUSY`; no unlock, close, allocator free, or context-state cleanup occurs.
4. Release and join worker A.
5. Complete the appropriate acknowledgement or reconciliation path.
6. After preventing all future calls and joining all users, destroy once; assert exactly one unlock, one close attempt, and one matching allocator free.

This scenario proves that destroy does not free a context while another operation owns the guard. It does not prove safety if the owner allows a new API call to begin after destruction has started.

### 5.3 Append versus acknowledgement

1. Pause append inside an injected callback while the guard is held.
2. Attempt `fcj4_test_append_ack_commit()` from another thread or the coordinating test thread.
3. Assert `EBUSY`, no state advancement, and no clearing of pending bytes.
4. Release append and join it.
5. Verify the normal durable-pending state still requires the exact acknowledgement before a subsequent append.

### 5.4 Append versus poison

1. Pause append while the guard is held.
2. Call `fcj4_test_append_poison()` from another thread.
3. Since the current function returns `void`, do not invent an error result. Assert through synchronized observations that the call does not alter the active operation's state while the guard is held.
4. Release and join append. Then, from a quiescent owner context, invoke poison and verify that subsequent append is rejected according to the existing poisoned-state contract.
5. Document the limitation that the void API cannot tell the caller whether poison was applied or skipped due to contention. Do not silently change this API in this milestone.

### 5.5 Independent contexts

- Run operations concurrently on two separately initialized contexts and two separate disposable files.
- Use distinct per-context mock state and callback gates.
- Assert each context's bytes, counters, result, allocator, and cleanup remain independent.
- No shared mutable mock counters may be accessed without synchronization.

### 5.6 Reentrant callback path

Retain the existing same-thread reentrant append test. Cross-thread tests supplement it; they do not replace it.

## 6. Test-state and memory-model requirements

- All fields in a test gate that are accessed by multiple threads must be protected by its mutex or be appropriate C atomics with an explicit reason.
- Do not use volatile as a synchronization primitive.
- Do not read mock counters from the main thread while a worker may update them unless the read is protected by the same mutex or occurs after a successful join.
- Do not let a callback access stack or heap state after its owner has begun cleanup.
- Keep callback function tables and callback user data immutable for the context lifetime, consistent with the existing adapter contract.
- The adapter's non-atomic state and file descriptor may be inspected only while the operation guard is held by the operation, or after all operations have joined and the context is quiescent.
- Tests must verify that failed guard acquisition performs no callback or descriptor operation.
- Preserve exact errno behavior on rejected calls. Do not let test synchronization errors overwrite the errno being asserted; capture results immediately and report pthread errors separately.
- Check all condition-variable/mutex initialization, lock, wait, signal, broadcast, unlock, destroy, thread-create, and thread-join return codes.
- Avoid cancellation of a worker while it owns the adapter guard. These tests use cooperative release and join, not pthread cancellation.

## 7. Build and sanitizer strategy

- Keep the test target Linux-only, as it is today.
- Link `test_fcj4_test_append` explicitly with `Threads::Threads`; do not add a thread dependency to the production executable as part of this milestone.
- Keep the ordinary C11/C17/C23 Debug/Release and ASan/UBSan workflows.
- Add a separate TSan build/test job or documented configuration for the focused FCJ4 append integration target. Do not combine `-fsanitize=thread` with `-fsanitize=address` in one executable; use separate build directories/configurations.
- Prefer a focused TSan run of `fcj4_test_append_integration` first. If supported by the CI compiler/runtime, run it in a dedicated Ubuntu Linux job. Record unsupported runtime/environment failures as such; do not convert them into a pass.
- Keep the existing 30-second CTest timeout or choose a justified slightly larger bound if sanitizer overhead requires it. The synchronization itself must remain condition-based and deterministic.
- Run the existing Makefile and Docker/non-root workflows unchanged to prove no production dependency or behavior was introduced.

TSan is dynamic evidence for the executed schedules and code paths. It does not replace explicit assertions, code review, ASan/UBSan, or the caller lifetime contract.

## 8. Acceptance criteria

The implementation PR must demonstrate:

1. All scenarios in §5 pass deterministically without sleeps or timing assumptions.
2. The second append while the first is paused returns `EBUSY` and does not touch I/O callbacks or file bytes.
3. Destroy while an append owns the guard returns `EBUSY` and does not unlock, close, or free.
4. Acknowledgement while append owns the guard returns `EBUSY` and leaves the durable-pending transition unchanged.
5. Poison contention behavior is documented and tested without claiming an observable return value that the API does not provide.
6. Independent contexts run concurrently without cross-context state or allocator interference.
7. All threads are joined before context destruction, mock-state cleanup, or fixture removal.
8. All pthread API errors are checked; no worker can remain blocked on a failing test path.
9. No data race is reported by the dedicated TSan run on the supported CI environment.
10. Existing C11/C17/C23 Debug/Release, ASan/UBSan, Makefile, Docker, and non-root checks remain green.
11. The production source list, executable, CLI, journal paths, FCJ2/legacy, and FCJ3 behavior are unchanged.
12. Documentation distinguishes proven test behavior from unsupported arbitrary concurrent destruction and from process-kill versus power-loss guarantees.

## 9. Threat and risk analysis

Assets at risk are the test context, descriptor ownership, journal bytes, in-memory writer revision, and the reliability of the fault harness itself.

Relevant failures include data races in test callbacks/counters, concurrent state transitions, descriptor close during I/O, freeing the context while an operation is active, stale pointer use after destroy, deadlocked test threads, and false-positive tests that rely on scheduling luck.

Controls are per-context callback gates, the existing atomic try-guard, externally enforced object lifetime, checked pthread synchronization, condition-based handshakes, join-before-cleanup, TSan in a separate build, disposable fixtures, and strict production-target isolation.

Residual limitations:
- A successful test run covers only the schedules exercised.
- TSan can miss unexecuted races and can have runtime/environment limitations.
- The adapter is not made safe for callers that start new operations concurrently with or after completed destruction.
- `flock` remains advisory and does not constrain non-cooperating writers.
- This milestone does not prove power-loss durability or authorize production use.

Update the FCJ4 section of `docs/security/THREAT_MODEL.md` in the implementation PR to state that the harness's try-guard is fail-fast exclusion only and that context lifetime is externally synchronized. Do not describe the adapter as a production concurrency boundary.

## 10. Rollback

Rollback removes only the new concurrency tests, their test-only synchronization helpers/build flags, and the concurrency-specific documentation. It must not alter journal bytes, production source lists, production behavior, user files, or FCJ2/legacy/FCJ3 behavior.
