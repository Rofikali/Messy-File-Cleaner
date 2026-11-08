#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/magic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <unistd.h>

#ifndef O_TMPFILE
#define O_TMPFILE (020000000 | O_DIRECTORY)
#endif
#ifndef AT_SYMLINK_FOLLOW
#define AT_SYMLINK_FOLLOW 0x400
#endif

static int unsupported(const char *operation, int error_code)
{
    fprintf(stderr, "SKIP: %s is unsupported in this filesystem/runtime: %s\n",
            operation, strerror(error_code));
    return 77;
}

static int write_all(int fd, const void *buffer, size_t length)
{
    const unsigned char *bytes = buffer;
    size_t offset = 0;

    while (offset < length) {
        ssize_t written = write(fd, bytes + offset, length - offset);
        if (written < 0 && errno == EINTR)
            continue;
        if (written <= 0) {
            if (written == 0)
                errno = EIO;
            return -1;
        }
        offset += (size_t)written;
    }
    return 0;
}

static int random_component(char output[sizeof(".fc-probe-") + 32U])
{
    static const char hex[] = "0123456789abcdef";
    unsigned char random_bytes[16];
    size_t offset = 0;

    while (offset < sizeof(random_bytes)) {
        ssize_t received = getrandom(random_bytes + offset,
                                     sizeof(random_bytes) - offset, 0);
        if (received < 0 && errno == EINTR)
            continue;
        if (received <= 0) {
            if (received == 0)
                errno = EIO;
            return -1;
        }
        offset += (size_t)received;
    }

    memcpy(output, ".fc-probe-", 10U);
    for (size_t i = 0; i < sizeof(random_bytes); ++i) {
        output[10U + i * 2U] = hex[random_bytes[i] >> 4U];
        output[11U + i * 2U] = hex[random_bytes[i] & 0x0fU];
    }
    output[42U] = '\0';
    return 0;
}

static int procfs_fd_directory(void)
{
    int fd = open("/proc/self/fd", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0)
        return -1;

    struct statfs filesystem;
    if (fstatfs(fd, &filesystem) != 0) {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }
    if ((unsigned long)filesystem.f_type != (unsigned long)PROC_SUPER_MAGIC) {
        close(fd);
        errno = ENODEV;
        return -1;
    }
    return fd;
}

static int linked_entry_matches(int directory_fd, const char *name,
                                const struct stat *expected)
{
    struct stat actual;
    if (fstatat(directory_fd, name, &actual, AT_SYMLINK_NOFOLLOW) != 0)
        return 0;
    return S_ISREG(actual.st_mode) &&
           actual.st_dev == expected->st_dev &&
           actual.st_ino == expected->st_ino;
}

