#define _POSIX_C_SOURCE 200809L

/* FS-01B KNOWN-RISK CHARACTERIZATION (not a safety regression pass).
 *
 * All paths are children of an exclusive mkdtemp fixture. The test retains
 * a separate hard link to the replacement inode before scheduling the
 * post-check entry swap, so the observed unowned unlink cannot destroy
 * the only copy of its bytes. Production has NO retire-probe hook.
 *
 * Passing means the known check/use gap was reproduced. A future fix must
 * change the expected outcome to preservation and convert this into a
 * positive security regression; never count this as production safety.
 */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "mover/mover.h"

#define CHECK(expr) do { \
    if (!(expr)) { \
        int saved_errno = errno; \
        fprintf(stderr, "FS-01B CHECK FAILED at %s:%d: %s (errno=%d)\n", \
                __FILE__, __LINE__, #expr, saved_errno); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static int probe_calls;

static void make_path(char dst[PATH_MAX], const char *parent, const char *name)
{
    int n = snprintf(dst, PATH_MAX, "%s/%s", parent, name);
    CHECK(n > 0 && n < PATH_MAX);
}

static void write_fixture(const char *path, const char *payload)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    CHECK(fd >= 0);
    size_t length = strlen(payload), offset = 0;
    while (offset < length) {
        ssize_t n = write(fd, payload + offset, length - offset);
        CHECK(n > 0);
        offset += (size_t)n;
    }
    CHECK(fsync(fd) == 0);
    CHECK(close(fd) == 0);
}

static void verify_payload(const char *path, const char *expected)
{
    char buffer[128];
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    CHECK(fd >= 0);
    ssize_t length = read(fd, buffer, sizeof(buffer));
    CHECK(length == (ssize_t)strlen(expected));
    CHECK(memcmp(buffer, expected, (size_t)length) == 0);
    CHECK(read(fd, buffer, sizeof(buffer)) == 0);
    CHECK(close(fd) == 0);
}

/* Runs immediately after the mover's identity check but before unlinkat.
 * The caller-supplied directory FD and names refer ONLY to our private
 * source directory. This intentionally simulates a competing directory
 * writer, without probabilistic sleeps or scheduling races. */
static int replace_quarantine_entry(int source_dirfd, const char *quarantine_name)
{
    probe_calls++;
    if (renameat(source_dirfd, quarantine_name,
                 source_dirfd, "retained-original.bin") != 0)
        return -1;
    if (renameat(source_dirfd, "replacement.bin",
                 source_dirfd, quarantine_name) != 0)
        return -1;
    return 0;
}

int main(void)
{
    char root_template[] = "/tmp/fc-fs01b-XXXXXX";
    char *root = mkdtemp(root_template);
    CHECK(root != NULL);

    char source_dir[PATH_MAX], output_dir[PATH_MAX];
    char victim[PATH_MAX], destination[PATH_MAX], replacement[PATH_MAX];
    char backup[PATH_MAX], retained[PATH_MAX];
    make_path(source_dir, root, "source");
    make_path(output_dir, root, "output");
    CHECK(mkdir(source_dir, 0700) == 0);
    CHECK(mkdir(output_dir, 0700) == 0);

    make_path(victim, source_dir, "victim.bin");
    make_path(destination, output_dir, "victim.bin");
    make_path(replacement, source_dir, "replacement.bin");
    make_path(backup, source_dir, "replacement-backup.bin");
    make_path(retained, source_dir, "retained-original.bin");

    write_fixture(victim, "original-safe-payload");
    write_fixture(replacement, "unrelated-replacement-payload");
    CHECK(link(replacement, backup) == 0);
    struct stat before;
    CHECK(stat(backup, &before) == 0);
    CHECK(before.st_nlink == 2);

    MoveResult result;
    memset(&result, 0, sizeof(result));
    fc_test_mover_set_retire_probe(replace_quarantine_entry);
    int move_rc = move_file_journaled(victim, output_dir, NULL, &result);
    fc_test_mover_set_retire_probe(NULL);

    CHECK(probe_calls == 1);
    CHECK(move_rc == 0);
    CHECK(result.error_code == 0);
    CHECK(strcmp(result.destination, destination) == 0);

    /* Important: these are both intact, so the fixture has lost no bytes. */
    verify_payload(destination, "original-safe-payload");
    verify_payload(retained, "original-safe-payload");
    verify_payload(backup, "unrelated-replacement-payload");

    /* Known UNSAFE result: mover reported success and unlinked an entry
     * which had been substituted after its prior identity verification.
     * We preserve the replacement's inode through the backup hard link.
     * This is evidence of the gap, not an acceptable product contract. */
    struct stat after;
    CHECK(stat(backup, &after) == 0);
    CHECK(after.st_dev == before.st_dev && after.st_ino == before.st_ino);
    CHECK(after.st_nlink == 1);
    CHECK(access(replacement, F_OK) == -1 && errno == ENOENT);
    CHECK(access(victim, F_OK) == -1 && errno == ENOENT);

    CHECK(unlink(destination) == 0);
    CHECK(unlink(retained) == 0);
    CHECK(unlink(backup) == 0);
    CHECK(rmdir(source_dir) == 0);
    CHECK(rmdir(output_dir) == 0);
    CHECK(rmdir(root) == 0);

    puts("FS-01B KNOWN-RISK CHARACTERIZATION: unchecked post-fstatat entry swap was unlinked;");
    puts("the test retained a hard-link backup; release safety is BLOCKED pending a separate fix.");
    return EXIT_SUCCESS;
}
