#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "scanner/scanner.h"

static char found[8][PATH_MAX];
static size_t found_count;

static int collect(const char *path)
{
    assert(found_count < 8);
    int n = snprintf(found[found_count], sizeof(found[found_count]), "%s", path);
    assert(n > 0 && (size_t)n < sizeof(found[found_count]));
    found_count++;
    return 0;
}

static int fail_after_one(const char *path)
{
    (void)path;
    found_count++;
    errno = ECANCELED;
    return -1;
}

static void write_file(const char *path)
{
    FILE *fp = fopen(path, "w");
    assert(fp != NULL);
    assert(fputs("fixture\n", fp) >= 0);
    assert(fclose(fp) == 0);
}

int main(void)
{
    char root_template[] = "/tmp/fc-scan-root-XXXXXX";
    char outside_template[] = "/tmp/fc-scan-outside-XXXXXX";
    char *root = mkdtemp(root_template);
    char *outside = mkdtemp(outside_template);
    assert(root != NULL && outside != NULL);

    char path[PATH_MAX], outside_file[PATH_MAX], outside_dir[PATH_MAX];
    int n = snprintf(path, sizeof(path), "%s/inside.txt", root);
    assert(n > 0 && (size_t)n < sizeof(path));
    write_file(path);

    n = snprintf(outside_file, sizeof(outside_file), "%s/secret.txt", outside);
    assert(n > 0 && (size_t)n < sizeof(outside_file));
    write_file(outside_file);

    n = snprintf(outside_dir, sizeof(outside_dir), "%s/external", outside);
    assert(n > 0 && (size_t)n < sizeof(outside_dir));
    assert(mkdir(outside_dir, 0700) == 0);
    char external_child[PATH_MAX];
    n = snprintf(external_child, sizeof(external_child), "%s/hidden.txt", outside_dir);
    assert(n > 0 && (size_t)n < sizeof(external_child));
    write_file(external_child);

    char nested[PATH_MAX];
    n = snprintf(nested, sizeof(nested), "%s", root);
    assert(n > 0 && (size_t)n < sizeof(nested));
    char component[181];
    memset(component, 'd', sizeof(component) - 1);
    component[sizeof(component) - 1] = '\0';

    for (int i = 0; i < 7; i++) {
        size_t used = strlen(nested);
        n = snprintf(nested + used, sizeof(nested) - used, "/%s", component);
        assert(n > 0 && (size_t)n < sizeof(nested) - used);
        assert(mkdir(nested, 0700) == 0);
    }

    char deep_file[PATH_MAX];
    n = snprintf(deep_file, sizeof(deep_file), "%s/deep.bin", nested);
    assert(n > 0 && (size_t)n < sizeof(deep_file));
    write_file(deep_file);

    char link_path[PATH_MAX];
    n = snprintf(link_path, sizeof(link_path), "%s/file-link", root);
    assert(n > 0 && (size_t)n < sizeof(link_path));
    assert(symlink(outside_file, link_path) == 0);

    n = snprintf(link_path, sizeof(link_path), "%s/dir-link", root);
    assert(n > 0 && (size_t)n < sizeof(link_path));
    assert(symlink(outside_dir, link_path) == 0);

    found_count = 0;
    assert(scan_directory(root, collect, NULL) == 0);
    assert(found_count == 2);
    assert(strlen(deep_file) > 1024);
    assert((strcmp(found[0], path) == 0 && strcmp(found[1], deep_file) == 0) ||
           (strcmp(found[1], path) == 0 && strcmp(found[0], deep_file) == 0));

    char root_link[PATH_MAX];
    n = snprintf(root_link, sizeof(root_link), "%s/root-link", outside);
    assert(n > 0 && (size_t)n < sizeof(root_link));
    assert(symlink(root, root_link) == 0);
    assert(scan_directory(root_link, collect, NULL) == -1);

    found_count = 0;
    errno = 0;
    assert(scan_directory(root, fail_after_one, NULL) == -1);
    assert(errno == ECANCELED);
    assert(found_count == 1);

    atomic_int stop_signal;
    atomic_init(&stop_signal, SIGTERM);
    found_count = 0;
    errno = 0;
    assert(scan_directory(root, collect, &stop_signal) == -1);
    assert(errno == ECANCELED);
    assert(found_count == 0);

    puts("scanner symlink, long-path, and cancellation tests passed");
    return 0;
}
