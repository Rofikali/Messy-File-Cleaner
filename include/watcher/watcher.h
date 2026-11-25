#ifndef WATCHER_H
#define WATCHER_H

#include "core/context.h"

int dispatch(AppContext *ctx, const char *path);
/* Watches a directory only; does not follow a final-component symlink.
 * Ancestor path components and later path replacement remain outside this guarantee. */
int start_watcher(const char *path, AppContext *ctx);

#endif
