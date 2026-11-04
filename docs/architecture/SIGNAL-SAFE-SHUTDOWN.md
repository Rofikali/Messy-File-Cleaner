# Signal-safe shutdown contract

## Signal handler

The SIGINT/SIGTERM handler performs one operation only: assign the signal number to `AppContext.stop_signal`, a a lock-free `atomic_int` stop flag (the build requires `ATOMIC_INT_LOCK_FREE == 2`). It does not call stdio, allocation, pthread APIs, or non-lock-free atomic operations.

## Normal-context shutdown

- Batch scanning checks the stop flag during traversal and from its enqueue callback.
- A cancelled scan returns `ECANCELED`, closes open directory streams, and exits with the conventional `128 + signal` status.
- Watch mode checks the same flag between bounded `poll()` waits and while applying event batches.
- Workers are stopped by the normal main-thread shutdown path, which sets the atomic running flag, wakes workers, drains queued work, joins them, and closes eventfds.
- Signal metrics are updated in normal execution context, not in the handler.

## Guarantees and limits

The handler uses only lock-free atomic stores, which are signal-safe under the C atomic rules. The compile-time assertion prevents building on a target where `atomic_int` is not always lock-free. Shutdown is cooperative: a worker already inside a filesystem operation finishes that operation before it drains and exits. A signal does not roll back a move that has already been durably published; the journal recovery protocol governs interrupted moves.
