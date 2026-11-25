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

/* Capture exact diagnostic bytes using disposable POSIX file descriptors. */
static void assert_safe_diagnostic(const char *path, const char *input,
                                   const char *expected)
{
    FILE *file = fopen(path, "wb");
    assert(file != NULL);
    assert(fputs(input, file) >= 0);
    assert(fclose(file) == 0);

    HashEntry entries[64];
    Config cfg;
    hashmap_init(&cfg.ext_map, entries, 64);
    init_config(&cfg);
    assert(fflush(stderr) == 0);
    FILE *capture = tmpfile();
    assert(capture != NULL);
    int saved = dup(STDERR_FILENO);
    assert(saved >= 0);
    assert(dup2(fileno(capture), STDERR_FILENO) >= 0);
    int result = load_config(path, &cfg);
    int flushed = fflush(stderr);
    int restored = dup2(saved, STDERR_FILENO);
    assert(close(saved) == 0);
    assert(restored >= 0 && flushed == 0);
    assert(result != 0);
    assert(cfg.ext_map.size == 0);
    assert(fseek(capture, 0, SEEK_SET) == 0);
    char bytes[256];
    size_t count = fread(bytes, 1, sizeof(bytes), capture);
    assert(!ferror(capture));
    assert(count == strlen(expected));
    assert(memcmp(bytes, expected, count) == 0);
    assert(fclose(capture) == 0);
}

