#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config/config.h"
#include "utils/hashmap.h"

static void write_config(const char *path, const char *destination)
{
    FILE *file = fopen(path, "w");
    assert(file != NULL);
    assert(fprintf(file, "ext.jpg=%s\n", destination) > 0);
    assert(fclose(file) == 0);
}

static int load_with_destination(const char *path, const char *destination)
{
    static HashEntry entries[64];
    Config config;
    hashmap_init(&config.ext_map, entries, 64);
    init_config(&config);
    write_config(path, destination);
    return load_config(path, &config);
}

int main(void)
{
    char path[] = "/tmp/fc-config-test-XXXXXX";
    int fd = mkstemp(path);
    assert(fd >= 0);
    assert(close(fd) == 0);

    assert(load_with_destination(path, "Jpeg/") == 0);
    assert(load_with_destination(path, "Png") == 0);
    assert(load_with_destination(path, "Others/") == 0);

    const char *unsafe[] = {
        "../outside",
        "/tmp/outside",
        "nested/category",
        "./",
        "../",
        "journal/",
        "safe//",
        "safe\\folder",
        NULL
    };

    for (size_t i = 0; unsafe[i] != NULL; i++)
        assert(load_with_destination(path, unsafe[i]) != 0);

    /* A typo must not silently select daemon mode. */
    const char *valid_modes[] = {"batch", "daemon", NULL};
    const char *invalid_modes[] = {"batc", "DAEMON", "watch", "batchx", "batch daemon", "daemon batch", "batch #unexpected", NULL};
    for (size_t i = 0; valid_modes[i]; ++i)
    {
        FILE *file = fopen(path, "w");
        assert(file != NULL);
        assert(fprintf(file, "mode=%s\n", valid_modes[i]) > 0);
        assert(fclose(file) == 0);
        static HashEntry entries[64];
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, 64);
        init_config(&cfg);
        assert(load_config(path, &cfg) == 0);
        assert(cfg.mode == (i == 0 ? MODE_BATCH : MODE_DAEMON));
    }
    for (size_t i = 0; invalid_modes[i]; ++i)
    {
        FILE *file = fopen(path, "w");
        assert(file != NULL);
        assert(fprintf(file, "mode=%s\n", invalid_modes[i]) > 0);
        assert(fclose(file) == 0);
        static HashEntry entries[64];
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, 64);
        init_config(&cfg);
        assert(load_config(path, &cfg) != 0);
    }

    /* Reject numeric typos, overflow and trailing tokens instead of atoi() fallback. */
    const char *numeric_keys[] = {"batch_size", "worker_threads", "idle_sleep_ms", NULL};
    const char *invalid_numbers[] = {"abc", "12junk", "999999999999999999999", "1 2", "", NULL};
    for (size_t k = 0; numeric_keys[k]; ++k)
        for (size_t i = 0; invalid_numbers[i]; ++i)
        {
            FILE *file = fopen(path, "w");
            assert(file != NULL);
            assert(fprintf(file, "%s=%s\n", numeric_keys[k], invalid_numbers[i]) > 0);
            assert(fclose(file) == 0);
            static HashEntry entries[64];
            Config cfg;
            hashmap_init(&cfg.ext_map, entries, 64);
            init_config(&cfg);
            assert(load_config(path, &cfg) != 0);
        }
    /* Bare recognized numeric keys must not silently retain defaults. */
    for (size_t k = 0; numeric_keys[k]; ++k)
    {
        FILE *file = fopen(path, "w");
        assert(file != NULL);
        assert(fprintf(file, "%s\n", numeric_keys[k]) > 0);
        assert(fclose(file) == 0);
        static HashEntry entries[64];
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, 64);
        init_config(&cfg);
        assert(load_config(path, &cfg) != 0);
    }

    /* Valid signed values retain the existing normalize/clamp behavior. */
    {
        FILE *file = fopen(path, "w");
        assert(file != NULL);
        assert(fputs("batch_size=128\nworker_threads=99\nidle_sleep_ms=-1\n", file) >= 0);
        assert(fclose(file) == 0);
        static HashEntry entries[64];
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, 64);
        init_config(&cfg);
        assert(load_config(path, &cfg) == 0);
        assert(cfg.batch_size == 128 && cfg.worker_threads == 16 && cfg.idle_sleep_ms == 0);
    }

    assert(unlink(path) == 0);
    puts("config destination security tests passed");
    return 0;
}
