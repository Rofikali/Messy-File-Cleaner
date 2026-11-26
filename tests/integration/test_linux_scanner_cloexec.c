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
#include <sys/wait.h>
#include <unistd.h>
#include "scanner/scanner.h"

enum { DEEP_LEVELS = 20, FIXTURE_PATH = 256 };
static struct stat identities[DEEP_LEVELS];
static size_t identity_count = 2;
static const char *identity_paths[DEEP_LEVELS];
static const char *expected_path;
static unsigned int callbacks, exec_checks;
static int callback_error;
static atomic_int cancel_scan;
static int cancel_after_probe;

static int same_inode(const struct stat *a, const struct stat *b)
{
    return a->st_dev == b->st_dev && a->st_ino == b->st_ino;
}

/* Count only descriptors belonging to private fixture directory inodes.
 * The inventory itself has a different inode and is closed before return. */
static int inventory_with_extra(unsigned int expected_root, unsigned int expected_child,
                                int require_cloexec, size_t extra_index)
{
    DIR *d = opendir("/proc/self/fd");
    if (!d)
        return -1;
    unsigned int seen[DEEP_LEVELS] = {0};
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
        for (size_t i = 0; i < identity_count; ++i)
            if (same_inode(&st, &identities[i]))
            {
                int flags = fcntl(fd, F_GETFD);
                if (flags < 0 || ((flags & FD_CLOEXEC) != 0) != require_cloexec ||
                    !S_ISDIR(st.st_mode))
                    failed = 1;
                ++seen[i];
            }
    }
    if (closedir(d) != 0)
        failed = 1;
    if (failed || seen[0] != expected_root)
        return -1;
    for (size_t i = 1; i < identity_count; ++i)
        if (seen[i] != expected_child + (unsigned int)(i == extra_index))
            return -1;
    return 0;
}

static int inventory(unsigned int expected_root, unsigned int expected_child,
                     int require_cloexec)
{
    return inventory_with_extra(expected_root, expected_child, require_cloexec,
                                identity_count);
}

enum { PROBE_CLEAN = 0, PROBE_ROOT = 1, PROBE_DEEPEST = 2,
       PROBE_REJECT_LEAK = 3, LEAK_DETECTED_EXIT = 42 };

/* All intentional inheritance is confined to the forked test child.
 * Mode 3 requires the child to observe the exact injected deepest inode,
 * then reject the zero-inheritance expectation. No shell or PATH lookup. */
static int exec_probe(unsigned int mode)
{
    if (mode > PROBE_REJECT_LEAK)
        return -1;
    char mode_arg[] = {(char)('0' + mode), '\0'};
    char *args[DEEP_LEVELS + 4] = {
        (char *)"scanner-exec-probe", (char *)"--exec-probe", mode_arg
    };
    for (size_t i = 0; i < identity_count; ++i)
        args[i + 3] = (char *)identity_paths[i];
    args[identity_count + 3] = NULL;
    pid_t pid = fork();
    if (pid < 0)
        return -1;
    if (pid == 0)
    {
        if (mode != PROBE_CLEAN)
        {
            const size_t target = mode == PROBE_ROOT ? 0 : identity_count - 1;
            if (open(identity_paths[target], O_RDONLY | O_DIRECTORY) < 0)
                _exit(125);
        }
        execv("/proc/self/exe", args);
        _exit(126);
    }
    int status;
    pid_t waited;
    do {
        waited = waitpid(pid, &status, 0);
    } while (waited < 0 && errno == EINTR);
    const int expected_exit = mode == PROBE_REJECT_LEAK ? LEAK_DETECTED_EXIT : 0;
    if (waited != pid || !WIFEXITED(status) || WEXITSTATUS(status) != expected_exit)
        return -1;
    ++exec_checks;
    return 0;
}

static int inspect_callback(const char *path)
{
    if (strcmp(path, expected_path) != 0 || inventory(1, 1, 1) != 0 ||
        exec_probe(PROBE_CLEAN) != 0 || exec_probe(PROBE_ROOT) != 0 ||
        (identity_count > 2 &&
         (exec_probe(PROBE_DEEPEST) != 0 || exec_probe(PROBE_REJECT_LEAK) != 0)) ||
        inventory(1, 1, 1) != 0)
    {
        errno = EIO;
        return -1;
    }
    ++callbacks;
    if (cancel_after_probe)
        atomic_store_explicit(&cancel_scan, 1, memory_order_relaxed);
    if (callback_error)
    {
        errno = callback_error;
        return -1;
    }
    return 0;
}

/* Cross the scanner's initial 16-frame capacity with real directory
 * streams, check every live inode, then repeat after cancellation. */
