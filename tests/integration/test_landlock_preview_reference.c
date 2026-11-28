#define _GNU_SOURCE
#include <stdio.h>

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

static int restricted_child(int dirfd)
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
    return 0;
}

int main(void)
{
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
    struct stat before, after, retained;
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
        _exit(restricted_child(dirfd));
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
    result = WEXITSTATUS(status);
cleanup:
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
        puts("PASS: read allowed, write-open and unlink denied; fixture preserved");
    return result;
}
#endif
