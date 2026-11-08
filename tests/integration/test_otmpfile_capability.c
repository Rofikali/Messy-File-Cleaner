#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <linux/magic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/stat.h>
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

static int fill_random(void *buffer, size_t length)
{
    unsigned char *bytes = buffer;
    size_t offset = 0;
    while (offset < length)
    {
        ssize_t count = getrandom(bytes + offset, length - offset, 0);
        if (count < 0 && errno == EINTR)
            continue;
        if (count <= 0)
        {
            if (count == 0)
                errno = EIO;
            return -1;
        }
        offset += (size_t)count;
    }
    return 0;
}

static int write_all(int fd, const void *buffer, size_t length)
{
    const unsigned char *bytes = buffer;
    size_t offset = 0;
    while (offset < length)
    {
        ssize_t count = write(fd, bytes + offset, length - offset);
        if (count < 0 && errno == EINTR)
            continue;
        if (count <= 0)
        {
            if (count == 0)
                errno = EIO;
            return -1;
        }
        offset += (size_t)count;
    }
    return 0;
}

static int same_inode(const struct stat *left, const struct stat *right)
{
    return S_ISREG(left->st_mode) && S_ISREG(right->st_mode) &&
           left->st_dev == right->st_dev && left->st_ino == right->st_ino;
}