static int deep_descriptor_tests(void)
{
    char dirs[DEEP_LEVELS][FIXTURE_PATH] = {"/tmp/fc-cloexec-deep-XXXXXX"};
    char file[FIXTURE_PATH];
    size_t created = 0;
    int file_created = 0, fd = -1, result = 1;
    if (!mkdtemp(dirs[0]))
        return 1;
    created = 1;
    for (size_t i = 1; i < DEEP_LEVELS; ++i)
    {
        int n = snprintf(dirs[i], sizeof dirs[i], "%s/d", dirs[i - 1]);
        if (n < 0 || (size_t)n >= sizeof dirs[i] || mkdir(dirs[i], 0700) != 0)
            goto cleanup;
        ++created;
    }
    int n = snprintf(file, sizeof file, "%s/file", dirs[DEEP_LEVELS - 1]);
    if (n < 0 || (size_t)n >= sizeof file)
        goto cleanup;
    fd = open(file, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (fd < 0)
        goto cleanup;
    file_created = 1;
    if (write(fd, "d", 1) != 1)
        goto cleanup;
    if (close(fd) != 0)
    {
        fd = -1;
        goto cleanup;
    }
    fd = -1;
    identity_count = DEEP_LEVELS;
    for (size_t i = 0; i < identity_count; ++i)
    {
        identity_paths[i] = dirs[i];
        if (stat(dirs[i], &identities[i]) != 0)
            goto cleanup;
        for (size_t j = 0; j < i; ++j)
            if (same_inode(&identities[i], &identities[j]))
                goto cleanup;
    }
    if (inventory(0, 0, 1) != 0)
        goto cleanup;
    expected_path = file;
    exec_checks = 0;
    callback_error = 0;
    for (unsigned int round = 0; round < 3U; ++round)
    {
        callbacks = 0;
        cancel_after_probe = round == 1U;
        atomic_store_explicit(&cancel_scan, 0, memory_order_relaxed);
        errno = 0;
        const int status = scan_directory(dirs[0], inspect_callback, &cancel_scan);
        const int error = errno;
        if ((round == 1U ? status != -1 || error != ECANCELED : status != 0) ||
            callbacks != 1 || inventory(0, 0, 1) != 0)
            goto cleanup;
    }
    if (exec_checks != 12U)
        goto cleanup;
    fd = open(file, O_RDONLY | O_CLOEXEC);
    char byte = 0, extra = 0;
    if (fd < 0 || read(fd, &byte, 1) != 1 || byte != 'd' ||
        read(fd, &extra, 1) != 0)
        goto cleanup;
    result = 0;
cleanup:
    if (fd >= 0 && close(fd) != 0)
        result = 1;
    if (file_created && unlink(file) != 0)
        result = 1;
    while (created != 0)
        if (rmdir(dirs[--created]) != 0)
            result = 1;
    if (result == 0)
        puts("linux_scanner_deep_cloexec=PASS depth=20 scans=3 live_identities=60 restored=3 exec_negative=3 exec_positive=3 exec_deepest=3 leak_rejected=3 abort_cancel=1 recovery=1 preserved=1");
    return result;
}

int main(int argc, char **argv)
{
    if (argc >= 5 && argc <= DEEP_LEVELS + 3 &&
        strcmp(argv[1], "--exec-probe") == 0)
    {
        if (argv[2][0] < '0' || argv[2][0] > '3' || argv[2][1] != '\0')
            return 1;
        const unsigned int mode = (unsigned int)(argv[2][0] - '0');
        identity_count = (size_t)(argc - 3);
        for (size_t i = 0; i < identity_count; ++i)
            if (stat(argv[i + 3], &identities[i]) != 0)
                return 1;
        if (mode == PROBE_CLEAN || mode == PROBE_ROOT)
            return inventory((unsigned int)(mode == PROBE_ROOT), 0, 0) == 0 ? 0 : 1;
        if (inventory_with_extra(0, 0, 0, identity_count - 1) != 0)
            return 1;
        if (mode == PROBE_DEEPEST)
            return 0;
        /* Do not credit a generic child error as detection. The exact
         * intentional leak was observed above; require rejection here. */
        return inventory(0, 0, 0) != 0 ? LEAK_DETECTED_EXIT : 1;
    }
    if (argc != 1)
        return 1;
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
        inventory(0, 0, 1) != 0)
        goto cleanup;
    expected_path = file;
    identity_paths[0] = root;
    identity_paths[1] = child;
    for (unsigned int round = 0; round < 3U; ++round)
    {
        callbacks = 0;
        if (scan_directory(root, inspect_callback, NULL) != 0 ||
            callbacks != 1 || inventory(0, 0, 1) != 0)
            goto cleanup;
    }
    /* Exercise real scanner unwinding after both child probes have run.
     * Each abort is followed immediately by a clean scan of the same inodes. */
    const int errors[] = {EACCES, EIO, 0};
    for (size_t scenario = 0; scenario < sizeof errors / sizeof errors[0]; ++scenario)
    {
        callbacks = 0;
        callback_error = errors[scenario];
        cancel_after_probe = callback_error == 0;
        atomic_store_explicit(&cancel_scan, 0, memory_order_relaxed);
        const int expected_error = callback_error ? callback_error : ECANCELED;
        errno = 0;
        const int scan_status = scan_directory(root, inspect_callback, &cancel_scan);
        const int scan_error = errno;
        if (scan_status != -1 || scan_error != expected_error ||
            callbacks != 1 || inventory(0, 0, 1) != 0)
            goto cleanup;
        callbacks = 0;
        callback_error = 0;
        cancel_after_probe = 0;
        atomic_store_explicit(&cancel_scan, 0, memory_order_relaxed);
        if (scan_directory(root, inspect_callback, &cancel_scan) != 0 ||
            callbacks != 1 || inventory(0, 0, 1) != 0)
            goto cleanup;
    }
    if (exec_checks != 18U)
        goto cleanup;
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
    {
        puts("linux_scanner_cloexec=PASS rounds=3 live_directory_checks=18 restored=9 preserved=1 exec_negative=9 exec_positive=9 abort_errors=2 abort_cancel=1 recovery=3");
        result = deep_descriptor_tests();
    }
    return result;
}
