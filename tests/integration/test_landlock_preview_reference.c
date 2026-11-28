#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#ifndef FC_HAVE_LANDLOCK_REFERENCE
int main(void)
{
    puts("NOT VERIFIED: Landlock headers or syscall constants unavailable");
    return 77;
}
#else
#include <errno.h>
#include <fcntl.h>
#include <linux/landlock.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int unavailable(int err)
{
    return err == ENOSYS || err == EOPNOTSUPP || err == EPERM || err == EACCES;
}

static int check_read(int dirfd)
{
    char value = 0;
    int fd = openat(dirfd, "source", O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0)
        return 1;
    ssize_t n = read(fd, &value, 1);
    int closed = close(fd);
    return n != 1 || value != 'X' || closed != 0;
}

static int restrict_preview(int dirfd, int extended)
{
    struct landlock_ruleset_attr rules = {
        .handled_access_fs = LANDLOCK_ACCESS_FS_WRITE_FILE |
                             LANDLOCK_ACCESS_FS_REMOVE_FILE
    };
    long abi = syscall(SYS_landlock_create_ruleset, NULL, 0,
                       LANDLOCK_CREATE_RULESET_VERSION);
    if (abi < 1) {
        int err = errno;
        return abi < 0 && unavailable(err) ? 77 : 1;
    }
    if (extended) {
#if defined(LANDLOCK_ACCESS_FS_TRUNCATE)
        if (abi < 3)
            return 77;
        rules.handled_access_fs |= LANDLOCK_ACCESS_FS_MAKE_REG |
                                   LANDLOCK_ACCESS_FS_MAKE_DIR |
                                   LANDLOCK_ACCESS_FS_REMOVE_DIR |
                                   LANDLOCK_ACCESS_FS_TRUNCATE;
        if (fchdir(dirfd) != 0)
            return 1;
#else
        return 77;
#endif
    }
    long rule = syscall(SYS_landlock_create_ruleset, &rules, sizeof(rules), 0);
    if (rule < 0)
        return unavailable(errno) ? 77 : 1;
    if (prctl(PR_SET_NO_NEW_PRIVS, 1L, 0L, 0L, 0L) != 0) {
        int err = errno;
        (void)close((int)rule);
        return unavailable(err) ? 77 : 1;
    }
    long restricted = syscall(SYS_landlock_restrict_self, (int)rule, 0);
    int err = errno;
    if (close((int)rule) != 0)
        return 1;
    if (restricted != 0)
        return unavailable(err) ? 77 : 1;
    return 0;
}

