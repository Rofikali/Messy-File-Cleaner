#define _XOPEN_SOURCE 700
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "scanner/scanner.h"

/* Private, bounded fixture; nftw is a test-only reference, not a replacement. */
enum { DIRS = 12, PER_DIR = 8, FILES = DIRS * PER_DIR, CAP = 1024, RUNS = 3 };
static char dirs[DIRS][CAP], files[FILES][CAP], links[2][CAP];
static unsigned char seen[FILES];
static size_t visits;

static int record(const char *path)
{
    for (size_t i = 0; i < FILES; ++i)
        if (strcmp(path, files[i]) == 0)
        {
            if (seen[i])
                break;
            seen[i] = 1;
            ++visits;
            return 0;
        }
    errno = EIO;
    return -1;
}

static int reference_callback(const char *path, const struct stat *st,
                              int type, struct FTW *info)
{
    (void)info;
    if (type == FTW_F && st && S_ISREG(st->st_mode))
        return record(path);
    if (type == FTW_D || type == FTW_SL || type == FTW_SLN)
        return 0;
    if (type == FTW_F && st && !S_ISREG(st->st_mode))
        return 0;
    errno = EIO;
    return -1;
}

static int sample(const char *root, int reference, int deep, int run)
{
    struct timespec start, end;
    memset(seen, 0, sizeof seen);
    visits = 0;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        return -1;
    int rc = reference ? nftw(root, reference_callback, 4, FTW_PHYS)
                       : scan_directory(root, record, NULL);
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0 || rc != 0 ||
        visits != FILES)
        return -1;
    for (size_t i = 0; i < FILES; ++i)
        if (!seen[i])
            return -1;
    double ns = ((double)end.tv_sec - (double)start.tv_sec) * 1e9 +
                ((double)end.tv_nsec - (double)start.tv_nsec);
    if (ns <= 0)
        return -1;
    printf("layout=%s mode=%s run=%d files=%zu elapsed_ns=%.0f\n",
           deep ? "deep" : "wide", reference ? "nftw" : "scanner",
           run, visits, ns);
    return 0;
}

static int fixture(int deep)
{
    char root[] = "/tmp/fc-scanner-eval-XXXXXX";
    size_t nd = 0, nf = 0, nl = 0;
    int result = -1;
    if (!mkdtemp(root))
        return -1;
    for (size_t i = 0; i < DIRS; ++i)
    {
        const char *parent = deep && i ? dirs[i - 1] : root;
        int n = snprintf(dirs[i], CAP, "%s/d%02zu", parent, i);
        if (n < 0 || n >= CAP || mkdir(dirs[i], 0700) != 0)
            goto cleanup;
        ++nd;
        for (size_t j = 0; j < PER_DIR; ++j)
        {
            n = snprintf(files[nf], CAP, "%s/f%02zu.txt", dirs[i], j);
            if (n < 0 || n >= CAP)
                goto cleanup;
            int fd = open(files[nf], O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC, 0600);
            if (fd < 0)
                goto cleanup;
            ++nf;
            if (close(fd) != 0)
                goto cleanup;
        }
    }
    int n = snprintf(links[0], CAP, "%s/file-link", root);
    if (n < 0 || n >= CAP || symlink(files[0], links[0]) != 0)
        goto cleanup;
    ++nl;
    n = snprintf(links[1], CAP, "%s/dir-link", root);
    if (n < 0 || n >= CAP || symlink(dirs[0], links[1]) != 0)
        goto cleanup;
    ++nl;
    for (int run = 1; run <= RUNS; ++run)
        if (sample(root, run % 2, deep, run) != 0 ||
            sample(root, !(run % 2), deep, run) != 0)
            goto cleanup;
    result = 0;
cleanup:
    /* Only paths successfully created by this fixture; no recursive deletion. */
    while (nl)
        if (unlink(links[--nl]) != 0)
            result = -1;
    while (nf)
        if (unlink(files[--nf]) != 0)
            result = -1;
    while (nd)
        if (rmdir(dirs[--nd]) != 0)
            result = -1;
    if (rmdir(root) != 0)
        result = -1;
    return result;
}

int main(void)
{
    return fixture(0) == 0 && fixture(1) == 0 ? 0 : 1;
}
