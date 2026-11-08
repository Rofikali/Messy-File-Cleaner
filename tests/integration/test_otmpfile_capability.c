#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef O_TMPFILE
#define O_TMPFILE (020000000 | O_DIRECTORY)
#endif
#ifndef AT_EMPTY_PATH
#define AT_EMPTY_PATH 0x1000
#endif

static int unsupported(const char *operation, int error_code)
{
    fprintf(stderr, "SKIP: %s is unsupported in this filesystem/runtime: %s\n",
            operation, strerror(error_code));
    return 77;
}

int main(void)
{
    char directory_template[] = "/tmp/fc-otmpfile-probe-XXXXXX";
    char *directory = mkdtemp(directory_template);
    if (!directory) {
        fprintf(stderr, "FAIL: mkdtemp: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    int directory_fd = open(directory, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (directory_fd < 0) {
        int saved_errno = errno;
        (void)rmdir(directory);
        fprintf(stderr, "FAIL: open temporary directory: %s\n",
                strerror(saved_errno));
        return EXIT_FAILURE;
    }

    int temporary_fd = openat(directory_fd, ".",
                              O_TMPFILE | O_RDWR | O_CLOEXEC, 0600);
    if (temporary_fd < 0) {
        int saved_errno = errno;
        close(directory_fd);
        (void)rmdir(directory);
        if (saved_errno == EOPNOTSUPP || saved_errno == ENOTSUP ||
            saved_errno == EINVAL || saved_errno == EISDIR ||
            saved_errno == EPERM || saved_errno == ENOSYS)
            return unsupported("O_TMPFILE", saved_errno);
        fprintf(stderr, "FAIL: openat(O_TMPFILE): %s\n", strerror(saved_errno));
        return EXIT_FAILURE;
    }

    static const char payload[] = "journal-owned-temp-capability-v1";
    size_t offset = 0;
    while (offset < sizeof(payload) - 1U) {
        ssize_t written = write(temporary_fd, payload + offset,
                                sizeof(payload) - 1U - offset);
        if (written < 0 && errno == EINTR)
            continue;
        if (written <= 0) {
            int saved_errno = written < 0 ? errno : EIO;
            close(temporary_fd);
            close(directory_fd);
            (void)rmdir(directory);
            fprintf(stderr, "FAIL: write anonymous staging file: %s\n",
                    strerror(saved_errno));
            return EXIT_FAILURE;
        }
        offset += (size_t)written;
    }

    if (fsync(temporary_fd) != 0) {
        int saved_errno = errno;
        close(temporary_fd);
        close(directory_fd);
        (void)rmdir(directory);
        fprintf(stderr, "FAIL: fsync anonymous staging file: %s\n",
                strerror(saved_errno));
        return EXIT_FAILURE;
    }

    struct stat expected;
    if (fstat(temporary_fd, &expected) != 0) {
        int saved_errno = errno;
        close(temporary_fd);
        close(directory_fd);
        (void)rmdir(directory);
        fprintf(stderr, "FAIL: fstat anonymous staging file: %s\n",
                strerror(saved_errno));
        return EXIT_FAILURE;
    }

    const char staging_name[] = ".fc-otmpfile-probe-entry";
    if (linkat(temporary_fd, "", directory_fd, staging_name, AT_EMPTY_PATH) != 0) {
        int saved_errno = errno;
        close(temporary_fd);
        close(directory_fd);
        (void)rmdir(directory);
        if (saved_errno == EOPNOTSUPP || saved_errno == ENOTSUP ||
            saved_errno == EINVAL || saved_errno == EPERM ||
            saved_errno == ENOSYS || saved_errno == ENOENT)
            return unsupported("linkat(AT_EMPTY_PATH)", saved_errno);
        fprintf(stderr, "FAIL: linkat(AT_EMPTY_PATH): %s\n",
                strerror(saved_errno));
        return EXIT_FAILURE;
    }

    struct stat linked;
    if (fstatat(directory_fd, staging_name, &linked, AT_SYMLINK_NOFOLLOW) != 0 ||
        !S_ISREG(linked.st_mode) || linked.st_dev != expected.st_dev ||
        linked.st_ino != expected.st_ino) {
        int saved_errno = errno;
        (void)unlinkat(directory_fd, staging_name, 0);
        close(temporary_fd);
        close(directory_fd);
        (void)rmdir(directory);
        fprintf(stderr, "FAIL: linked entry did not retain the anonymous inode identity%s%s\n",
                saved_errno ? ": " : "", saved_errno ? strerror(saved_errno) : "");
        return EXIT_FAILURE;
    }

    if (unlinkat(directory_fd, staging_name, 0) != 0 ||
        fsync(directory_fd) != 0) {
        int saved_errno = errno;
        close(temporary_fd);
        close(directory_fd);
        (void)rmdir(directory);
        fprintf(stderr, "FAIL: remove/sync probe entry: %s\n",
                strerror(saved_errno));
        return EXIT_FAILURE;
    }

    if (close(temporary_fd) != 0 || close(directory_fd) != 0 ||
        rmdir(directory) != 0) {
        fprintf(stderr, "FAIL: cleanup capability probe: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    puts("PASS: O_TMPFILE, fsync, AT_EMPTY_PATH linking, and inode identity verified");
    return EXIT_SUCCESS;
}
