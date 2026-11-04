#ifndef WATCHER_H
#define WATCHER_H

#include "core/context.h"

int dispatch(AppContext *ctx, const char *path);
int start_watcher(const char *path, AppContext *ctx);

#endif
