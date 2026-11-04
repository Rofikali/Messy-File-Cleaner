# Deterministic worker-start failure testing

The worker lifecycle test builds `src/core/thread_pool.c` with `FC_TESTING` enabled. The production executable does not define this macro and contains no active fault-injection controls.

The test hooks can fail:
- `eventfd()` after a chosen number of successful calls, to verify descriptor-creation failure leaves no started workers or open worker descriptors.
- `pthread_create()` after a chosen number of successful calls, to verify partial startup wakes and joins already-created workers, closes descriptors, resets the worker count, and returns the original error.

Each injected failure is one-shot. The test then starts a normal worker pool to prove the lifecycle can recover after a failed startup.

These tests validate deterministic failure paths; they do not simulate kernel-level resource exhaustion in every environment. Future coverage should add queue-notification and shutdown-during-active-move fault injection without changing production behavior.
