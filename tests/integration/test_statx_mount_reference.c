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
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <linux/magic.h>
#include <sys/vfs.h>
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

/* Read metadata only on fixed, already existing mount roots. */
static int cross_mount_reference(void)
{
    int root = open("/", O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (root < 0)
        return 1;
    int proc = open("/proc", O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    int result = 1;
    if (proc < 0) {
        if (errno == ENOENT || errno == ENOTDIR || errno == EACCES || errno == EPERM)
            result = 77;
        goto cleanup;
    }
    struct statfs root_fs, proc_fs;
    if (fstatfs(root, &root_fs) != 0 || fstatfs(proc, &proc_fs) != 0)
        goto cleanup;
    if (proc_fs.f_type != PROC_SUPER_MAGIC || root_fs.f_type == proc_fs.f_type) {
        result = 77;
        goto cleanup;
    }
    struct statx a = {0}, b = {0};
    result = query(root, &a, 1);
    if (result != 0)
        goto cleanup;
    result = 1;
    if (query(proc, &b, 0) != 0 ||
        !S_ISDIR(a.stx_mode) || !S_ISDIR(b.stx_mode) ||
        a.stx_mnt_id == b.stx_mnt_id ||
        (a.stx_dev_major == b.stx_dev_major && a.stx_dev_minor == b.stx_dev_minor))
        goto cleanup;
    result = 0;
cleanup:
    if (proc >= 0 && close(proc) != 0)
        result = 1;
    if (close(root) != 0)
        result = 1;
    if (result == 0)
        puts("PASS: existing root/procfs mount IDs and device identities differ");
    else if (result == 77)
        puts("NOT VERIFIED: distinct root/procfs or statx mount metadata unavailable");
    return result;
}

static int rejected(int fd, const char *path, unsigned int mask, int expected)
{
    struct statx value = {0};
    return statx(fd, path, AT_SYMLINK_NOFOLLOW, mask, &value) == -1 &&
           errno == expected;
}

static int error_reference(void)
{
    char path[] = "/tmp/fc-statx-errors-XXXXXX";
    if (mkdtemp(path) == NULL)
        return 1;
    int root = open(path, O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    int result = 1;
    if (root < 0)
        goto cleanup;
    struct statx before = {0}, after = {0};
    result = query(root, &before, 1);
    if (result != 0)
        goto cleanup;
    result = 1;
    if (!rejected(root, "missing", REQUIRED_MASK, ENOENT) ||
        !rejected(-1, ".", REQUIRED_MASK, EBADF) ||
        !rejected(root, ".", REQUIRED_MASK | 0x80000000U, EINVAL) ||
        query(root, &after, 0) != 0 ||
        before.stx_mnt_id != after.stx_mnt_id ||
        before.stx_ino != after.stx_ino)
        goto cleanup;
    result = 0;
cleanup:
    if (root >= 0 && close(root) != 0)
        result = 1;
    if (rmdir(path) != 0)
        result = 1;
    if (result == 0)
        puts("PASS: ENOENT, EBADF, EINVAL and valid repeat query");
    else if (result == 77)
        puts("NOT VERIFIED: statx error reference unavailable");
    return result;
}

static int parse_mount_id(const char *line, uintmax_t *value)
{
    const char *p = line + 7; /* exact mnt_id: prefix checked by caller */
    while (*p == ' ' || *p == '\t')
        ++p;
    if (*p < '0' || *p > '9')
        return 1;
    char *end;
    errno = 0;
    uintmax_t parsed = strtoumax(p, &end, 10);
    if (errno != 0 || end == p)
        return 1;
    while (*end == ' ' || *end == '\t')
        ++end;
    if (*end != '\n' || end[1] != '\0')
        return 1;
    *value = parsed;
    return 0;
}

static int fdinfo_reference(void)
{
    int root = open("/", O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (root < 0)
        return 1;
    struct statx value = {0};
    int result = query(root, &value, 1);
    if (result != 0)
        goto cleanup;
    result = 1;
    char path[64];
    int n = snprintf(path, sizeof(path), "/proc/self/fdinfo/%d", root);
    if (n < 0 || (size_t)n >= sizeof(path))
        goto cleanup;
    FILE *stream = fopen(path, "r");
    if (stream == NULL) {
        if (errno == ENOENT || errno == ENOTDIR ||
            errno == EACCES || errno == EPERM)
            result = 77;
        goto cleanup;
    }
    char line[256];
    int found = 0, valid = 1;
    uintmax_t observed = 0;
    unsigned int lines = 0;
    while (fgets(line, sizeof(line), stream) != NULL) {
        if (++lines > 64 || strchr(line, '\n') == NULL) {
            valid = 0;
            break;
        }
        if (strncmp(line, "mnt_id:", 7) == 0) {
            if (found || parse_mount_id(line, &observed) != 0) {
                valid = 0;
                break;
            }
            found = 1;
        }
    }
    if (ferror(stream))
        valid = 0;
    if (fclose(stream) != 0)
        valid = 0;
    if (valid)
        result = !found ? 77 : observed == (uintmax_t)value.stx_mnt_id ? 0 : 1;
cleanup:
    if (close(root) != 0)
        result = 1;
    if (result == 0)
        puts("PASS: ordinary statx and own fdinfo mount IDs agree");
    else if (result == 77)
        puts("NOT VERIFIED: statx/fdinfo mount metadata unavailable");
    return result;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--errors") == 0)
        return error_reference();
    if (argc == 2 && strcmp(argv[1], "--fdinfo") == 0)
        return fdinfo_reference();
    if (argc == 2 && strcmp(argv[1], "--cross-mount") == 0)
        return cross_mount_reference();
    if (argc != 1)
        return 1;
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