int main(void)
{
    char path[] = "/tmp/fc-config-test-XXXXXX";
    int fd = mkstemp(path);
    assert(fd >= 0);
    assert(close(fd) == 0);

    /* Neither invalid destination nor oversized mapping key may echo
       terminal control bytes or untrusted strings into stderr. */
    assert_safe_diagnostic(path, "ext.jpg=Bad\033[31mFolder\n",
                           "config: unsafe extension destination\n");
    assert_safe_diagnostic(path, "ext.bad\033[31mkey=../outside\n",
                           "config: unsafe extension destination\n");
    assert_safe_diagnostic(path, "ext.long\033[31mextensionname=Safe\n",
                           "config: invalid extension key\n");
    assert_safe_diagnostic(path, "ext.j\033pg=Jpeg\n",
                           "config: invalid extension key\n");
    assert_safe_diagnostic(path, "ext.j\tpg=Jpeg\n",
                           "config: invalid extension key\n");

    /* All C whitespace separators must identify a malformed bare recognized
       setting, including vertical tab and form feed. */
    {
        const char *keys[] = {"mode", "batch_size", "worker_threads", "idle_sleep_ms", NULL};
        const char separators[] = {'\v', '\f'};
        for (size_t k = 0; keys[k]; ++k)
            for (size_t s = 0; s < sizeof(separators); ++s)
            {
                FILE *file = fopen(path, "w");
                assert(file != NULL);
                assert(fprintf(file, "batch_size=123\n%s%cunexpected\n", keys[k], separators[s]) > 0);
                assert(fclose(file) == 0);
                HashEntry entries[64];
                Config cfg;
                hashmap_init(&cfg.ext_map, entries, 64);
                init_config(&cfg);
                cfg.batch_size = 77;
                assert(hashmap_put(&cfg.ext_map, ".png", "Existing") == 0);
                assert(load_config(path, &cfg) != 0);
                assert(cfg.batch_size == 77 && cfg.ext_map.size == 1);
                assert(strcmp(hashmap_get(&cfg.ext_map, ".png"), "Existing") == 0);
            }
    }

    /* Initialization is null-safe and resets only scalar defaults. */
    init_config(NULL);
    free_config(NULL);
    {
        HashEntry entries[64];
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, 64);
        assert(hashmap_put(&cfg.ext_map, ".png", "Existing") == 0);
        cfg.batch_size = 77;
        cfg.worker_threads = 3;
        cfg.idle_sleep_ms = 9;
        cfg.mode = MODE_DAEMON;
        init_config(&cfg);
        assert(cfg.batch_size == 64 && cfg.worker_threads == 1);
        assert(cfg.idle_sleep_ms == 1 && cfg.mode == MODE_BATCH);
        assert(cfg.ext_map.entries == entries && cfg.ext_map.capacity == 64);
        assert(cfg.ext_map.size == 1);
        assert(strcmp(hashmap_get(&cfg.ext_map, ".png"), "Existing") == 0);
        free_config(&cfg);
        assert(cfg.ext_map.entries == entries && cfg.ext_map.size == 1);
    }

    /* Reject invalid API arguments before opening the file or copying storage. */
    {
        HashEntry entries[64];
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, 64);
        init_config(&cfg);
        cfg.batch_size = 77;
        assert(hashmap_put(&cfg.ext_map, ".png", "Existing") == 0);
        assert(load_config(NULL, &cfg) != 0);
        assert(load_config("", &cfg) != 0);
        assert(load_config(path, NULL) != 0);
        assert(cfg.batch_size == 77 && cfg.ext_map.size == 1);
        assert(strcmp(hashmap_get(&cfg.ext_map, ".png"), "Existing") == 0);

        FILE *empty = fopen(path, "w");
        assert(empty != NULL);
        assert(fclose(empty) == 0);
        /* Empty config previously reached normalization despite invalid map metadata. */
        Config invalid = cfg;
        invalid.ext_map.capacity = 3;
        assert(load_config(path, &invalid) != 0);
        assert(invalid.ext_map.capacity == 3 && invalid.batch_size == 77);
        invalid = cfg;
        invalid.ext_map.size = 65;
        assert(load_config(path, &invalid) != 0);
        assert(invalid.ext_map.size == 65 && invalid.batch_size == 77);
        invalid = cfg;
        invalid.ext_map.entries = NULL;
        assert(load_config(path, &invalid) != 0);
        invalid = cfg;
        invalid.ext_map.capacity = 0;
        assert(load_config(path, &invalid) != 0);
        assert(cfg.ext_map.entries == entries && cfg.ext_map.size == 1);
        assert(strcmp(hashmap_get(&cfg.ext_map, ".png"), "Existing") == 0);
    }

    assert(load_with_destination(path, "Jpeg/") == 0);
    assert(load_with_destination(path, "Png") == 0);
    assert(load_with_destination(path, "Others/") == 0);


    /* Late invalid input must preserve both scalar and routing state. */
    {
        FILE *file = fopen(path, "w");
        assert(file != NULL);
        assert(fputs("batch_size=123\next.jpg=NewFolder\nmode=invalid\n", file) >= 0);
        assert(fclose(file) == 0);
        HashEntry entries[64];
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, 64);
        init_config(&cfg);
        assert(hashmap_put(&cfg.ext_map, ".png", "Existing") == 0);
        cfg.batch_size = 77;
        assert(load_config(path, &cfg) != 0);
        assert(cfg.batch_size == 77);
        assert(cfg.ext_map.size == 1);
        assert(strcmp(hashmap_get(&cfg.ext_map, ".png"), "Existing") == 0);
        assert(hashmap_get(&cfg.ext_map, ".jpg") == NULL);
        assert(cfg.ext_map.entries == entries);
    }

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

    /* Extension destinations must reject extra tokens, not silently accept the prefix. */
    const char *invalid_extension_lines[] = {
        "ext.jpg=Jpeg extra\n",
        "ext.jpg=Jpeg #unexpected\n",
        "ext.jpg=Jpeg Png\n",
        NULL
    };
    for (size_t i = 0; invalid_extension_lines[i]; ++i)
    {
        FILE *file = fopen(path, "w");
        assert(file != NULL);
        assert(fputs(invalid_extension_lines[i], file) >= 0);
        assert(fclose(file) == 0);
        static HashEntry entries[64];
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, 64);
        init_config(&cfg);
        assert(load_config(path, &cfg) != 0);
    }

    /* Missing extension destination must not silently omit a routing rule. */
    const char *missing_extension_lines[] = {"ext.jpg\n", "ext.jpg=\n", "  ext.png = \n", NULL};
    for (size_t i = 0; missing_extension_lines[i]; ++i)
    {
        FILE *file = fopen(path, "w");
        assert(file != NULL);
        assert(fputs(missing_extension_lines[i], file) >= 0);
        assert(fclose(file) == 0);
        static HashEntry entries[64];
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, 64);
        init_config(&cfg);
        assert(load_config(path, &cfg) != 0);
    }

    /* A missing extension name must not register a meaningless '.' mapping. */
    const char *empty_extension_lines[] = {"ext.=Jpeg\n", "ext..=Jpeg\n", "  ext. = Jpeg\n", NULL};
    for (size_t i = 0; empty_extension_lines[i]; ++i)
    {
        FILE *file = fopen(path, "w");
        assert(file != NULL);
        assert(fputs(empty_extension_lines[i], file) >= 0);
        assert(fclose(file) == 0);
        static HashEntry entries[64];
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, 64);
        init_config(&cfg);
        assert(load_config(path, &cfg) != 0);
    }

    /* Bare mode must fail rather than silently selecting the default execution mode. */
    {
        FILE *file = fopen(path, "w");
        assert(file != NULL);
        assert(fputs("mode\n", file) >= 0);
        assert(fclose(file) == 0);
        static HashEntry entries[64];
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, 64);
        init_config(&cfg);
        assert(load_config(path, &cfg) != 0);
    }

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
    /* Whitespace before '=' must not bypass validation of recognized numeric keys. */
    for (size_t k = 0; numeric_keys[k]; ++k)
    {
        FILE *file = fopen(path, "w");
        assert(file != NULL);
        assert(fprintf(file, "%s =12junk\n", numeric_keys[k]) > 0);
        assert(fclose(file) == 0);
        static HashEntry entries[64];
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, 64);
        init_config(&cfg);
        assert(load_config(path, &cfg) != 0);
    }

    /* Leading indentation must not hide malformed numeric values. */
    for (size_t k = 0; numeric_keys[k]; ++k)
    {
        FILE *file = fopen(path, "w");
        assert(file != NULL);
        assert(fprintf(file, "  %s=12junk\n", numeric_keys[k]) > 0);
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

    /* A key wider than sscanf's 63-character field must be rejected,
       including a long unrecognized key with an otherwise valid value. */
    {
        FILE *file = fopen(path, "w");
        assert(file != NULL);
        for (int i = 0; i < 64; ++i)
            assert(fputc('k', file) != EOF);
        assert(fputs("=batch\n", file) >= 0);
        assert(fclose(file) == 0);
        HashEntry entries[64];
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, 64);
        init_config(&cfg);
        assert(load_config(path, &cfg) != 0);
    }

    /* A NUL in a physical line (including comments and final lines) must
       fail atomically, rather than hide a suffix from C-string parsing. */
    {
        static const char *const prefixes[] = {"mode=batch", "# comment", "ext.jpg=Jpeg", ""};
        static const char *const suffixes[] = {"", "daemon\n", "\n"};
        for (size_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); ++i)
            for (size_t j = 0; j < sizeof(suffixes) / sizeof(suffixes[0]); ++j)
            {
                FILE *file = fopen(path, "wb");
                assert(file != NULL);
                assert(fputs("batch_size=123\n", file) >= 0);
                assert(fputs(prefixes[i], file) >= 0);
                assert(fputc(0, file) != EOF);
                assert(fputs(suffixes[j], file) >= 0);
                assert(fclose(file) == 0);
                HashEntry entries[64];
                Config cfg;
                hashmap_init(&cfg.ext_map, entries, 64);
                init_config(&cfg);
                cfg.batch_size = 77;
                assert(hashmap_put(&cfg.ext_map, ".png", "Existing") == 0);
                assert(load_config(path, &cfg) != 0);
                assert(cfg.batch_size == 77 && cfg.ext_map.size == 1);
                assert(strcmp(hashmap_get(&cfg.ext_map, ".png"), "Existing") == 0);
                assert(cfg.ext_map.entries == entries);
            }
        FILE *file = fopen(path, "wb");
        assert(file != NULL);
        assert(fputs("mode=batch", file) >= 0);
        assert(fclose(file) == 0);
        HashEntry entries[64];
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, 64);
        init_config(&cfg);
        assert(load_config(path, &cfg) == 0);
        assert(cfg.mode == MODE_BATCH);
    }

    /* A long physical config line must fail, not parse a valid truncated prefix. */
    {
        FILE *file = fopen(path, "w");
        assert(file != NULL);
        assert(fputs("mode=batch", file) >= 0);
        for (int i = 0; i < 300; ++i)
            assert(fputc(' ', file) != EOF);
        assert(fputs("daemon\n", file) >= 0);
        assert(fclose(file) == 0);
        static HashEntry entries[64];
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, 64);
        init_config(&cfg);
        assert(load_config(path, &cfg) != 0);
    }

#ifdef __linux__
    /* On Linux fopen(directory) may succeed, but fgets fails with EISDIR.
       This disposable fixture exercises the stream-error path without
       touching customer data or requiring injected production hooks. */
    {
        char dir[] = "/tmp/fc-config-read-error-XXXXXX";
        assert(mkdtemp(dir) != NULL);
        static HashEntry entries[64];
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, 64);
        init_config(&cfg);
        assert(load_config(dir, &cfg) != 0);
        assert(rmdir(dir) == 0);
    }
#endif

    assert(unlink(path) == 0);
    puts("config destination security tests passed");
    return 0;
}
