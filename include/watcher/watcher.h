// include/watcher/watcher.h

#ifndef WATCHER_H
#define WATCHER_H

#include "core/context.h" // <--- Add this

void start_watcher(const char *path, AppContext *ctx);

#endif