static int restricted_child(int dirfd, int extended)
{
    int setup = restrict_preview(dirfd, extended);
    if (setup != 0)
        return setup;
    if (check_read(dirfd) != 0)
        return 1;
    int fd = openat(dirfd, "source", O_WRONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd >= 0) {
        (void)close(fd);
        return 1;
    }
    if (errno != EACCES)
        return 1;
    if (unlinkat(dirfd, "source", 0) != -1 || errno != EACCES)
        return 1;
    if (extended) {
        fd = openat(dirfd, "new-file", O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        if (fd >= 0) {
            (void)close(fd);
            return 1;
        }
        if (errno != EACCES)
            return 1;
        if (renameat(dirfd, "source", dirfd, "renamed") != -1 || errno != EACCES)
            return 1;
        if (mkdirat(dirfd, "new-dir", 0700) != -1 || errno != EACCES)
            return 1;
        if (unlinkat(dirfd, "existing-dir", AT_REMOVEDIR) != -1 || errno != EACCES)
            return 1;
        if (truncate("source", 0) != -1 || errno != EACCES)
            return 1;
    }
    return 0;
}

static int descriptor_child(int dirfd, int inherited, int close_first)
{
    if (close_first && close(inherited) != 0)
        return 1;
    int setup = restrict_preview(dirfd, 0);
    if (setup != 0)
        return setup;

    int fresh = openat(dirfd, "descriptor", O_WRONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fresh >= 0) {
        (void)close(fresh);
        return 1;
    }
    if (errno != EACCES)
        return 1;

    if (close_first) {
        if (fcntl(inherited, F_GETFD) != -1 || errno != EBADF)
            return 1;
        if (pwrite(inherited, "Z", 1, 0) != -1 || errno != EBADF)
            return 1;
    } else {
        /* Deliberately exercise only our disposable one-byte fixture. */
        if (pwrite(inherited, "Y", 1, 0) != 1)
            return 1;
        if (close(inherited) != 0)
            return 1;
    }
    return 0;
}

static int run_descriptor_child(int dirfd, int fd, int close_first)
{
    pid_t child = fork();
    if (child < 0)
        return 1;
    if (child == 0)
        _exit(descriptor_child(dirfd, fd, close_first));
    int status;
    pid_t waited;
    do {
        waited = waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    if (waited != child || !WIFEXITED(status))
        return 1;
    return WEXITSTATUS(status);
}

static int descriptor_fixture_matches(int dirfd, int fd,
                                      const struct stat *before, char expected)
{
    struct stat current, named;
    char actual = 0;
    if (fstat(fd, &current) != 0 ||
        fstatat(dirfd, "descriptor", &named, AT_SYMLINK_NOFOLLOW) != 0 ||
        !S_ISREG(named.st_mode) || current.st_dev != before->st_dev ||
        current.st_ino != before->st_ino || current.st_dev != named.st_dev ||
        current.st_ino != named.st_ino || current.st_size != 1 ||
        current.st_nlink != 1 || pread(fd, &actual, 1, 0) != 1 ||
        actual != expected)
        return 0;
    return 1;
}

static int descriptor_reference(void)
{
    char path[] = "/tmp/fc-landlock-descriptor-XXXXXX";
    if (mkdtemp(path) == NULL)
        return 1;
    int dirfd = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (dirfd < 0) {
        (void)rmdir(path);
        return 1;
    }
    int result = 1;
    int fd = openat(dirfd, "descriptor", O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (fd < 0)
        goto cleanup;
    struct stat before;
    if (write(fd, "X", 1) != 1 || fstat(fd, &before) != 0 ||
        !descriptor_fixture_matches(dirfd, fd, &before, 'X'))
        goto close_file;
    result = run_descriptor_child(dirfd, fd, 0);
    if (result != 0) {
        if (!descriptor_fixture_matches(dirfd, fd, &before, 'X'))
            result = 1;
        goto close_file;
    }
    if (!descriptor_fixture_matches(dirfd, fd, &before, 'Y')) {
        result = 1;
        goto close_file;
    }
    result = run_descriptor_child(dirfd, fd, 1);
    if (!descriptor_fixture_matches(dirfd, fd, &before, 'Y'))
        result = 1;
    /* A second-child skip cannot establish the paired hypothesis. */
close_file:
    if (close(fd) != 0)
        result = 1;
cleanup:
    if (unlinkat(dirfd, "descriptor", 0) != 0 && errno != ENOENT)
        result = 1;
    if (close(dirfd) != 0)
        result = 1;
    if (rmdir(path) != 0)
        result = 1;
    if (result == 0)
        puts("PASS: inherited writable fd retained authority; explicit close denied access");
    else if (result == 77)
        puts("NOT VERIFIED: Landlock descriptor reference unavailable");
    return result;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--descriptor") == 0)
        return descriptor_reference();
    int extended = argc == 2 && strcmp(argv[1], "--extended") == 0;
    if (argc != 1 && !extended)
        return 1;
    char path[] = "/tmp/fc-landlock-reference-XXXXXX";
    if (mkdtemp(path) == NULL)
        return 1;
    int dirfd = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    int result = 1;
    if (dirfd < 0) {
        (void)rmdir(path);
        return 1;
    }
    int fd = openat(dirfd, "source", O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (fd < 0)
        goto cleanup;
    ssize_t written = write(fd, "X", 1);
    int closed = close(fd);
    if (written != 1 || closed != 0)
        goto cleanup;
    if (linkat(dirfd, "source", dirfd, "retained", 0) != 0)
        goto cleanup;
    struct stat before, after, retained, directory_before, directory_after, absent;
    if (extended &&
        (mkdirat(dirfd, "existing-dir", 0700) != 0 ||
         fstatat(dirfd, "existing-dir", &directory_before, AT_SYMLINK_NOFOLLOW) != 0))
        goto cleanup;
    if (fstatat(dirfd, "source", &before, AT_SYMLINK_NOFOLLOW) != 0 ||
        before.st_nlink != 2 || check_read(dirfd) != 0)
        goto cleanup;
    fd = openat(dirfd, "source", O_WRONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0)
        goto cleanup;
    if (close(fd) != 0)
        goto cleanup;
    pid_t child = fork();
    if (child < 0)
        goto cleanup;
    if (child == 0)
        _exit(restricted_child(dirfd, extended));
    int status;
    pid_t waited;
    do {
        waited = waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    if (waited != child || !WIFEXITED(status))
        goto cleanup;
    if (fstatat(dirfd, "source", &after, AT_SYMLINK_NOFOLLOW) != 0 ||
        fstatat(dirfd, "retained", &retained, AT_SYMLINK_NOFOLLOW) != 0 ||
        before.st_dev != after.st_dev || before.st_ino != after.st_ino ||
        after.st_dev != retained.st_dev || after.st_ino != retained.st_ino ||
        after.st_nlink != 2 || after.st_size != 1 || check_read(dirfd) != 0)
        goto cleanup;
    if (extended) {
        if (fstatat(dirfd, "existing-dir", &directory_after, AT_SYMLINK_NOFOLLOW) != 0 ||
            !S_ISDIR(directory_after.st_mode) ||
            directory_before.st_dev != directory_after.st_dev ||
            directory_before.st_ino != directory_after.st_ino)
            goto cleanup;
        if (fstatat(dirfd, "new-file", &absent, AT_SYMLINK_NOFOLLOW) != -1 ||
            errno != ENOENT)
            goto cleanup;
        if (fstatat(dirfd, "renamed", &absent, AT_SYMLINK_NOFOLLOW) != -1 ||
            errno != ENOENT)
            goto cleanup;
        if (fstatat(dirfd, "new-dir", &absent, AT_SYMLINK_NOFOLLOW) != -1 ||
            errno != ENOENT)
            goto cleanup;
    }
    result = WEXITSTATUS(status);
cleanup:
    if (extended) {
        if (unlinkat(dirfd, "new-file", 0) != 0 && errno != ENOENT)
            result = 1;
        if (unlinkat(dirfd, "renamed", 0) != 0 && errno != ENOENT)
            result = 1;
        if (unlinkat(dirfd, "new-dir", AT_REMOVEDIR) != 0 && errno != ENOENT)
            result = 1;
        if (unlinkat(dirfd, "existing-dir", AT_REMOVEDIR) != 0 && errno != ENOENT)
            result = 1;
    }
    if (unlinkat(dirfd, "source", 0) != 0 && errno != ENOENT)
        result = 1;
    if (unlinkat(dirfd, "retained", 0) != 0 && errno != ENOENT)
        result = 1;
    if (close(dirfd) != 0)
        result = 1;
    if (rmdir(path) != 0)
        result = 1;
    if (result == 77)
        puts("NOT VERIFIED: Landlock unavailable or restricted");
    else if (result == 0)
        puts(extended ? "PASS: ABI 3 mutation rights denied; fixture preserved" :
                        "PASS: read allowed, write-open and unlink denied; fixture preserved");
    return result;
}
#endif