static int verify_procfs(void)
{
    int proc_fd = open("/proc/self/fd", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (proc_fd < 0)
        return -1;

    struct statfs filesystem;
    int result = fstatfs(proc_fd, &filesystem);
    int saved_errno = errno;
    (void)close(proc_fd);
    if (result != 0)
    {
        errno = saved_errno;
        return -1;
    }
    if ((unsigned long)filesystem.f_type != (unsigned long)PROC_SUPER_MAGIC)
    {
        errno = ENODEV;
        return -1;
    }
    return 0;
}

int main(void)
{
    char directory_template[] = "/tmp/fc-otmpfile-probe-XXXXXX";
    char *directory = mkdtemp(directory_template);
    if (!directory)
    {
        fprintf(stderr, "FAIL: mkdtemp: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    int directory_fd = open(directory, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (directory_fd < 0)
    {
        int saved_errno = errno;
        (void)rmdir(directory);
        fprintf(stderr, "FAIL: open temporary directory: %s\n",
                strerror(saved_errno));
        return EXIT_FAILURE;
    }

    struct stat directory_stat;
    if (fstat(directory_fd, &directory_stat) != 0 ||
        !S_ISDIR(directory_stat.st_mode))
    {
        int saved_errno = errno ? errno : ENOTDIR;
        (void)close(directory_fd);
        (void)rmdir(directory);
        fprintf(stderr, "FAIL: validate destination directory: %s\n",
                strerror(saved_errno));
        return EXIT_FAILURE;
    }

    if (verify_procfs() != 0)
    {
        int saved_errno = errno;
        (void)close(directory_fd);
        (void)rmdir(directory);
        if (saved_errno == ENOENT || saved_errno == ENODEV ||
            saved_errno == EACCES || saved_errno == EPERM)
            return unsupported("/proc/self/fd procfs", saved_errno);
        fprintf(stderr, "FAIL: verify /proc/self/fd: %s\n",
                strerror(saved_errno));
        return EXIT_FAILURE;
    }

    int temporary_fd = openat(directory_fd, ".",
                              O_TMPFILE | O_RDWR | O_CLOEXEC, 0600);
    if (temporary_fd < 0)
    {
        int saved_errno = errno;
        (void)close(directory_fd);
        (void)rmdir(directory);
        if (saved_errno == EOPNOTSUPP || saved_errno == ENOTSUP ||
            saved_errno == EINVAL || saved_errno == EISDIR ||
            saved_errno == EPERM || saved_errno == ENOSYS)
            return unsupported("O_TMPFILE", saved_errno);
        fprintf(stderr, "FAIL: openat(O_TMPFILE): %s\n", strerror(saved_errno));
        return EXIT_FAILURE;
    }

    static const char payload[] = "journal-owned-temp-capability-v2";
    if (write_all(temporary_fd, payload, sizeof(payload) - 1U) != 0)
    {
        int saved_errno = errno;
        (void)close(temporary_fd);
        (void)close(directory_fd);
        (void)rmdir(directory);
        fprintf(stderr, "FAIL: write anonymous staging file: %s\n",
                strerror(saved_errno));
        return EXIT_FAILURE;
    }

    if (fsync(temporary_fd) != 0)
    {
        int saved_errno = errno;
        (void)close(temporary_fd);
        (void)close(directory_fd);
        (void)rmdir(directory);
        fprintf(stderr, "FAIL: fsync anonymous staging file: %s\n",
                strerror(saved_errno));
        return EXIT_FAILURE;
    }

    struct stat expected;
    if (fstat(temporary_fd, &expected) != 0 ||
        !S_ISREG(expected.st_mode) || expected.st_nlink != 0 ||
        expected.st_uid != geteuid() ||
        expected.st_dev != directory_stat.st_dev)
    {
        int saved_errno = errno ? errno : EINVAL;
        (void)close(temporary_fd);
        (void)close(directory_fd);
        (void)rmdir(directory);
        fprintf(stderr, "FAIL: anonymous staging inode validation: %s\n",
                strerror(saved_errno));
        return EXIT_FAILURE;
    }

    unsigned char random_bytes[16];
    if (fill_random(random_bytes, sizeof(random_bytes)) != 0)
    {
        int saved_errno = errno;
        (void)close(temporary_fd);
        (void)close(directory_fd);
        (void)rmdir(directory);
        fprintf(stderr, "FAIL: getrandom for probe component: %s\n",
                strerror(saved_errno));
        return EXIT_FAILURE;
    }

    char staging_name[sizeof(".fc-otmpfile-probe-") + sizeof(random_bytes) * 2U];
    static const char hex[] = "0123456789abcdef";
    const char prefix[] = ".fc-otmpfile-probe-";
    size_t prefix_length = sizeof(prefix) - 1U;
    memcpy(staging_name, prefix, prefix_length);
    for (size_t i = 0; i < sizeof(random_bytes); i++)
    {
        staging_name[prefix_length + i * 2U] = hex[random_bytes[i] >> 4];
        staging_name[prefix_length + i * 2U + 1U] =
            hex[random_bytes[i] & 0x0fU];
    }
    staging_name[prefix_length + sizeof(random_bytes) * 2U] = '\0';

    char proc_path[64];
    int proc_length = snprintf(proc_path, sizeof(proc_path),
                               "/proc/self/fd/%d", temporary_fd);
    if (proc_length < 0 || (size_t)proc_length >= sizeof(proc_path))
    {
        (void)close(temporary_fd);
        (void)close(directory_fd);
        (void)rmdir(directory);
        fprintf(stderr, "FAIL: format proc-fd path\n");
        return EXIT_FAILURE;
    }

    if (linkat(AT_FDCWD, proc_path, directory_fd, staging_name,
               AT_SYMLINK_FOLLOW) != 0)
    {
        int saved_errno = errno;
        (void)close(temporary_fd);
        (void)close(directory_fd);
        (void)rmdir(directory);
        if (saved_errno == EOPNOTSUPP || saved_errno == ENOTSUP ||
            saved_errno == EINVAL || saved_errno == EPERM ||
            saved_errno == EACCES || saved_errno == ENOSYS ||
            saved_errno == ENOENT)
            return unsupported("linkat(proc-fd, AT_SYMLINK_FOLLOW)", saved_errno);
        fprintf(stderr, "FAIL: linkat(proc-fd): %s\n", strerror(saved_errno));
        return EXIT_FAILURE;
    }

    struct stat linked;
    if (fstatat(directory_fd, staging_name, &linked,
                AT_SYMLINK_NOFOLLOW) != 0 ||
        !same_inode(&expected, &linked))
    {
        int saved_errno = errno ? errno : ESTALE;
        (void)close(temporary_fd);
        (void)close(directory_fd);
        fprintf(stderr,
                "FAIL: linked entry identity mismatch; entry preserved: %s\n",
                strerror(saved_errno));
        return EXIT_FAILURE;
    }

    struct stat before_unlink;
    if (fstatat(directory_fd, staging_name, &before_unlink,
                AT_SYMLINK_NOFOLLOW) != 0 ||
        !same_inode(&expected, &before_unlink))
    {
        int saved_errno = errno ? errno : ESTALE;
        (void)close(temporary_fd);
        (void)close(directory_fd);
        fprintf(stderr,
                "FAIL: probe entry changed before unlink; entry preserved: %s\n",
                strerror(saved_errno));
        return EXIT_FAILURE;
    }

    if (unlinkat(directory_fd, staging_name, 0) != 0)
    {
        int saved_errno = errno;
        (void)close(temporary_fd);
        (void)close(directory_fd);
        fprintf(stderr, "FAIL: unlink exact verified probe entry: %s\n",
                strerror(saved_errno));
        return EXIT_FAILURE;
    }

    if (fsync(directory_fd) != 0)
    {
        int saved_errno = errno;
        (void)close(temporary_fd);
        (void)close(directory_fd);
        fprintf(stderr, "FAIL: fsync destination directory: %s\n",
                strerror(saved_errno));
        return EXIT_FAILURE;
    }

    if (close(temporary_fd) != 0 || close(directory_fd) != 0 ||
        rmdir(directory) != 0)
    {
        fprintf(stderr, "FAIL: cleanup capability probe: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    puts("PASS: O_TMPFILE, verified procfs, unprivileged proc-fd link, "
         "inode identity, exact cleanup, and directory fsync verified");
    return EXIT_SUCCESS;
}
