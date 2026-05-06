// ./include/watcher/watcher.h
// include/watcher/watcher.h

#ifndef WATCHER_H
#define WATCHER_H

#include "core/context.h" // <--- Add this

int dispatch(AppContext *ctx, const char *path);
void start_watcher(const char *path, AppContext *ctx);

#endif
