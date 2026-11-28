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
#include <time.h>

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

enum { TIMING_SAMPLES = 7, TIMING_CALLS = 10000, WARMUP_CALLS = 256 };

static int timed_batch(int fd, const struct statx *expected, int extended,
                       unsigned int calls, uint64_t *elapsed)
{
    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        return 1;
    for (unsigned int i = 0; i < calls; ++i) {
        if (extended) {
            struct statx value;
            if (statx(fd, "", AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW,
                      REQUIRED_MASK, &value) != 0 ||
                (value.stx_mask & REQUIRED_MASK) != REQUIRED_MASK ||
                value.stx_dev_major != expected->stx_dev_major ||
                value.stx_dev_minor != expected->stx_dev_minor ||
                value.stx_ino != expected->stx_ino ||
                (value.stx_mode & S_IFMT) != (expected->stx_mode & S_IFMT) ||
                value.stx_mnt_id != expected->stx_mnt_id)
                return 1;
        } else {
            struct stat value;
            if (fstat(fd, &value) != 0 ||
                major(value.st_dev) != expected->stx_dev_major ||
                minor(value.st_dev) != expected->stx_dev_minor ||
                (uint64_t)value.st_ino != expected->stx_ino ||
                (value.st_mode & S_IFMT) != (expected->stx_mode & S_IFMT))
                return 1;
        }
    }
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0)
        return 1;
    if (end.tv_sec < start.tv_sec ||
        (end.tv_sec == start.tv_sec && end.tv_nsec <= start.tv_nsec))
        return 1;
    uint64_t seconds = (uint64_t)(end.tv_sec - start.tv_sec);
    if (seconds > 10)
        return 1;
    *elapsed = seconds * UINT64_C(1000000000) +
               (uint64_t)end.tv_nsec - (uint64_t)start.tv_nsec;
    return *elapsed == 0;
}

static uint64_t median_per_call(uint64_t samples[TIMING_SAMPLES])
{
    for (unsigned int i = 1; i < TIMING_SAMPLES; ++i) {
        uint64_t v = samples[i];
        unsigned int j = i;
        while (j > 0 && samples[j - 1] > v) {
            samples[j] = samples[j - 1];
            --j;
        }
        samples[j] = v;
    }
    return samples[TIMING_SAMPLES / 2] / TIMING_CALLS;
}

static int timing_reference(void)
{
    char path[] = "/tmp/fc-statx-timing-XXXXXX";
    if (mkdtemp(path) == NULL)
        return 1;
    int fd = open(path, O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    int result = 1;
    if (fd < 0)
        goto cleanup;
    struct statx expected = {0};
    result = query(fd, &expected, 1);
    if (result != 0)
        goto cleanup;
    result = 1;
    uint64_t ignored, conventional[TIMING_SAMPLES], extended[TIMING_SAMPLES];
    if (timed_batch(fd, &expected, 0, WARMUP_CALLS, &ignored) ||
        timed_batch(fd, &expected, 1, WARMUP_CALLS, &ignored))
        goto cleanup;
    for (unsigned int i = 0; i < TIMING_SAMPLES; ++i) {
        if (i % 2 == 0) {
            if (timed_batch(fd, &expected, 0, TIMING_CALLS, &conventional[i]) ||
                timed_batch(fd, &expected, 1, TIMING_CALLS, &extended[i]))
                goto cleanup;
        } else {
            if (timed_batch(fd, &expected, 1, TIMING_CALLS, &extended[i]) ||
                timed_batch(fd, &expected, 0, TIMING_CALLS, &conventional[i]))
                goto cleanup;
        }
        printf("statx_timing sample=%u calls=%u first=%s fstat_batch_ns=%" PRIu64
               " statx_batch_ns=%" PRIu64 "\n", i + 1, (unsigned int)TIMING_CALLS,
               i % 2 == 0 ? "fstat" : "statx", conventional[i], extended[i]);
    }
    printf("statx_timing median_fstat_ns_per_call=%" PRIu64
           " median_statx_ns_per_call=%" PRIu64
           " scope=warm_single_fd_with_validation\n",
           median_per_call(conventional), median_per_call(extended));
    result = 0;
cleanup:
    if (fd >= 0 && close(fd) != 0)
        result = 1;
    if (rmdir(path) != 0)
        result = 1;
    if (result == 77)
        puts("NOT VERIFIED: statx timing capability unavailable");
    return result;
}

/* Observe existing mounts only; topology is not guaranteed by this test. */
static int same_device_reference(void)
{
    static const char *const candidates[] = {"/proc/sys", "/proc/irq", "/proc/bus"};
    int root = open("/proc", O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (root < 0)
        return errno == ENOENT || errno == ENOTDIR || errno == EACCES ||
               errno == EPERM ? 77 : 1;
    int result = 1;
    struct statfs root_fs;
    struct statx a = {0};
    if (fstatfs(root, &root_fs) != 0)
        goto cleanup;
    if (root_fs.f_type != PROC_SUPER_MAGIC) {
        result = 77;
        goto cleanup;
    }
    result = query(root, &a, 1);
    if (result != 0)
        goto cleanup;
    result = 77;
    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
        int fd = open(candidates[i], O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        if (fd < 0) {
            if (errno == ENOENT || errno == ENOTDIR || errno == EACCES || errno == EPERM)
                continue;
            result = 1;
            break;
        }
        struct statfs fs;
        struct statx b = {0};
        int matched = 0;
        int valid = fstatfs(fd, &fs) == 0;
        if (valid && fs.f_type == PROC_SUPER_MAGIC) {
            valid = query(fd, &b, 0) == 0 && S_ISDIR(b.stx_mode);
            if (valid)
                matched = a.stx_dev_major == b.stx_dev_major &&
                          a.stx_dev_minor == b.stx_dev_minor &&
                          a.stx_mnt_id != b.stx_mnt_id;
        }
        if (close(fd) != 0 || !valid) {
            result = 1;
            break;
        }
        if (matched) {
            result = 0;
            break;
        }
    }
cleanup:
    if (close(root) != 0)
        result = 1;
    if (result == 0)
        puts("PASS: existing procfs directories share device identity but differ in mount ID");
    else if (result == 77)
        puts("NOT VERIFIED: no qualifying existing same-device mount pair or capability");
    return result;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--same-device") == 0)
        return same_device_reference();
    if (argc == 2 && strcmp(argv[1], "--timing") == 0)
        return timing_reference();
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
