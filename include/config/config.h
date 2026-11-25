#pragma once

#include "utils/hashmap.h"

typedef enum
{
    MODE_BATCH,
    MODE_DAEMON
} Mode;

typedef struct
{
    HashMap ext_map;

    int batch_size;
    int worker_threads;
    int idle_sleep_ms;

    Mode mode;

} Config;

/* Set scalar defaults only; preserve caller-owned ext_map. NULL is a no-op. */
void init_config(Config *cfg);
/* Requires a nonempty path resolving to a regular file (symlinks allowed)
   and initialized, caller-owned hash storage
   with power-of-two capacity and size <= capacity. Returns -1 on invalid
   arguments or load failure, preserving caller state. The caller must
   supply storage for the declared capacity and valid initialized entries. */
int load_config(const char *path, Config *cfg);
void free_config(Config *cfg);
