#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/openat2.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

/* Test-only Linux UAPI characterization; no production fallback or moves. */
#ifdef SYS_openat2
static int confined(int fd, const char *path)
{
    struct open_how how = {
        .flags = O_RDONLY | O_DIRECTORY | O_CLOEXEC,
        .resolve = RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS | RESOLVE_NO_MAGICLINKS
    };
    return (int)syscall(SYS_openat2, fd, path, &how, sizeof how);
}

static int same_directory(int fd, const char *path)
{
    int a = openat(fd, path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (a < 0)
        return -1;
    int b = confined(fd, path);
    struct stat x, y;
    int ok = b >= 0 && fstat(a, &x) == 0 && fstat(b, &y) == 0 &&
             x.st_dev == y.st_dev && x.st_ino == y.st_ino;
    if (b >= 0 && close(b) != 0)
        ok = 0;
    if (close(a) != 0)
        ok = 0;
    return ok ? 0 : -1;
}

static int rejected(int fd, const char *path, int expected_errno)
{
    int candidate = confined(fd, path);
    if (candidate >= 0)
    {
        close(candidate);
        return -1;
    }
    return errno == expected_errno ? 0 : -1;
}

static int baseline_accepts(int fd, const char *path)
{
    int result = openat(fd, path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    return result < 0 ? -1 : close(result);
}

static int same_identity(const struct stat *a, const struct stat *b)
{
    return a->st_dev == b->st_dev && a->st_ino == b->st_ino;
}

/* Deterministic rename/replacement of our own fixture directories only.
 * Confinement is relative to the held directory object, not an immutable name. */
static int identity_fixture(int base)
{
    int held = -1, child = -1, reopened = -1, anchored = -1, fresh = -1;
    int original_exists = 0, moved = 0, original_child = 0;
    int replacement = 0, replacement_child = 0, result = -1;
    struct stat initial, initial_child, current, current_child, new_dir, new_child;
    if (mkdirat(base, "identity", 0700) != 0)
        goto cleanup;
    original_exists = 1;
    held = confined(base, "identity");
    if (held < 0 || fstat(held, &initial) != 0)
        goto cleanup;
    if (mkdirat(held, "child", 0700) != 0)
        goto cleanup;
    original_child = 1;
    child = confined(held, "child");
    if (child < 0 || fstat(child, &initial_child) != 0)
        goto cleanup;

    if (renameat(base, "identity", base, "relocated") != 0)
        goto cleanup;
    moved = 1;
    if (rejected(base, "identity", ENOENT) != 0)
        goto cleanup;
    if (mkdirat(base, "identity", 0700) != 0)
        goto cleanup;
    replacement = 1;
    reopened = confined(base, "identity");
    if (reopened < 0 || fstat(reopened, &new_dir) != 0 ||
        same_identity(&initial, &new_dir))
        goto cleanup;
    if (mkdirat(reopened, "child", 0700) != 0)
        goto cleanup;
    replacement_child = 1;
    if (fstat(held, &current) != 0 || !same_identity(&initial, &current))
        goto cleanup;
    anchored = confined(held, "child");
    if (anchored < 0 || fstat(anchored, &current_child) != 0 ||
        !same_identity(&initial_child, &current_child))
        goto cleanup;
    fresh = confined(base, "identity/child");
    if (fresh < 0 || fstat(fresh, &new_child) != 0 ||
        same_identity(&initial_child, &new_child))
        goto cleanup;
    if (same_directory(base, "relocated/child") != 0)
        goto cleanup;
    int relocated = confined(base, "relocated/child");
    if (relocated < 0)
        goto cleanup;
    struct stat relocated_child;
    int identical = fstat(relocated, &relocated_child) == 0 &&
                    same_identity(&initial_child, &relocated_child);
    if (close(relocated) != 0 || !identical)
        goto cleanup;
    puts("openat2_identity_fixture=PASS; held descriptor and replaced name distinguished");
    result = 0;
cleanup:
    if (fresh >= 0 && close(fresh) != 0) result = -1;
    if (anchored >= 0 && close(anchored) != 0) result = -1;
    if (child >= 0 && close(child) != 0) result = -1;
    if (replacement_child && unlinkat(reopened, "child", AT_REMOVEDIR) != 0) result = -1;
    if (reopened >= 0 && close(reopened) != 0) result = -1;
    if (replacement && unlinkat(base, "identity", AT_REMOVEDIR) != 0) result = -1;
    if (original_child && unlinkat(held, "child", AT_REMOVEDIR) != 0) result = -1;
    if (held >= 0 && close(held) != 0) result = -1;
    if (original_exists && unlinkat(base, moved ? "relocated" : "identity",
                                     AT_REMOVEDIR) != 0) result = -1;
    return result;
}

static int run_fixture(void)
{
    char root[] = "/tmp/fc-path-eval-XXXXXX";
    int parent = -1, base = -1, result = 1;
    int made_base = 0, made_outside = 0, made_real = 0, made_child = 0;
    int made_alias = 0, made_escape = 0;
    if (!mkdtemp(root))
        return 1;
    parent = open(root, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (parent < 0)
        goto cleanup;
    if (mkdirat(parent, "base", 0700) != 0)
        goto cleanup;
    made_base = 1;
    if (mkdirat(parent, "outside", 0700) != 0)
        goto cleanup;
    made_outside = 1;
    base = openat(parent, "base", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (base < 0)
        goto cleanup;
    if (mkdirat(base, "real", 0700) != 0)
        goto cleanup;
    made_real = 1;
    if (mkdirat(base, "real/child", 0700) != 0)
        goto cleanup;
    made_child = 1;
    if (symlinkat("real", base, "alias") != 0)
        goto cleanup;
    made_alias = 1;
    if (symlinkat("../outside", base, "escape") != 0)
        goto cleanup;
    made_escape = 1;

    int probe = confined(base, ".");
    if (probe < 0)
    {
        if (errno == ENOSYS || errno == EPERM)
        {
            puts("openat2 unavailable or policy denied; capability NOT VERIFIED");
            result = 77;
        }
        goto cleanup;
    }
    if (close(probe) != 0)
        goto cleanup;
    /* Only private fixture paths, including intentional relative escape. */
    if (same_directory(base, "real/child") != 0 ||
        same_directory(base, "real/../real") != 0 ||
        same_directory(base, ".") != 0 ||
        baseline_accepts(base, "alias/child") != 0 ||
        baseline_accepts(base, "../outside") != 0 ||
        rejected(base, "alias/child", ELOOP) != 0 ||
        rejected(base, "alias", ELOOP) != 0 ||
        rejected(base, "escape", ELOOP) != 0 ||
        rejected(base, "../outside", EXDEV) != 0 ||
        rejected(base, root, EXDEV) != 0 ||
        rejected(base, "missing", ENOENT) != 0)
        goto cleanup;
    puts("openat2_path_cases=11; private fixture, no production change");
    if (identity_fixture(base) != 0)
        goto cleanup;
    result = 0;
cleanup:
    if (made_escape && unlinkat(base, "escape", 0) != 0) result = 1;
    if (made_alias && unlinkat(base, "alias", 0) != 0) result = 1;
    if (made_child && unlinkat(base, "real/child", AT_REMOVEDIR) != 0) result = 1;
    if (made_real && unlinkat(base, "real", AT_REMOVEDIR) != 0) result = 1;
    if (base >= 0 && close(base) != 0) result = 1;
    if (made_outside && unlinkat(parent, "outside", AT_REMOVEDIR) != 0) result = 1;
    if (made_base && unlinkat(parent, "base", AT_REMOVEDIR) != 0) result = 1;
    if (parent >= 0 && close(parent) != 0) result = 1;
    if (rmdir(root) != 0) result = 1;
    return result;
}
#endif

int main(void)
{
#ifdef SYS_openat2
    return run_fixture();
#else
    puts("SYS_openat2 unavailable in build headers; capability NOT VERIFIED");
    return 77;
#endif
}
