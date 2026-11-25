#define _XOPEN_SOURCE 700
#include <errno.h>
#include <dirent.h>
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

/* Linux procfs observer: own enumeration fd is consistently excluded by
 * subtracting the same baseline. Separate from timed traversal samples. */
static int observe_fds;
static size_t baseline_fds, callback_peak_delta, observed_callbacks;

static int count_fds(size_t *out)
{
    DIR *directory = opendir("/proc/self/fd");
    if (!directory)
        return -1;
    size_t count = 0;
    struct dirent *entry;
    errno = 0;
    while ((entry = readdir(directory)) != NULL)
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
            ++count;
    int saved = errno;
    if (closedir(directory) != 0 || saved != 0)
        return -1;
    *out = count;
    return 0;
}

static int record(const char *path)
{
    for (size_t i = 0; i < FILES; ++i)
        if (strcmp(path, files[i]) == 0)
        {
            if (seen[i])
                break;
            if (observe_fds)
            {
                size_t now;
                if (count_fds(&now) != 0 || now < baseline_fds)
                {
                    errno = EIO;
                    return -1;
                }
                size_t delta = now - baseline_fds;
                if (delta > callback_peak_delta)
                    callback_peak_delta = delta;
                ++observed_callbacks;
            }
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

/* Only callback-visible high water, not a continuous FD trace. Verify
 * baseline restoration after each traversal, with no timing claims. */
static int descriptor_sample(const char *root, int reference, int deep, int budget)
{
    size_t after;
    if (count_fds(&baseline_fds) != 0)
        return -1;
    memset(seen, 0, sizeof seen);
    visits = 0;
    callback_peak_delta = 0;
    observed_callbacks = 0;
    observe_fds = 1;
    int rc = reference ? nftw(root, reference_callback, budget, FTW_PHYS)
                       : scan_directory(root, record, NULL);
    observe_fds = 0;
    if (count_fds(&after) != 0 || after != baseline_fds ||
        rc != 0 || visits != FILES || observed_callbacks != FILES)
        return -1;
    for (size_t i = 0; i < FILES; ++i)
        if (!seen[i])
            return -1;
    /* Our scanner retains every ancestor DIR until unwind. nftw's budget
     * is reported, not asserted as a process-wide descriptor guarantee. */
    if (!reference && callback_peak_delta != (deep ? DIRS + 1U : 2U))
        return -1;
    printf("descriptor layout=%s mode=%s budget=%d callbacks=%zu baseline=%zu after=%zu callback_peak_delta=%zu\n",
           deep ? "deep" : "wide", reference ? "nftw" : "scanner",
           reference ? budget : 0, visits, baseline_fds, after,
           callback_peak_delta);
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
    if (descriptor_sample(root, 0, deep, 0) != 0)
        goto cleanup;
    const int budgets[] = {1, 4, 16};
    for (size_t i = 0; i < sizeof budgets / sizeof budgets[0]; ++i)
        if (descriptor_sample(root, 1, deep, budgets[i]) != 0)
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
