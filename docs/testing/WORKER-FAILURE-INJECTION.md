# Deterministic worker-start failure testing

The worker lifecycle test builds `src/core/thread_pool.c` with `FC_TESTING` enabled. The production executable does not define this macro and contains no active fault-injection controls.

The test hooks can fail:
- `eventfd()` after a chosen number of successful calls, to verify descriptor-creation failure leaves no started workers or open worker descriptors.
- `pthread_create()` after a chosen number of successful calls, to verify partial startup wakes and joins already-created workers, closes descriptors, resets the worker count, and returns the original error.
- Worker eventfd notification, to verify an `EIO` notification failure is surfaced and a subsequent shutdown can still wake, join, and close the worker cleanly.
- A worker pause immediately after dequeue and active-work accounting, to verify shutdown waits for in-flight processing instead of returning while a worker is still active.

Each injected failure is one-shot. The active-work gate is controlled by the test and is compiled only when `FC_TESTING` is enabled. Production builds contain no active fault-injection controls.

The lifecycle tests validate deterministic failure paths; they do not simulate kernel-level resource exhaustion in every environment or prove crash consistency of a real filesystem move. Future coverage should add shutdown while a real journaled move is blocked in filesystem I/O, queue-notification saturation, and signal-driven shutdown under sustained producer load.
