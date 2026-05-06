// ./include/utils/limits.h
#ifndef UTILS_LIMITS_H
#define UTILS_LIMITS_H

#include <limits.h>

// ── PATH_MAX fallback ─────────────────────────
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// ── NAME_MAX fallback ─────────────────────────
#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#endif
