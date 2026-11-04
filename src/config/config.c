#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "config/config.h"

#define LINE_MAX 256
#define CONFIG_MAX_WORKERS 16

static int valid_destination_folder(const char *value)
{
    if (!value || value[0] == '\0' || value[0] == '/' ||
        strchr(value, '\\') != NULL)
        return 0;

    size_t length = strlen(value);
    if (length == 0 || length >= MAX_VAL_LEN)
        return 0;

    /* A single optional trailing slash is accepted for existing configs. */
    size_t component_length = length;
    if (value[length - 1] == '/')
        component_length--;
    if (component_length == 0 || component_length >= MAX_VAL_LEN)
        return 0;

    /* Destinations are intentionally one directory component only. */
    for (size_t i = 0; i < component_length; i++)
    {
        unsigned char ch = (unsigned char)value[i];
        if (ch == '/' || ch == '\\' || iscntrl(ch) || ch == ':')
            return 0;
    }

    if ((component_length == 1 && value[0] == '.') ||
        (component_length == 2 && value[0] == '.' && value[1] == '.') ||
        (component_length == 7 && strncmp(value, "journal", 7) == 0))
        return 0;

    /* Reject repeated trailing separators or any other separator. */
    if (component_length != length - (value[length - 1] == '/' ? 1U : 0U))
        return 0;

    return 1;
}

static void config_apply_defaults(Config *cfg)
{
    cfg->batch_size = 64;
    cfg->worker_threads = 1;
    cfg->idle_sleep_ms = 1;
    cfg->mode = MODE_BATCH;
}

static void config_normalize(Config *cfg)
{
    if (cfg->batch_size <= 0)
        cfg->batch_size = 64;

    if (cfg->worker_threads <= 0)
        cfg->worker_threads = 1;
    else if (cfg->worker_threads > CONFIG_MAX_WORKERS)
        cfg->worker_threads = CONFIG_MAX_WORKERS;

    if (cfg->idle_sleep_ms < 0)
        cfg->idle_sleep_ms = 0;
}

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
        if (line[0] == '#' || line[0] == '\n')
            continue;

        char key[64], value[128];

        if (sscanf(line, "%63[^=]=%127s", key, value) != 2)
            continue;

        if (strncmp(key, "ext.", 4) == 0)
        {
            const char *ext = key + 4;
            char normalized_key[64];

            if (!valid_destination_folder(value))
            {
                fprintf(stderr, "config: unsafe destination folder '%s' for key '%s'\\n",
                        value, key);
                fclose(f);
                return -1;
            }

            if (ext[0] == '.')
            {
                int written = snprintf(normalized_key, sizeof(normalized_key), "%s", ext);
                if (written < 0 || (size_t)written >= sizeof(normalized_key))
                {
                    fprintf(stderr, "config: extension key too long '%s'\n", key);
                    fclose(f);
                    return -1;
                }
            }
            else
            {
                int written = snprintf(normalized_key, sizeof(normalized_key), ".%s", ext);
                if (written < 0 || (size_t)written >= sizeof(normalized_key))
                {
                    fprintf(stderr, "config: extension key too long '%s'\n", key);
                    fclose(f);
                    return -1;
                }
            }

            if (hashmap_put(&cfg->ext_map, normalized_key, value) != 0)
            {
                fprintf(stderr, "config: extension map full for key '%s'\n", key);
                fclose(f);
                return -1;
            }
        }
        else if (strcmp(key, "batch_size") == 0)
            cfg->batch_size = atoi(value);

        else if (strcmp(key, "worker_threads") == 0)
            cfg->worker_threads = atoi(value);

        else if (strcmp(key, "idle_sleep_ms") == 0)
            cfg->idle_sleep_ms = atoi(value);

        else if (strcmp(key, "mode") == 0)
        {
            if (strcmp(value, "batch") == 0)
                cfg->mode = MODE_BATCH;
            else
                cfg->mode = MODE_DAEMON;
        }
    }

    fclose(f);
    config_normalize(cfg);
    return 0;
}

void init_config(Config *cfg)
{
    config_apply_defaults(cfg);
}

void free_config(Config *cfg)
{
    (void)cfg;
}
