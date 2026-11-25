#ifndef WATCHER_H
#define WATCHER_H

#include "core/context.h"

/* Returns an index in [0, num_workers), or -1/EINVAL for invalid inputs.
 * The caller keeps num_workers stable and in [1, MAX_WORKERS] during dispatch. */
int dispatch(AppContext *ctx, const char *path);
/* Watches a directory only; trims trailing separators (except the root slash)
 * before no-follow registration. Event paths use the trimmed root.
 * Ancestor symlinks, dot-component aliases and later path replacement are outside this guarantee. */
int start_watcher(const char *path, AppContext *ctx);

#ifdef FC_WATCHER_TESTING
/* Test-only entry point; not part of the production ABI. */
int fc_test_process_watcher_buffer(const char *buffer, size_t length,
                                   const char *root, AppContext *ctx);
#endif

#endif
