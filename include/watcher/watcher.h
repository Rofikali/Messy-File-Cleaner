#ifndef WATCHER_H
#define WATCHER_H

#include "core/context.h"

/* Returns an index in [0, num_workers), or -1/EINVAL for invalid inputs.
 * The caller keeps num_workers stable and in [1, MAX_WORKERS] during dispatch. */
int dispatch(AppContext *ctx, const char *path);
/* Watches a directory only; does not follow a final-component symlink.
 * Ancestor path components and later path replacement remain outside this guarantee. */
int start_watcher(const char *path, AppContext *ctx);

#endif
