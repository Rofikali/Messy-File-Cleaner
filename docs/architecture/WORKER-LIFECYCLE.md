# Worker lifecycle and queue wakeups

## Topology

Each worker owns exactly one SPSC queue and one eventfd. Batch mode has the main scanner as the producer; watch mode has the watcher thread as the producer. A worker is the sole consumer for its queue. Do not add a second producer or consumer without replacing the queue algorithm and its tests.

## Startup contract

- Worker count must be between 1 and MAX_WORKERS.
- Each eventfd is created with CLOEXEC and NONBLOCK.
- Every pthread_create and eventfd creation result is checked.
- If startup fails part-way through, the application stops, wakes and joins every worker already started, closes its eventfds, and returns an error.
- Workers do not allocate a per-thread batch buffer; one allocation failure can no longer silently remove a consumer and deadlock a full queue.

## Work and wakeup contract

- Producers enqueue first, then notify the corresponding worker.
- EINTR is retried. EAGAIN from eventfd write means a wake counter is already pending and is treated as a successful notification.
- Workers drain their queue before waiting again.
- poll() waits for eventfd readiness with a bounded timeout, avoiding an unbounded blocking read if the descriptor becomes invalid.
- A notification or poll/read failure sets worker_error and stops new intake. Unprocessed source files remain on disk.

## Shutdown contract

- The producer must finish before join_workers is called.
- join_workers sets running=false, wakes all workers, lets each worker drain its queue, joins every thread, and closes every eventfd.
- The main batch loop does not busy-spin on active_workers and queue state.
- Shutdown failures and worker failures affect the process exit status.

## Known limitations

- pthread/eventfd failure injection is not yet available; integration tests cover normal multi-queue wakeup, queue draining, empty-worker shutdown, and invalid worker counts.
- The SPSC queue has an effective capacity of QUEUE_SIZE - 1.
- A process crash can still lose in-memory queue entries. The filesystem journal and source-preservation rules are the recovery mechanism; the queue is not durable.
- Watcher error propagation and full inotify overflow/reconciliation handling remain separate work.
