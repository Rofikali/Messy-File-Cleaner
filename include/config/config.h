// // ./include/core/config.h

// #ifndef CONFIG_H
// #define CONFIG_H

// #include "utils/hashmap.h"

// typedef struct
// {
//     HashMap ext_map;

//     int batch_size;
//     int worker_threads;
//     int idle_sleep_ms;

//     int fsync_batch;

// } Config;

// int load_config(const char *path, Config *cfg);

// #endif

#ifndef CONFIG_H
#define CONFIG_H

#include "utils/hashmap.h"

typedef struct
{
    HashMap ext_map;

    int batch_size;
    int worker_threads;
    int idle_sleep_ms;
    int fsync_batch;

} Config;

int load_config(const char *path, Config *cfg);

#endif
