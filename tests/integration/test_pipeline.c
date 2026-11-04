#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "scanner/scanner.h"

static char found_paths[8][PATH_MAX];
static size_t found_count;

static void collect_path(const char *path)
{
    assert(found_count < 8);
    (void)snprintf(found_paths[found_count], sizeof(found_paths[found_count]), "%s", path);
    found_count++;
}

static void write_fixture(const char *path)
{
    FILE *file = fopen(path, "w");
    assert(file != NULL);
    assert(fputs("scanner fixture\n", file) >= 0);
    assert(fclose(file) == 0);
}

static void remove_tree(const char *path)
{
    DIR *directory = opendir(path);
    if (!directory)
    {
        (void)unlink(path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        char child[PATH_MAX];
        int written = snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
        assert(written > 0 && (size_t)written < sizeof(child));
        struct stat st;
        assert(lstat(child, &st) == 0);
        if (S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode))
            remove_tree(child);
        else
            assert(unlink(child) == 0);
    }

    closedir(directory);
    assert(rmdir(path) == 0);
}

int main(void)
{
    char root_template[] = "/tmp/file-cleaner-scan-XXXXXX";
    char outside_template[] = "/tmp/file-cleaner-outside-XXXXXX";
    char *root = mkdtemp(root_template);
    char *outside = mkdtemp(outside_template);
    assert(root != NULL && outside != NULL);

    char regular_file[PATH_MAX];
    char outside_file[PATH_MAX];
    char outside_dir[PATH_MAX];
    char nested_path[PATH_MAX];

    assert(snprintf(regular_file, sizeof(regular_file), "%s/inside.txt", root) > 0);
    assert(snprintf(outside_file, sizeof(outside_file), "%s/secret.txt", outside) > 0);
    assert(snprintf(outside_dir, sizeof(outside_dir), "%s/external", outside) > 0);
    assert(snprintf(nested_path, sizeof(nested_path), "%s", root) > 0);

    write_fixture(regular_file);
    write_fixture(outside_file);
    assert(mkdir(outside_dir, 0700) == 0);

    /* Build a path longer than the old 1024-byte scanner buffer. */
    char component[181];
    memset(component, 'd', sizeof(component) - 1);
    component[sizeof(component) - 1] = '\0';
    for (int i = 0; i < 7; i++)
    {
        size_t current_len = strlen(nested_path);
        int written = snprintf(nested_path + current_len,
                               sizeof(nested_path) - current_len,
                               "/%s", component);
        assert(written > 0 && (size_t)written < sizeof(nested_path) - current_len);
        assert(mkdir(nested_path, 0700) == 0);
    }

    char deep_file[PATH_MAX];
    assert(snprintf(deep_file, sizeof(deep_file), "%s/deep.bin", nested_path) > 0);
    write_fixture(deep_file);

    char file_link[PATH_MAX];
    char dir_link[PATH_MAX];
    assert(snprintf(file_link, sizeof(file_link), "%s/external-file-link", root) > 0);
    assert(snprintf(dir_link, sizeof(dir_link), "%s/external-dir-link", root) > 0);
    assert(symlink(outside_file, file_link) == 0);
    assert(symlink(outside_dir, dir_link) == 0);

    assert(mkfifo((snprintf(file_link, sizeof(file_link), "%s/pipe", root), file_link), 0600) == 0);

    found_count = 0;
    assert(scan_directory(root, collect_path) == 0);
    assert(found_count == 2);
    assert(strcmp(found_paths[0], regular_file) == 0 ||
           strcmp(found_paths[1], regular_file) == 0);
    assert(strcmp(found_paths[0], deep_file) == 0 ||
           strcmp(found_paths[1], deep_file) == 0);

    char root_link[PATH_MAX];
    assert(snprintf(root_link, sizeof(root_link), "%s/root-link", outside) > 0);
    assert(symlink(root, root_link) == 0);
    assert(scan_directory(root_link, collect_path) == -1);

    remove_tree(root);
    remove_tree(outside);
    puts("scanner security tests passed");
    return 0;
}
