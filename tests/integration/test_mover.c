#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mover/mover.h"
#include "utils/fs_utils.h"

static void write_text(const char *path, const char *value)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    assert(fd >= 0);
    size_t length = strlen(value);
    size_t offset = 0;
    while (offset < length) {
        ssize_t n = write(fd, value + offset, length - offset);
        assert(n > 0);
        offset += (size_t)n;
    }
    assert(fsync(fd) == 0);
    assert(close(fd) == 0);
}

static void assert_text(const char *path, const char *expected)
{
    char actual[256];
    int fd = open(path, O_RDONLY | O_NOFOLLOW);
    if (fd < 0)
        fprintf(stderr, "assert_text: cannot open '%s': errno=%d (%s)\\n",
                path, errno, strerror(errno));
    assert(fd >= 0);
    ssize_t n = read(fd, actual, sizeof(actual) - 1);
    assert(n >= 0);
    actual[n] = '\0';
    assert(close(fd) == 0);
    assert(strcmp(actual, expected) == 0);
}

static void make_path(char output[PATH_MAX], const char *root, const char *name)
{
    int n = snprintf(output, PATH_MAX, "%s/%s", root, name);
    assert(n > 0 && n < PATH_MAX);
}

static void assert_no_private_destination_files(const char *directory)
{
    DIR *dir = opendir(directory);
    assert(dir != NULL);
    if (dir == NULL)
        return;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        assert(strncmp(entry->d_name, ".fc-tmp-", 8) != 0);
        assert(strncmp(entry->d_name, ".fc-retire-", 11) != 0);
    }
    int close_rc = closedir(dir);
    assert(close_rc == 0);
    (void)close_rc;
}

static void assert_path_missing(const char *path)
{
    int rc = access(path, F_OK);
    int saved_errno = errno;
    assert(rc != 0 && saved_errno == ENOENT);
    (void)rc;
}

static void test_copy_failure_preserves_source(const char *source_dir,
                                               const char *destination_dir)
{
    char source[PATH_MAX], destination[PATH_MAX];
    make_path(source, source_dir, "partial-failure.bin");
    make_path(destination, destination_dir, "partial-failure.bin");
    write_text(source, "0123456789abcdef");

    MoveResult result;
    int move_rc;
    fc_test_mover_set_write_fault(FC_TEST_WRITE_ENOSPC_AFTER_PARTIAL, 4);
    errno = 0;
    move_rc = move_file_journaled(source, destination_dir, NULL, &result);
    assert(move_rc == -1);
    (void)move_rc;
    assert(errno == ENOSPC);
    assert(result.error_code == ENOSPC);
    assert_text(source, "0123456789abcdef");
    assert_path_missing(destination);
    assert_no_private_destination_files(destination_dir);

    /* A partial write followed by EIO has the same source-preservation rule. */
    fc_test_mover_set_write_fault(FC_TEST_WRITE_EIO_AFTER_PARTIAL, 5);
    errno = 0;
    move_rc = move_file_journaled(source, destination_dir, NULL, &result);
    assert(move_rc == -1);
    (void)move_rc;
    assert(errno == EIO);
    assert(result.error_code == EIO);
    assert_text(source, "0123456789abcdef");
    assert_path_missing(destination);
    assert_no_private_destination_files(destination_dir);

    /* Permission failure at temp creation is deterministic, even as root. */
    fc_test_mover_set_temp_create_error(EACCES);
    errno = 0;
    move_rc = move_file_journaled(source, destination_dir, NULL, &result);
    assert(move_rc == -1);
    (void)move_rc;
    assert(errno == EACCES);
    assert(result.error_code == EACCES);
    assert_text(source, "0123456789abcdef");
    assert_path_missing(destination);
    assert_no_private_destination_files(destination_dir);

    fc_test_mover_set_write_fault(FC_TEST_WRITE_EINTR_ONCE, 0);
    move_rc = move_file_journaled(source, destination_dir, NULL, &result);
    assert(move_rc == 0);
    (void)move_rc;
    assert_text(destination, "0123456789abcdef");
    assert_path_missing(source);
    assert_no_private_destination_files(destination_dir);
    unlink(destination);
}

