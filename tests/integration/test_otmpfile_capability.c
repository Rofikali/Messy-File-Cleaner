#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>

#include <linux/magic.h>

#ifndef O_TMPFILE
#define O_TMPFILE (020000000 | O_DIRECTORY)
#endif

static int unsupported(const char *operation, int error_code)
{
    fprintf(stderr, "SKIP: %s is unsupported in this filesystem/runtime: %s\n",
            operation, strerror(error_code));
    return 77;
}

static int random_bytes(void *buffer, size_t length)
{
    size_t offset = 0U;
    while (offset < length)
    {
        ssize_t received = getrandom((char *)buffer + offset,
                                     length - offset, 0);
        if (received < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (received == 0)
        {
            errno = EIO;
            return -1;
        }
        offset += (size_t)received;
    }
    return 0;
}

static int same_inode(const struct stat *left, const struct stat *right)
{
    return S_ISREG(left->st_mode) && S_ISREG(right->st_mode) &&
           left->st_dev == right->st_dev && left->st_ino == right->st_ino;
}

int main(void)
{
    char directory_template[] = "/tmp/fc-otmpfile-probe-XXXXXX";
    char *directory = mkdtemp(directory_template);
    if (directory == NULL)
    {
        fprintf(stderr, "FAIL: mkdtemp: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    int result = EXIT_FAILURE;
    int directory_fd = -1;
    int proc_fd_dir = -1;
    int temporary_fd = -1;
    char staging_name[sizeof(".fc-probe-") + 24U];
    char proc_path[64];
    int staging_entry_created = 0;

    directory_fd = open(directory, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (directory_fd < 0)
    {
        fprintf(stderr, "FAIL: open temporary directory: %s\n",
                strerror(errno));
        goto cleanup;
    }

    struct stat directory_stat;
    if (fstat(directory_fd, &directory_stat) != 0 ||
        !S_ISDIR(directory_stat.st_mode))
    {
        fprintf(stderr, "FAIL: destination descriptor is not a directory: %s\n",
                strerror(errno));
        goto cleanup;
    }

    proc_fd_dir = open("/proc/self/fd", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (proc_fd_dir < 0)
    {
        int saved_errno = errno;
        result = unsupported("/proc/self/fd", saved_errno);
        goto cleanup;
    }

    struct statfs procfs_stat;
    if (fstatfs(proc_fd_dir, &procfs_stat) != 0)
    {
        fprintf(stderr, "FAIL: fstatfs(/proc/self/fd): %s\n", strerror(errno));
        goto cleanup;
    }
    if ((unsigned long)procfs_stat.f_type != (unsigned long)PROC_SUPER_MAGIC)
    {
        result = unsupported("/proc/self/fd is not procfs", ENODEV);
        goto cleanup;
    }

    temporary_fd = openat(directory_fd, ".",
                          O_TMPFILE | O_RDWR | O_CLOEXEC, 0600);
    if (temporary_fd < 0)
    {
        int saved_errno = errno;
        if (saved_errno == EOPNOTSUPP || saved_errno == ENOTSUP ||
            saved_errno == EINVAL || saved_errno == EISDIR ||
            saved_errno == EPERM || saved_errno == ENOSYS)
            result = unsupported("O_TMPFILE", saved_errno);
        else
            fprintf(stderr, "FAIL: openat(O_TMPFILE): %s\n",
                    strerror(saved_errno));
        goto cleanup;
    }

    static const char payload[] = "journal-owned-temp-capability-v2";
    size_t offset = 0U;
    while (offset < sizeof(payload) - 1U)
    {
        ssize_t written = write(temporary_fd, payload + offset,
                                sizeof(payload) - 1U - offset);
        if (written < 0 && errno == EINTR)
            continue;
        if (written <= 0)
        {
            int saved_errno = written < 0 ? errno : EIO;
            fprintf(stderr, "FAIL: write anonymous staging file: %s\n",
                    strerror(saved_errno));
            goto cleanup;
        }
        offset += (size_t)written;
    }

    if (fsync(temporary_fd) != 0)
    {
        fprintf(stderr, "FAIL: fsync anonymous staging file: %s\n",
                strerror(errno));
        goto cleanup;
    }

    struct stat expected;
    if (fstat(temporary_fd, &expected) != 0)
    {
        fprintf(stderr, "FAIL: fstat anonymous staging file: %s\n",
                strerror(errno));
        goto cleanup;
    }
    if (!S_ISREG(expected.st_mode) || expected.st_uid != geteuid() ||
        expected.st_nlink != 0 || expected.st_dev != directory_stat.st_dev)
    {
        fprintf(stderr, "FAIL: anonymous staging inode failed owner/type/link/device checks\n");
        goto cleanup;
    }

    unsigned char random_name[12];
    if (random_bytes(random_name, sizeof(random_name)) != 0)
    {
        fprintf(stderr, "FAIL: getrandom for probe component: %s\n",
                strerror(errno));
        goto cleanup;
    }

    static const char hex[] = "0123456789abcdef";
    size_t name_offset = (size_t)snprintf(staging_name, sizeof(staging_name),
                                          ".fc-probe-");
    if (name_offset >= sizeof(staging_name))
    {
        fprintf(stderr, "FAIL: staging component formatting overflow\n");
        goto cleanup;
    }
    for (size_t i = 0U; i < sizeof(random_name); ++i)
    {
        staging_name[name_offset++] = hex[random_name[i] >> 4U];
        staging_name[name_offset++] = hex[random_name[i] & 0x0fU];
    }
    staging_name[name_offset] = '\0';

    int path_length = snprintf(proc_path, sizeof(proc_path),
                               "/proc/self/fd/%d", temporary_fd);
    if (path_length < 0 || (size_t)path_length >= sizeof(proc_path))
    {
        fprintf(stderr, "FAIL: proc-fd path formatting overflow\n");
        goto cleanup;
    }

    if (linkat(AT_FDCWD, proc_path, directory_fd, staging_name,
               AT_SYMLINK_FOLLOW) != 0)
    {
        int saved_errno = errno;
        if (saved_errno == EOPNOTSUPP || saved_errno == ENOTSUP ||
            saved_errno == EINVAL || saved_errno == EPERM ||
            saved_errno == EACCES || saved_errno == ENOSYS ||
            saved_errno == ENOENT)
            result = unsupported("linkat(proc-fd, AT_SYMLINK_FOLLOW)",
                                 saved_errno);
        else
            fprintf(stderr, "FAIL: linkat(proc-fd): %s\n",
                    strerror(saved_errno));
        goto cleanup;
    }
    staging_entry_created = 1;

    struct stat linked;
    if (fstatat(directory_fd, staging_name, &linked,
                AT_SYMLINK_NOFOLLOW) != 0 || !same_inode(&linked, &expected))
    {
        fprintf(stderr, "FAIL: linked entry did not retain exact anonymous inode identity\n");
        goto cleanup;
    }

    /* Revalidate the exact inode immediately before removing this probe entry. */
    struct stat before_unlink;
    if (fstatat(directory_fd, staging_name, &before_unlink,
                AT_SYMLINK_NOFOLLOW) != 0 ||
        !same_inode(&before_unlink, &expected))
    {
        fprintf(stderr, "FAIL: refusing to unlink a probe entry whose identity changed\n");
        goto cleanup;
    }
    if (unlinkat(directory_fd, staging_name, 0) != 0)
    {
        fprintf(stderr, "FAIL: unlink exact probe entry: %s\n", strerror(errno));
        goto cleanup;
    }
    staging_entry_created = 0;

    if (fsync(directory_fd) != 0)
    {
        fprintf(stderr, "FAIL: fsync destination directory: %s\n",
                strerror(errno));
        goto cleanup;
    }

    result = EXIT_SUCCESS;
    puts("PASS: O_TMPFILE, verified procfs, proc-fd linking, exact inode identity, and directory fsync verified");

cleanup:
    if (staging_entry_created)
    {
        struct stat current;
        if (fstatat(directory_fd, staging_name, &current,
                    AT_SYMLINK_NOFOLLOW) == 0)
        {
            struct stat original;
            if (temporary_fd >= 0 && fstat(temporary_fd, &original) == 0 &&
                same_inode(&current, &original))
            {
                if (unlinkat(directory_fd, staging_name, 0) == 0)
                    (void)fsync(directory_fd);
            }
            else
            {
                fprintf(stderr, "WARNING: preserving probe entry; inode identity could not be proven\n");
            }
        }
    }
    if (temporary_fd >= 0)
        (void)close(temporary_fd);
    if (proc_fd_dir >= 0)
        (void)close(proc_fd_dir);
    if (directory_fd >= 0)
        (void)close(directory_fd);
    if (rmdir(directory) != 0 && errno != ENOENT)
        fprintf(stderr, "WARNING: temporary probe directory preserved: %s\n",
                strerror(errno));
    return result;
}
