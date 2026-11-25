#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "classifier/classifier.h"

enum { CASES = 12, LOOKUPS = 32768, RUNS = 3, MAX_PATH = 4096 };
static const char fallback[] = "Other/";
static const char *const keys[] = {".jpg", ".png", ".gz", ".txt"};
static const char *const folders[] = {"Jpeg/", "Png/", "Archive/", "Text/"};

/* Test-only candidate: start at end, find final dot before final slash.
 * No replacement of libc or production classifier. */
static const char *reverse_classify(Classifier *c, const char *path)
{
    if (!c || !path || !*path)
        return NULL;
    const char *end = path + strlen(path);
    const char *p = end;
    while (p != path)
    {
        --p;
        if (*p == '/')
            break;
        if (*p == '.')
        {
            if (p == path || p[-1] == '/' || p + 1 == end)
                return c->default_folder;
            const char *folder = hashmap_get(c->map, p);
            return folder ? folder : c->default_folder;
        }
    }
    return c->default_folder;
}

static int same(const char *a, const char *b)
{
    return a && b ? strcmp(a, b) == 0 : a == b;
}

static int sample(Classifier *c, char paths[CASES][MAX_PATH],
                  const char *const expected[CASES], size_t prefix,
                  int candidate, int run)
{
    struct timespec start, end;
    size_t counts[CASES] = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        return -1;
    for (size_t i = 0; i < LOOKUPS; ++i)
    {
        size_t j = i % CASES;
        const char *value = candidate ? reverse_classify(c, paths[j])
                                      : classify(c, paths[j]);
        if (!same(value, expected[j]))
            return -1;
        ++counts[j];
    }
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0 || c->map->size != 4)
        return -1;
    for (size_t j = 0; j < CASES; ++j)
        if (counts[j] != LOOKUPS / CASES + (j < LOOKUPS % CASES ? 1U : 0U))
            return -1;
    double ns = ((double)end.tv_sec - (double)start.tv_sec) * 1e9 +
                ((double)end.tv_nsec - (double)start.tv_nsec);
    if (ns <= 0)
        return -1;
    printf("mode=%s prefix_bytes=%zu run=%d lookups=%d elapsed_ns=%.0f ns_per_classification=%.3f\n",
           candidate ? "reverse" : "libc", prefix, run, LOOKUPS, ns, ns / LOOKUPS);
    return 0;
}

int main(void)
{
    const char *const names[CASES] = {
        "photo.jpg", "photo.png", "archive.tar.gz", "notes.txt", "noext",
        ".profile", "end.", "name.unknown", ".hidden.jpg", "x.JPG", "", ".."
    };
    const char *const expected[CASES] = {
        folders[0], folders[1], folders[2], folders[3], fallback,
        fallback, fallback, fallback, folders[0], fallback, fallback, fallback
    };
    const size_t prefixes[] = {0, 32, 256, 1024, 4000};
    static char paths[CASES][MAX_PATH];
    static HashEntry entries[64];
    HashMap map;
    Classifier c;
    hashmap_init(&map, entries, 64);
    for (size_t i = 0; i < 4; ++i)
        if (hashmap_put(&map, keys[i], folders[i]) != 0)
            return 1;
    classifier_init(&c, &map, fallback);
    if (classify(NULL, "a.jpg") || reverse_classify(NULL, "a.jpg") ||
        classify(&c, NULL) || reverse_classify(&c, NULL) ||
        classify(&c, "") || reverse_classify(&c, ""))
        return 1;
    for (size_t i = 0; i < CASES; ++i)
    {
        const char *want = *names[i] ? expected[i] : NULL;
        if (!same(classify(&c, names[i]), want) ||
            !same(reverse_classify(&c, names[i]), want))
            return 1;
    }
    for (size_t k = 0; k < sizeof prefixes / sizeof prefixes[0]; ++k)
    {
        size_t prefix = prefixes[k];
        for (size_t j = 0; j < CASES; ++j)
        {
            /* Even the zero-length prefix has a slash and nonempty path. */
            for (size_t i = 0; i < prefix; ++i)
                paths[j][i] = i % 16 == 0 ? '.' : (i % 31 == 0 ? '/' : 'a');
            paths[j][prefix] = '/';
            size_t n = strlen(names[j]);
            if (prefix + 1 + n + 1 > MAX_PATH)
                return 1;
            memcpy(paths[j] + prefix + 1, names[j], n + 1);
        }
        for (int r = 1; r <= RUNS; ++r)
            if (sample(&c, paths, expected, prefix, r % 2, r) != 0 ||
                sample(&c, paths, expected, prefix, !(r % 2), r) != 0)
                return 1;
    }
    return 0;
}