static void test_post_copy_failure_states(const char *source_dir,
                                              const char *destination_dir)
{
    char source[PATH_MAX], destination[PATH_MAX];
    MoveResult result;
    int move_rc;
    int saved_errno;

    /* Failure syncing the complete temporary file: source remains the only copy. */
    make_path(source, source_dir, "file-sync-failure.bin");
    make_path(destination, destination_dir, "file-sync-failure.bin");
    write_text(source, "durable source bytes");
    fc_test_mover_set_fsync_fault(1U, EIO);
    errno = 0;
    move_rc = move_file_journaled(source, destination_dir, NULL, &result);
    saved_errno = errno;
    assert(move_rc == -1 && saved_errno == EIO);
    assert_text(source, "durable source bytes");
    assert_path_missing(destination);
    assert_no_private_destination_files(destination_dir);

    /* Publication failure: no destination may appear and source is preserved. */
    make_path(source, source_dir, "publish-failure.bin");
    make_path(destination, destination_dir, "publish-failure.bin");
    write_text(source, "publication source bytes");
    fc_test_mover_set_publish_error(EACCES);
    errno = 0;
    move_rc = move_file_journaled(source, destination_dir, NULL, &result);
    saved_errno = errno;
    assert(move_rc == -1 && saved_errno == EACCES);
    assert_text(source, "publication source bytes");
    assert_path_missing(destination);
    assert_no_private_destination_files(destination_dir);

    /* Destination-directory fsync fails after publication: retain both copies. */
    make_path(source, source_dir, "destination-dir-sync-failure.bin");
    make_path(destination, destination_dir, "destination-dir-sync-failure.bin");
    write_text(source, "both copies must survive");
    fc_test_mover_set_fsync_fault(2U, EIO);
    errno = 0;
    move_rc = move_file_journaled(source, destination_dir, NULL, &result);
    saved_errno = errno;
    assert(move_rc == -1 && saved_errno == EIO);
    assert_text(source, "both copies must survive");
    assert_text(destination, "both copies must survive");
    assert_no_private_destination_files(destination_dir);

    /* Source-retirement failure occurs before quarantine rename: keep both copies. */
    make_path(source, source_dir, "retire-failure.bin");
    make_path(destination, destination_dir, "retire-failure.bin");
    write_text(source, "source retirement must not lose data");
    fc_test_mover_set_retire_error(EACCES);
    errno = 0;
    move_rc = move_file_journaled(source, destination_dir, NULL, &result);
    saved_errno = errno;
    assert(move_rc == -1 && saved_errno == EACCES);
    assert_text(source, "source retirement must not lose data");
    assert_text(destination, "source retirement must not lose data");
    assert_no_private_destination_files(destination_dir);
    assert_no_private_destination_files(source_dir);

    /* Source-directory fsync fails after unlink: destination remains complete.
     * The API reports the durability error; the published copy is not removed. */
    make_path(source, source_dir, "source-dir-sync-failure.bin");
    make_path(destination, destination_dir, "source-dir-sync-failure.bin");
    write_text(source, "published destination survives");
    fc_test_mover_set_fsync_fault(3U, EIO);
    errno = 0;
    move_rc = move_file_journaled(source, destination_dir, NULL, &result);
    saved_errno = errno;
    assert(move_rc == -1 && saved_errno == EIO);
    assert_path_missing(source);
    assert_text(destination, "published destination survives");
    assert_no_private_destination_files(destination_dir);
    assert_no_private_destination_files(source_dir);
    unlink(destination);

    /* Clean the retained source/destination pairs from earlier failure cases. */
    make_path(source, source_dir, "file-sync-failure.bin");
    unlink(source);
    make_path(source, source_dir, "publish-failure.bin");
    unlink(source);
    make_path(source, source_dir, "destination-dir-sync-failure.bin");
    unlink(source);
    make_path(destination, destination_dir, "destination-dir-sync-failure.bin");
    unlink(destination);
    make_path(source, source_dir, "retire-failure.bin");
    unlink(source);
    make_path(destination, destination_dir, "retire-failure.bin");
    unlink(destination);
}

