#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "scanner/scanner.h"

static char found[8][PATH_MAX];
static size_t found_count;
static atomic_int callback_stop;

static int collect(const char *path)
{
    assert(found_count < 8);
    int n = snprintf(found[found_count], sizeof(found[found_count]), "%s", path);
    assert(n > 0 && (size_t)n < sizeof(found[found_count]));
    found_count++;
    return 0;
}

static int fail_after_one(const char *path)
{
    (void)path;
    found_count++;
    errno = ECANCELED;
    return -1;
}

static int fail_without_errno(const char *path)
{
    (void)path;
    found_count++;
    errno = 0;
    return -1;
}

static int cancel_after_one(const char *path)
{
    (void)path;
    found_count++;
    atomic_store(&callback_stop, SIGTERM);
    return 0;
}

static int fail_with_permission_error(const char *path)
{
    (void)path;
    found_count++;
    errno = EPERM;
    return -1;
}

static void write_file(const char *path)
{
    FILE *fp = fopen(path, "w");
    assert(fp != NULL);
    assert(fputs("fixture\n", fp) >= 0);
    assert(fclose(fp) == 0);
}

int main(void)
{
    char root_template[] = "/tmp/fc-scan-root-XXXXXX";
    char outside_template[] = "/tmp/fc-scan-outside-XXXXXX";
    char *root = mkdtemp(root_template);
    char *outside = mkdtemp(outside_template);
    assert(root != NULL && outside != NULL);

    char path[PATH_MAX], outside_file[PATH_MAX], outside_dir[PATH_MAX];
    int n = snprintf(path, sizeof(path), "%s/inside.txt", root);
    assert(n > 0 && (size_t)n < sizeof(path));
    write_file(path);

    n = snprintf(outside_file, sizeof(outside_file), "%s/secret.txt", outside);
    assert(n > 0 && (size_t)n < sizeof(outside_file));
    write_file(outside_file);

    n = snprintf(outside_dir, sizeof(outside_dir), "%s/external", outside);
    assert(n > 0 && (size_t)n < sizeof(outside_dir));
    assert(mkdir(outside_dir, 0700) == 0);
    char external_child[PATH_MAX];
    n = snprintf(external_child, sizeof(external_child), "%s/hidden.txt", outside_dir);
    assert(n > 0 && (size_t)n < sizeof(external_child));
    write_file(external_child);

    char nested[PATH_MAX];
    n = snprintf(nested, sizeof(nested), "%s", root);
    assert(n > 0 && (size_t)n < sizeof(nested));
    char component[181];
    memset(component, 'd', sizeof(component) - 1);
    component[sizeof(component) - 1] = '\0';

    for (int i = 0; i < 7; i++) {
        size_t used = strlen(nested);
        n = snprintf(nested + used, sizeof(nested) - used, "/%s", component);
        assert(n > 0 && (size_t)n < sizeof(nested) - used);
        assert(mkdir(nested, 0700) == 0);
    }

    char deep_file[PATH_MAX];
    n = snprintf(deep_file, sizeof(deep_file), "%s/deep.bin", nested);
    assert(n > 0 && (size_t)n < sizeof(deep_file));
    write_file(deep_file);

    /* A second real nested directory must still be traversed normally. */
    char sibling[PATH_MAX], sibling_file[PATH_MAX];
    n = snprintf(sibling, sizeof(sibling), "%s/second", root);
    assert(n > 0 && (size_t)n < sizeof(sibling));
    assert(mkdir(sibling, 0700) == 0);
    n = snprintf(sibling_file, sizeof(sibling_file), "%s/ok.txt", sibling);
    assert(n > 0 && (size_t)n < sizeof(sibling_file));
    write_file(sibling_file);

    char link_path[PATH_MAX];
    n = snprintf(link_path, sizeof(link_path), "%s/file-link", root);
    assert(n > 0 && (size_t)n < sizeof(link_path));
    assert(symlink(outside_file, link_path) == 0);

    n = snprintf(link_path, sizeof(link_path), "%s/dir-link", root);
    assert(n > 0 && (size_t)n < sizeof(link_path));
    assert(symlink(outside_dir, link_path) == 0);

    found_count = 0;
    assert(scan_directory(root, collect, NULL) == 0);
    assert(found_count == 3);
    assert(strlen(deep_file) > 1024);
    int saw_inside = 0, saw_deep = 0, saw_sibling = 0;
    for (size_t i = 0; i < found_count; ++i) {
        saw_inside += strcmp(found[i], path) == 0;
        saw_deep += strcmp(found[i], deep_file) == 0;
        saw_sibling += strcmp(found[i], sibling_file) == 0;
    }
    assert(saw_inside == 1 && saw_deep == 1 && saw_sibling == 1);

    char root_link[PATH_MAX];
    n = snprintf(root_link, sizeof(root_link), "%s/root-link", outside);
    assert(n > 0 && (size_t)n < sizeof(root_link));
    assert(symlink(root, root_link) == 0);
    found_count = 0;
    assert(scan_directory(root_link, collect, NULL) == -1);
    assert(found_count == 0);

    char suffixed_root[PATH_MAX];
    for (int separators = 1; separators <= 3; ++separators)
    {
        n = snprintf(suffixed_root, sizeof(suffixed_root), "%s%.*s",
                     root_link, separators, "///");
        assert(n > 0 && (size_t)n < sizeof(suffixed_root));
        found_count = 0;
        errno = 0;
        assert(scan_directory(suffixed_root, collect, NULL) == -1);
        assert(errno == ENOTDIR || errno == ELOOP);
        assert(found_count == 0);

        n = snprintf(suffixed_root, sizeof(suffixed_root), "%s%.*s",
                     root, separators, "///");
        assert(n > 0 && (size_t)n < sizeof(suffixed_root));
        assert(scan_directory(suffixed_root, collect, NULL) == 0);
        assert(found_count == 3);
        int inside = 0, deep = 0, second = 0;
        for (size_t i = 0; i < found_count; ++i)
        {
            inside += strcmp(found[i], path) == 0;
            deep += strcmp(found[i], deep_file) == 0;
            second += strcmp(found[i], sibling_file) == 0;
        }
        assert(inside == 1 && deep == 1 && second == 1);
    }

    found_count = 0;
    errno = 0;
    assert(scan_directory(root, fail_after_one, NULL) == -1);
    assert(errno == ECANCELED);
    assert(found_count == 1);

    /* A callback that fails without setting errno must report EIO, not success. */
    found_count = 0;
    errno = 0;
    assert(scan_directory(root, fail_without_errno, NULL) == -1);
    assert(errno == EIO);
    assert(found_count == 1);
    assert(access(path, F_OK) == 0);
    assert(access(deep_file, F_OK) == 0);

    /* Invalid inputs fail before any traversal or callback invocation. */
    found_count = 0;
    errno = 0;
    assert(scan_directory(NULL, collect, NULL) == -1);
    assert(errno == EINVAL);
    errno = 0;
    assert(scan_directory("", collect, NULL) == -1);
    assert(errno == EINVAL);
    errno = 0;
    assert(scan_directory(root, NULL, NULL) == -1);
    assert(errno == EINVAL);
    assert(found_count == 0);

    atomic_int stop_signal;
    atomic_init(&stop_signal, SIGTERM);
    found_count = 0;
    errno = 0;
    assert(scan_directory(root, collect, &stop_signal) == -1);
    assert(errno == ECANCELED);
    assert(found_count == 0);

    char missing[PATH_MAX];
    n = snprintf(missing, sizeof(missing), "%s/not-created", root);
    assert(n > 0 && (size_t)n < sizeof(missing));
    errno = 0;
    assert(scan_directory(missing, collect, &stop_signal) == -1);
    assert(errno == ECANCELED);
    assert(found_count == 0);
    /* Input validation still precedes cancellation. */
    errno = 0;
    assert(scan_directory(NULL, collect, &stop_signal) == -1 && errno == EINVAL);
    atomic_store(&stop_signal, 0);
    errno = 0;
    assert(scan_directory(missing, collect, &stop_signal) == -1 && errno == ENOENT);

    atomic_init(&callback_stop, 0);
    found_count = 0;
    errno = 0;
    assert(scan_directory(root, cancel_after_one, &callback_stop) == -1);
    assert(errno == ECANCELED);
    assert(found_count == 1);
    assert(access(path, F_OK) == 0 && access(deep_file, F_OK) == 0);

    /* Two regular files ensure the second callback is reached independent of
     * directory enumeration order after the first metadata lookup fails. */
    char errors_dir[PATH_MAX], error_a[PATH_MAX], error_b[PATH_MAX];
    n = snprintf(errors_dir, sizeof(errors_dir), "%s/errors", root);
    assert(n > 0 && (size_t)n < sizeof(errors_dir));
    assert(mkdir(errors_dir, 0700) == 0);
    n = snprintf(error_a, sizeof(error_a), "%s/a.txt", errors_dir);
    assert(n > 0 && (size_t)n < sizeof(error_a));
    n = snprintf(error_b, sizeof(error_b), "%s/b.txt", errors_dir);
    assert(n > 0 && (size_t)n < sizeof(error_b));
    write_file(error_a);
    write_file(error_b);

    found_count = 0;
    fc_test_fail_next_scanner_stat(EACCES);
    errno = 0;
    assert(scan_directory(errors_dir, fail_with_permission_error, NULL) == -1);
    assert(errno == EACCES && found_count == 1);

    found_count = 0;
    atomic_store(&callback_stop, 0);
    fc_test_fail_next_scanner_stat(EACCES);
    errno = 0;
    assert(scan_directory(errors_dir, cancel_after_one, &callback_stop) == -1);
    assert(errno == EACCES && found_count == 1);
    assert(atomic_load(&callback_stop) == SIGTERM);

    found_count = 0;
    fc_test_fail_next_scanner_stat(EACCES);
    errno = 0;
    assert(scan_directory(errors_dir, collect, NULL) == -1);
    assert(errno == EACCES && found_count == 1);
    assert(access(error_a, F_OK) == 0 && access(error_b, F_OK) == 0);
    assert(unlink(error_a) == 0 && unlink(error_b) == 0);
    assert(rmdir(errors_dir) == 0);

    /* Remove only the known disposable fixtures on successful completion. */
    assert(unlink(root_link) == 0);
    assert(unlink(link_path) == 0); /* dir-link */
    n = snprintf(link_path, sizeof(link_path), "%s/file-link", root);
    assert(n > 0 && (size_t)n < sizeof(link_path));
    assert(unlink(link_path) == 0);
    assert(unlink(path) == 0);
    assert(unlink(deep_file) == 0);
    for (int i = 0; i < 7; ++i)
    {
        assert(rmdir(nested) == 0);
        char *slash = strrchr(nested, '/');
        assert(slash != NULL);
        *slash = '\0';
    }
    assert(strcmp(nested, root) == 0);
    assert(unlink(sibling_file) == 0);
    assert(rmdir(sibling) == 0);
    assert(rmdir(root) == 0);
    assert(unlink(external_child) == 0);
    assert(rmdir(outside_dir) == 0);
    assert(unlink(outside_file) == 0);
    assert(rmdir(outside) == 0);

    puts("scanner symlink, long-path, and cancellation tests passed");
    return 0;
}
