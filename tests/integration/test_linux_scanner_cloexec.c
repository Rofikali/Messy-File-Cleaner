/* Linux selftest-style check of the real scanner; no hooks or file moves. */
#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "scanner/scanner.h"

static struct stat identities[2];
static const char *expected_path;
static unsigned int callbacks;

static int same_inode(const struct stat *a, const struct stat *b)
{
    return a->st_dev == b->st_dev && a->st_ino == b->st_ino;
}

/* Count only descriptors belonging to the two private directory inodes.
 * The inventory itself has a different inode and is closed before return. */
static int inventory(unsigned int expected)
{
    DIR *d = opendir("/proc/self/fd");
    if (!d)
        return -1;
    unsigned int seen[2] = {0, 0};
    int failed = 0;
    for (;;)
    {
        errno = 0;
        struct dirent *e = readdir(d);
        if (!e)
        {
            if (errno)
                failed = 1;
            break;
        }
        char *end = NULL;
        errno = 0;
        long number = strtol(e->d_name, &end, 10);
        if (errno || end == e->d_name || *end || number < 0 || number > INT_MAX)
            continue;
        int fd = (int)number;
        struct stat st;
        if (fstat(fd, &st) != 0)
        {
            failed = 1;
            break;
        }
        for (size_t i = 0; i < 2; ++i)
            if (same_inode(&st, &identities[i]))
            {
                int flags = fcntl(fd, F_GETFD);
                if (flags < 0 || !(flags & FD_CLOEXEC) || !S_ISDIR(st.st_mode))
                    failed = 1;
                ++seen[i];
            }
    }
    if (closedir(d) != 0)
        failed = 1;
    return !failed && seen[0] == expected && seen[1] == expected ? 0 : -1;
}

static int inspect_callback(const char *path)
{
    if (strcmp(path, expected_path) != 0 || inventory(1) != 0)
    {
        errno = EIO;
        return -1;
    }
    ++callbacks;
    return 0;
}

int main(void)
{
    char root[] = "/tmp/fc-cloexec-XXXXXX";
    if (!mkdtemp(root))
        return 1;
    char child[sizeof root + 8], file[sizeof root + 16];
    int result = 1, fd = -1, child_created = 0, file_created = 0;
    if (snprintf(child, sizeof child, "%s/child", root) < 0 ||
        snprintf(file, sizeof file, "%s/child/file", root) < 0)
        goto cleanup;
    if (mkdir(child, 0700) != 0)
        goto cleanup;
    child_created = 1;
    fd = open(file, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (fd < 0)
        goto cleanup;
    file_created = 1;
    if (write(fd, "x", 1) != 1)
        goto cleanup;
    if (close(fd) != 0)
    {
        fd = -1;
        goto cleanup;
    }
    fd = -1;
    if (stat(root, &identities[0]) != 0 || stat(child, &identities[1]) != 0 ||
        inventory(0) != 0)
        goto cleanup;
    expected_path = file;
    for (unsigned int round = 0; round < 3U; ++round)
    {
        callbacks = 0;
        if (scan_directory(root, inspect_callback, NULL) != 0 ||
            callbacks != 1 || inventory(0) != 0)
            goto cleanup;
    }
    fd = open(file, O_RDONLY | O_CLOEXEC);
    char byte = 0, extra = 0;
    if (fd < 0 || read(fd, &byte, 1) != 1 || byte != 'x' ||
        read(fd, &extra, 1) != 0)
        goto cleanup;
    result = 0;
cleanup:
    if (fd >= 0 && close(fd) != 0)
        result = 1;
    if (file_created && unlink(file) != 0)
        result = 1;
    if (child_created && rmdir(child) != 0)
        result = 1;
    if (rmdir(root) != 0)
        result = 1;
    if (result == 0)
        puts("linux_scanner_cloexec=PASS rounds=3 live_directory_checks=6 restored=3 preserved=1");
    return result;
}
