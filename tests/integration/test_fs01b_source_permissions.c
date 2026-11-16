#define _POSIX_C_SOURCE 200809L

/* FS-01B-GUARD: Real mover behavior, without FC_TESTING.
 * All files live beneath a newly created private, 0700 mkdtemp root.
 * Shared-permission source directories must fail before publication.
 * This is mitigation for different-UID writable directories, not a cure
 * for adversaries with the current process UID or file capabilities.
 */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mover/mover.h"

#define CHECK(expression) do { \
    if (!(expression)) { \
        int e = errno; \
        fprintf(stderr, "FS-01B guard: %s:%d: %s (errno=%d)\n", \
                __FILE__, __LINE__, #expression, e); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static void path_of(char path[PATH_MAX], const char *parent, const char *leaf)
{
    int n = snprintf(path, PATH_MAX, "%s/%s", parent, leaf);
    CHECK(n > 0 && n < PATH_MAX);
}

static void write_fixture(const char *path, const char *bytes)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    CHECK(fd >= 0);
    size_t size = strlen(bytes), at = 0;
    while (at < size) {
        ssize_t n = write(fd, bytes + at, size - at);
        CHECK(n > 0);
        at += (size_t)n;
    }
    CHECK(fsync(fd) == 0);
    CHECK(close(fd) == 0);
}

static void check_file_bytes(const char *path, const char *bytes)
{
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    CHECK(fd >= 0);
    char buffer[128];
    ssize_t n = read(fd, buffer, sizeof(buffer));
    CHECK(n >= 0);
    CHECK((size_t)n == strlen(bytes));
    CHECK(memcmp(buffer, bytes, (size_t)n) == 0);
    CHECK(read(fd, buffer, sizeof(buffer)) == 0);
    CHECK(close(fd) == 0);
}

static void check_missing(const char *path)
{
    struct stat st;
    errno = 0;
    CHECK(lstat(path, &st) == -1 && errno == ENOENT);
}

static void check_permission_gate(const char *src_dir, const char *dst_dir,
                                  const char *name, mode_t shared_mode)
{
    char src[PATH_MAX], dst[PATH_MAX];
    path_of(src, src_dir, name);
    path_of(dst, dst_dir, name);
    write_fixture(src, "source-bytes-must-survive");

    CHECK(chmod(src_dir, shared_mode) == 0);
    struct stat source_before;
    CHECK(stat(src, &source_before) == 0);

    MoveResult result;
    memset(&result, 0, sizeof(result));
    errno = 0;
    int rc = move_file_journaled(src, dst_dir, NULL, &result);
    int saved = errno;
    CHECK(rc == -1 && saved == EPERM);
    CHECK(result.error_code == EPERM);
    CHECK(result.destination[0] == '\0');
    check_file_bytes(src, "source-bytes-must-survive");
    check_missing(dst);
    struct stat source_after;
    CHECK(stat(src, &source_after) == 0);
    CHECK(source_before.st_dev == source_after.st_dev &&
          source_before.st_ino == source_after.st_ino);

    /* Restoring the private source directory allows ordinary move
     * semantics without an untrusted group/other writable boundary. */
    CHECK(chmod(src_dir, 0700) == 0);
    memset(&result, 0, sizeof(result));
    CHECK(move_file_journaled(src, dst_dir, NULL, &result) == 0);
    CHECK(strcmp(result.destination, dst) == 0);
    check_missing(src);
    check_file_bytes(dst, "source-bytes-must-survive");
    CHECK(unlink(dst) == 0);
}

int main(void)
{
    char root_pattern[] = "/tmp/fc-fs01b-guard-XXXXXX";
    char *root = mkdtemp(root_pattern);
    CHECK(root != NULL);

    char src_dir[PATH_MAX], dst_dir[PATH_MAX];
    path_of(src_dir, root, "input");
    path_of(dst_dir, root, "output");
    CHECK(mkdir(src_dir, 0700) == 0);
    CHECK(mkdir(dst_dir, 0700) == 0);

    check_permission_gate(src_dir, dst_dir, "group-writable.bin", 0770);
    check_permission_gate(src_dir, dst_dir, "world-writable.bin", 0777);

    CHECK(rmdir(src_dir) == 0);
    CHECK(rmdir(dst_dir) == 0);
    CHECK(rmdir(root) == 0);
    puts("FS-01B guard: shared-directory retirement rejected without mutation");
    return EXIT_SUCCESS;
}
