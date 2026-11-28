#define _GNU_SOURCE
#include <stdio.h>

#ifndef FC_HAVE_STATX_MOUNT_REFERENCE
int main(void)
{
    puts("NOT VERIFIED: statx mount-ID headers or libc unavailable");
    return 77;
}
#else
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#define REQUIRED_MASK (STATX_TYPE | STATX_INO | STATX_MNT_ID)

static int query(int fd, struct statx *value, int initial)
{
    if (statx(fd, "", AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW,
              REQUIRED_MASK, value) != 0) {
        if (initial && (errno == ENOSYS || errno == EOPNOTSUPP ||
                        errno == EPERM || errno == EACCES))
            return 77;
        return 1;
    }
    if ((value->stx_mask & REQUIRED_MASK) != REQUIRED_MASK)
        return initial ? 77 : 1;
    struct stat conventional;
    if (fstat(fd, &conventional) != 0 ||
        value->stx_dev_major != major(conventional.st_dev) ||
        value->stx_dev_minor != minor(conventional.st_dev) ||
        value->stx_ino != (uint64_t)conventional.st_ino ||
        (value->stx_mode & S_IFMT) != (conventional.st_mode & S_IFMT))
        return 1;
    return 0;
}

int main(void)
{
    char path[] = "/tmp/fc-statx-mount-XXXXXX";
    if (mkdtemp(path) == NULL)
        return 1;
    int root = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (root < 0) {
        (void)rmdir(path);
        return 1;
    }
    int a = -1, alias = -1, b = -1;
    int made_a = 0, made_alias = 0, made_b = 0;
    struct statx dir = {0}, first = {0}, link = {0}, separate = {0};
    int result = query(root, &dir, 1);
    if (result != 0)
        goto cleanup;
    result = 1;
    a = openat(root, "first", O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (a < 0)
        goto cleanup;
    made_a = 1;
    if (linkat(root, "first", root, "alias", 0) != 0)
        goto cleanup;
    made_alias = 1;
    alias = openat(root, "alias", O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (alias < 0)
        goto cleanup;
    b = openat(root, "separate", O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (b < 0)
        goto cleanup;
    made_b = 1;
    if (query(a, &first, 0) != 0 || query(alias, &link, 0) != 0 ||
        query(b, &separate, 0) != 0)
        goto cleanup;
    if (!S_ISDIR(dir.stx_mode) || !S_ISREG(first.stx_mode) ||
        !S_ISREG(link.stx_mode) || !S_ISREG(separate.stx_mode) ||
        dir.stx_mnt_id != first.stx_mnt_id ||
        first.stx_mnt_id != link.stx_mnt_id ||
        first.stx_mnt_id != separate.stx_mnt_id ||
        first.stx_dev_major != link.stx_dev_major ||
        first.stx_dev_minor != link.stx_dev_minor ||
        first.stx_ino != link.stx_ino ||
        first.stx_ino == separate.stx_ino)
        goto cleanup;
    result = 0;
cleanup:
    if (b >= 0 && close(b) != 0) result = 1;
    if (alias >= 0 && close(alias) != 0) result = 1;
    if (a >= 0 && close(a) != 0) result = 1;
    if (made_b && unlinkat(root, "separate", 0) != 0) result = 1;
    if (made_alias && unlinkat(root, "alias", 0) != 0) result = 1;
    if (made_a && unlinkat(root, "first", 0) != 0) result = 1;
    if (close(root) != 0) result = 1;
    if (rmdir(path) != 0) result = 1;
    if (result == 0)
        puts("PASS: statx/fstat agree; same mount, hard-link identity and separate inode distinguished");
    else if (result == 77)
        puts("NOT VERIFIED: statx mount ID unavailable");
    return result;
}
#endif