int main(void)
{
    char directory_template[] = "/tmp/fc-otmpfile-probe-XXXXXX";
    char *directory = mkdtemp(directory_template);
    if (!directory) {
        fprintf(stderr, "FAIL: mkdtemp: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    int result = EXIT_FAILURE;
    int directory_fd = -1;
    int temporary_fd = -1;
    int procfs_fd = -1;
    char staging_name[sizeof(".fc-probe-") + 32U];
    int linked = 0;
    struct stat expected;

    directory_fd = open(directory, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (directory_fd < 0) {
        fprintf(stderr, "FAIL: open temporary directory: %s\n", strerror(errno));
        goto cleanup;
    }

    procfs_fd = procfs_fd_directory();
    if (procfs_fd < 0) {
        int saved_errno = errno;
        result = unsupported("verified /proc/self/fd procfs", saved_errno);
        goto cleanup;
    }

    temporary_fd = openat(directory_fd, ".",
                          O_TMPFILE | O_RDWR | O_CLOEXEC, 0600);
    if (temporary_fd < 0) {
        int saved_errno = errno;
        if (saved_errno == EOPNOTSUPP || saved_errno == ENOTSUP ||
            saved_errno == EINVAL || saved_errno == EISDIR ||
            saved_errno == EPERM || saved_errno == ENOSYS)
            result = unsupported("O_TMPFILE", saved_errno);
        else
            fprintf(stderr, "FAIL: openat(O_TMPFILE): %s\n", strerror(saved_errno));
        goto cleanup;
    }

    static const char payload[] = "journal-owned-temp-proc-fd-capability-v2";
    if (write_all(temporary_fd, payload, sizeof(payload) - 1U) != 0 ||
        fsync(temporary_fd) != 0) {
        fprintf(stderr, "FAIL: write/fsync anonymous staging file: %s\n",
                strerror(errno));
        goto cleanup;
    }

    if (fstat(temporary_fd, &expected) != 0) {
        fprintf(stderr, "FAIL: fstat anonymous staging file: %s\n", strerror(errno));
        goto cleanup;
    }
    struct stat directory_stat;
    if (fstat(directory_fd, &directory_stat) != 0) {
        fprintf(stderr, "FAIL: fstat destination directory: %s\n", strerror(errno));
        goto cleanup;
    }
    if (!S_ISREG(expected.st_mode) || expected.st_uid != geteuid() ||
        expected.st_nlink != 0 || expected.st_dev != directory_stat.st_dev) {
        fprintf(stderr, "FAIL: anonymous inode ownership/type/link/device invariant failed\n");
        goto cleanup;
    }

    if (random_component(staging_name) != 0) {
        fprintf(stderr, "FAIL: getrandom staging component: %s\n", strerror(errno));
        goto cleanup;
    }

    char proc_path[64];
    int path_length = snprintf(proc_path, sizeof(proc_path),
                               "/proc/self/fd/%d", temporary_fd);
    if (path_length < 0 || (size_t)path_length >= sizeof(proc_path)) {
        fprintf(stderr, "FAIL: proc-fd path formatting overflow\n");
        goto cleanup;
    }

    if (linkat(AT_FDCWD, proc_path, directory_fd, staging_name,
               AT_SYMLINK_FOLLOW) != 0) {
        int saved_errno = errno;
        if (saved_errno == EOPNOTSUPP || saved_errno == ENOTSUP ||
            saved_errno == EINVAL || saved_errno == EPERM ||
            saved_errno == EACCES || saved_errno == ENOSYS ||
            saved_errno == ENOENT)
            result = unsupported("linkat(proc-fd, AT_SYMLINK_FOLLOW)", saved_errno);
        else
            fprintf(stderr, "FAIL: linkat(proc-fd): %s\n", strerror(saved_errno));
        goto cleanup;
    }
    linked = 1;

    if (!linked_entry_matches(directory_fd, staging_name, &expected)) {
        fprintf(stderr, "FAIL: linked entry does not match the still-open anonymous inode\n");
        goto cleanup;
    }

    if (!linked_entry_matches(directory_fd, staging_name, &expected)) {
        fprintf(stderr, "FAIL: staging entry identity changed before cleanup\n");
        goto cleanup;
    }
    if (unlinkat(directory_fd, staging_name, 0) != 0) {
        fprintf(stderr, "FAIL: unlink exact verified probe: %s\n", strerror(errno));
        goto cleanup;
    }
    linked = 0;
    if (fsync(directory_fd) != 0) {
        fprintf(stderr, "FAIL: fsync destination directory: %s\n", strerror(errno));
        goto cleanup;
    }

    puts("PASS: O_TMPFILE, verified procfs, unprivileged proc-fd linking, exact inode identity, and directory fsync verified");
    result = EXIT_SUCCESS;

cleanup:
    if (linked && directory_fd >= 0 && temporary_fd >= 0) {
        if (fstat(temporary_fd, &expected) == 0 &&
            linked_entry_matches(directory_fd, staging_name, &expected)) {
            if (unlinkat(directory_fd, staging_name, 0) != 0)
                fprintf(stderr, "WARN: verified probe cleanup failed: %s\n", strerror(errno));
            else if (fsync(directory_fd) != 0)
                fprintf(stderr, "WARN: probe cleanup directory fsync failed: %s\n", strerror(errno));
        } else {
            fprintf(stderr, "WARN: probe entry identity uncertain; preserving it\n");
        }
    }
    if (temporary_fd >= 0 && close(temporary_fd) != 0)
        fprintf(stderr, "WARN: close staging descriptor failed: %s\n", strerror(errno));
    if (procfs_fd >= 0 && close(procfs_fd) != 0)
        fprintf(stderr, "WARN: close procfs descriptor failed: %s\n", strerror(errno));
    if (directory_fd >= 0 && close(directory_fd) != 0)
        fprintf(stderr, "WARN: close destination descriptor failed: %s\n", strerror(errno));
    if (rmdir(directory) != 0 && errno != ENOENT)
        fprintf(stderr, "WARN: temporary directory cleanup failed: %s\n", strerror(errno));
    return result;
}
