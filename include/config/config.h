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

void init_config(Config *cfg);
int load_config(const char *path, Config *cfg);
void free_config(Config *cfg);
