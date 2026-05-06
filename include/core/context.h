#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdatomic.h>
#include "core/lf_queue.h"
#include "classifier/classifier.h"
#include "config/config.h"
#include "journal/journal.h"

#define MAX_WORKERS 16

typedef struct
{
    atomic_ulong events_seen;
    atomic_ulong events_enqueued;
    atomic_ulong queue_backpressure_waits;
    atomic_ulong dry_run_moves;
    atomic_ulong files_moved;
    atomic_ulong move_failures;
    atomic_ulong watcher_events;
    atomic_ulong watcher_idle_polls;
    atomic_ulong signals_received;
} ObservabilityMetrics;

typedef struct
{
    LFQueue queues[MAX_WORKERS];
    int num_workers;

    int efd[MAX_WORKERS];

    atomic_int running;
    atomic_int active_workers;
    atomic_int scanning_done;
    atomic_int dry_run;

    Classifier classifier;
    Config cfg;
    MoveJournal journal;
    ObservabilityMetrics metrics;

} AppContext;

#endif
