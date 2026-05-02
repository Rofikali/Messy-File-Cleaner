// ./src/core/config.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config/config.h"

#define LINE_MAX 256

int load_config(const char *path, Config *cfg)
{
  FILE *f = fopen(path, "r");
  if (!f)
  {
    perror("config open");
    return -1;
  }

  char line[LINE_MAX];

  while (fgets(line, sizeof(line), f))
  {
    // skip comments
    if (line[0] == '#' || line[0] == '\n')
      continue;

    char key[64], value[128];

    if (sscanf(line, "%63[^=]=%127s", key, value) != 2)
      continue;

    // ── EXTENSIONS ─────────────
    if (strncmp(key, "ext.", 4) == 0)
    {
      hashmap_put(&cfg->ext_map, key + 4, strdup(value));
    }

    // ── PIPELINE ───────────────
    else if (strcmp(key, "batch_size") == 0)
      cfg->batch_size = atoi(value);

    else if (strcmp(key, "worker_threads") == 0)
      cfg->worker_threads = atoi(value);

    else if (strcmp(key, "idle_sleep_ms") == 0)
      cfg->idle_sleep_ms = atoi(value);

    else if (strcmp(key, "fsync_batch") == 0)
      cfg->fsync_batch = atoi(value);
  }

  fclose(f);
  return 0;
}