int main(void)
{
    char root_template[] = "/tmp/fc-mover-XXXXXX";
    char *root = mkdtemp(root_template);
    assert(root != NULL);

    char source_dir[PATH_MAX], destination_dir[PATH_MAX], outside_dir[PATH_MAX];
    make_path(source_dir, root, "source");
    make_path(destination_dir, root, "destination");
    make_path(outside_dir, root, "outside");
    assert(mkdir(source_dir, 0700) == 0);
    assert(mkdir(destination_dir, 0700) == 0);
    assert(mkdir(outside_dir, 0700) == 0);

    /* A collision must preserve the pre-existing destination byte-for-byte. */
    char source[PATH_MAX], existing[PATH_MAX], expected_destination[PATH_MAX];
    make_path(source, source_dir, "report.txt");
    make_path(existing, destination_dir, "report.txt");
    make_path(expected_destination, destination_dir, "report_1.txt");
    write_text(source, "source contents");
    write_text(existing, "do not overwrite");

    MoveResult result;
    assert(move_file_journaled(source, destination_dir, NULL, &result) == 0);
    assert(access(source, F_OK) != 0 && errno == ENOENT);
    assert_text(existing, "do not overwrite");
    assert_text(expected_destination, "source contents");
    assert(strcmp(result.destination, expected_destination) == 0);

    /* Injected partial-write ENOSPC and one-shot EINTR are deterministic. */
    test_copy_failure_preserves_source(source_dir, destination_dir);
    test_post_copy_failure_states(source_dir, destination_dir);

    /* A symlink source is rejected; its target and link remain untouched. */
    char target[PATH_MAX], symlink_source[PATH_MAX];
    make_path(target, outside_dir, "important.txt");
    make_path(symlink_source, source_dir, "linked.txt");
    write_text(target, "outside data");
    assert(symlink(target, symlink_source) == 0);
    errno = 0;
    assert(move_file(symlink_source, destination_dir) == -1);
    assert(errno == ELOOP);
    assert(access(symlink_source, F_OK) == 0);
    assert_text(target, "outside data");

    /* Special files are rejected without blocking or being removed. */
    char fifo_source[PATH_MAX];
    make_path(fifo_source, source_dir, "pipe");
    assert(mkfifo(fifo_source, 0600) == 0);
    errno = 0;
    assert(move_file(fifo_source, destination_dir) == -1);
    assert(errno == EINVAL);
    struct stat fifo_stat;
    assert(lstat(fifo_source, &fifo_stat) == 0 && S_ISFIFO(fifo_stat.st_mode));

    /* A symlink used as the destination directory is rejected. */
    char destination_link[PATH_MAX];
    make_path(destination_link, root, "destination-link");
    assert(symlink(outside_dir, destination_link) == 0);
    char preserved_source[PATH_MAX];
    make_path(preserved_source, source_dir, "preserve.txt");
    write_text(preserved_source, "must remain");
    assert(move_file(preserved_source, destination_link) == -1);
    assert_text(preserved_source, "must remain");

    /* Symlink ancestors must not redirect source or destination traversal. */
    char source_ancestor_link[PATH_MAX], linked_source_path[PATH_MAX];
    make_path(source_ancestor_link, root, "source-ancestor-link");
    assert(symlink(outside_dir, source_ancestor_link) == 0);
    make_path(linked_source_path, source_ancestor_link, "important.txt");
    errno = 0;
    assert(move_file(linked_source_path, destination_dir) == -1);
    assert(errno == ENOTDIR || errno == ELOOP);
    assert_text(target, "outside data");

    char destination_ancestor_link[PATH_MAX], redirected_destination[PATH_MAX];
    make_path(destination_ancestor_link, root, "destination-ancestor-link");
    assert(symlink(outside_dir, destination_ancestor_link) == 0);
    make_path(redirected_destination, destination_ancestor_link, "nested");
    char ancestor_test_source[PATH_MAX];
    make_path(ancestor_test_source, source_dir, "ancestor-test.txt");
    write_text(ancestor_test_source, "source must survive");
    errno = 0;
    assert(move_file(ancestor_test_source, redirected_destination) == -1);
    assert(errno == ENOTDIR || errno == ELOOP);
    assert_text(ancestor_test_source, "source must survive");

    /* Dot-dot traversal is rejected rather than normalized implicitly. */
    errno = 0;
    assert(move_file(ancestor_test_source, "../outside") == -1);
    assert(errno == EINVAL);
    assert_text(ancestor_test_source, "source must survive");

    /* Missing destination components are created descriptor-relatively. */
    char nested_destination[PATH_MAX], nested_destination_file[PATH_MAX];
    make_path(nested_destination, root, "new-parent/new-child");
    make_path(nested_destination_file, nested_destination, "ancestor-test.txt");
    assert(move_file(ancestor_test_source, nested_destination) == 0);
    assert_text(nested_destination_file, "source must survive");

    /* The standalone copy helper must never truncate an existing file. */
    char copy_source[PATH_MAX], copy_destination[PATH_MAX];
    make_path(copy_source, source_dir, "copy-source.bin");
    make_path(copy_destination, destination_dir, "report.txt");
    write_text(copy_source, "copy payload");
    errno = 0;
    assert(copy_file_safe(copy_source, copy_destination) == -1);
    assert(errno == EEXIST);
    assert_text(copy_destination, "do not overwrite");

    unlink(destination_link);
    unlink(source_ancestor_link);
    unlink(destination_ancestor_link);
    unlink(fifo_source);
    unlink(symlink_source);
    unlink(target);
    unlink(copy_source);
    unlink(expected_destination);
    unlink(existing);
    unlink(preserved_source);
    unlink(nested_destination_file);
    rmdir(nested_destination);
    char nested_parent[PATH_MAX];
    make_path(nested_parent, root, "new-parent");
    rmdir(nested_parent);
    rmdir(source_dir);
    rmdir(destination_dir);
    rmdir(outside_dir);
    rmdir(root);

    puts("mover security tests passed");
    return 0;
}
