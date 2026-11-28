#define _GNU_SOURCE
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>

enum { EVENT_CAP = 32768, READ_CAP = EVENT_CAP + 16 };

static int queue_limit(unsigned int *limit)
{
    FILE *stream = fopen("/proc/sys/fs/inotify/max_queued_events", "r");
    if (!stream)
        return 77;
    char line[64];
    int result = 77;
    unsigned int parsed = 0;
    if (fgets(line, sizeof(line), stream) && strchr(line, '\n')) {
        char *end;
        errno = 0;
        unsigned long n = strtoul(line, &end, 10);
        if (!errno && end != line && *end == '\n' && end[1] == '\0' &&
            n > 0 && n <= EVENT_CAP) {
            parsed = (unsigned int)n;
            result = 0;
        }
    }
    if (ferror(stream))
        result = 1;
    if (fclose(stream) != 0)
        result = 1;
    if (result == 0)
        *limit = parsed;
    return result;
}

static int create_empty(int root, const char *name, int *owned)
{
    int fd = openat(root, name, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (fd < 0)
        return 1;
    *owned = 1;
    return close(fd) != 0;
}

static int observe(int fd, int wd, unsigned int *overflow, unsigned int *late)
{
    char buffer[8192];
    for (unsigned int reads = 0; reads < READ_CAP; ++reads) {
        ssize_t n = read(fd, buffer, sizeof(buffer));
        if (n < 0) {
            if (errno == EAGAIN)
                return 0;
            if (errno == EINTR)
                continue;
            return 1;
        }
        if (n == 0)
            return 1;
        size_t offset = 0;
        while (offset < (size_t)n) {
            struct inotify_event event;
            if ((size_t)n - offset < sizeof(event))
                return 1;
            memcpy(&event, buffer + offset, sizeof(event));
            offset += sizeof(event);
            if (event.len > (size_t)n - offset)
                return 1;
            if (event.mask == IN_Q_OVERFLOW) {
                if (event.wd != -1 || event.len != 0)
                    return 1;
                ++*overflow;
            } else {
                if (event.wd != wd || !event.len ||
                    (event.mask != IN_CREATE && event.mask != IN_DELETE))
                    return 1;
                const char *name = buffer + offset;
                if (!memchr(name, '\0', event.len))
                    return 1;
                if (strcmp(name, "late") == 0)
                    ++*late;
                else if (strcmp(name, "churn") != 0)
                    return 1;
            }
            offset += event.len;
        }
    }
    return 1;
}

static int same_regular(const struct stat *a, const struct stat *b)
{
    return S_ISREG(a->st_mode) && S_ISREG(b->st_mode) &&
           a->st_dev == b->st_dev && a->st_ino == b->st_ino &&
           a->st_size == 0 && b->st_size == 0;
}

enum { RESCAN_OK = 0, RESCAN_ERROR = 1, RESCAN_STALE = 2,
       RESCAN_UNEXPECTED = 3 };

struct interposition {
    int *late_owned;
    int displaced_owned;
    int ran;
};

/* Only the private test fixture is mutated; keep the original inode linked. */
static int replace_late(int root, struct interposition *hook)
{
    struct stat value;
    if (fstatat(root, "displaced", &value, AT_SYMLINK_NOFOLLOW) != -1 ||
        errno != ENOENT)
        return 1;
    if (renameat(root, "late", root, "displaced") != 0)
        return 1;
    *hook->late_owned = 0;
    hook->displaced_owned = 1;
    if (create_empty(root, "late", hook->late_owned) != 0)
        return 1;
    hook->ran = 1;
    return 0;
}

/* Fixed-fixture oracle, not a general or atomic snapshot API. */
static int rescan(int root, const struct stat *stable, const struct stat *late,
                  struct interposition *hook)
{
    int fd = openat(root, ".", O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0)
        return 1;
    DIR *dir = fdopendir(fd);
    if (!dir) {
        (void)close(fd);
        return 1;
    }
    unsigned int seen = 0, entries = 0, reads = 0;
    int result = 1;
    while (++reads <= 32) {
        errno = 0;
        struct dirent *entry = readdir(dir);
        if (!entry) {
            if (errno == 0 && seen == 15U && entries == 4U)
                result = 0;
            break;
        }
        const char *name = entry->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            continue;
        if (++entries > 16)
            break;
        if (hook && strcmp(name, "late") == 0 && !hook->ran &&
            replace_late(root, hook) != 0)
            break;
        struct stat value;
        if (fstatat(root, name, &value, AT_SYMLINK_NOFOLLOW) != 0)
            break;
        if ((strcmp(name, "stable") == 0 && !same_regular(&value, stable)) ||
            (strcmp(name, "late") == 0 && !same_regular(&value, late))) {
            result = RESCAN_STALE;
            break;
        }
        unsigned int bit = 0;
        if (strcmp(name, "stable") == 0 && same_regular(&value, stable))
            bit = 1;
        else if (strcmp(name, "late") == 0 && same_regular(&value, late))
            bit = 2;
        else if (strcmp(name, "alias") == 0 && S_ISLNK(value.st_mode))
            bit = 4;
        else if (strcmp(name, "subdir") == 0 && S_ISDIR(value.st_mode))
            bit = 8;
        if (bit == 0) {
            result = RESCAN_UNEXPECTED;
            break;
        }
        if ((seen & bit) != 0)
            break;
        seen |= bit;
    }
    if (closedir(dir) != 0)
        result = 1;
    return result;
}

static int queue_empty(int fd)
{
    char buffer[8192];
    for (unsigned int i = 0; i < 8; ++i) {
        ssize_t n = read(fd, buffer, sizeof(buffer));
        if (n < 0 && errno == EINTR)
            continue;
        return n < 0 && errno == EAGAIN ? 0 : 1;
    }
    return 1;
}

static int fresh_event(int fd, int wd)
{
    char buffer[8192];
    unsigned int seen = 0;
    for (unsigned int i = 0; i < 16; ++i) {
        ssize_t n = read(fd, buffer, sizeof(buffer));
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return errno == EAGAIN && seen == 1 ? 0 : 1;
        }
        if (n < (ssize_t)sizeof(struct inotify_event))
            return 1;
        size_t offset = 0;
        while (offset < (size_t)n) {
            struct inotify_event event;
            if ((size_t)n - offset < sizeof(event))
                return 1;
            memcpy(&event, buffer + offset, sizeof(event));
            offset += sizeof(event);
            if (!event.len || event.len > (size_t)n - offset ||
                event.wd != wd || event.mask != IN_CREATE)
                return 1;
            const char *name = buffer + offset;
            if (!memchr(name, '\0', event.len) || strcmp(name, "fresh") != 0 ||
                ++seen != 1)
                return 1;
            offset += event.len;
        }
    }
    return 1;
}

/* Single-threaded fixture; no claim about concurrent re-registration gaps. */
static int restart_reference(int root, const char *path, int *fd, int *wd,
                             int *churn, int *fresh, const struct stat *stable,
                             const struct stat *late, int gap_mode, int *gap_owned)
{
    if (create_empty(root, "churn", churn) || unlinkat(root, "churn", 0) != 0)
        return 1;
    *churn = 0;
    struct pollfd pending = { .fd = *fd, .events = POLLIN };
    if (poll(&pending, 1, 0) != 1 || pending.revents != POLLIN)
        return 1;
    int old = *fd;
    *fd = -1;
    *wd = -1;
    if (close(old) != 0)
        return 1;
    struct stat gap_identity = {0};
    if (gap_mode &&
        (create_empty(root, "gap", gap_owned) ||
         fstatat(root, "gap", &gap_identity, AT_SYMLINK_NOFOLLOW) != 0 ||
         !S_ISREG(gap_identity.st_mode) || gap_identity.st_size != 0))
        return 1;
    *fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (*fd < 0)
        return 1;
    *wd = inotify_add_watch(*fd, path, IN_CREATE | IN_DELETE | IN_ONLYDIR | IN_DONT_FOLLOW);
    if (*wd < 0 || queue_empty(*fd))
        return 1;
    int snapshot = rescan(root, stable, late, NULL);
    struct stat a, b, c;
    if (gap_mode) {
        if (snapshot != RESCAN_UNEXPECTED || queue_empty(*fd) ||
            fstatat(root, "gap", &c, AT_SYMLINK_NOFOLLOW) != 0 ||
            !same_regular(&c, &gap_identity) ||
            fstatat(root, "stable", &a, AT_SYMLINK_NOFOLLOW) != 0 ||
            !same_regular(&a, stable) ||
            fstatat(root, "late", &b, AT_SYMLINK_NOFOLLOW) != 0 ||
            !same_regular(&b, late))
            return 1;
        puts("inotify_gap new_queue_empty=1 gap_present=1 snapshot=INCOMPLETE reason=UNEXPECTED_MEMBERSHIP");
        return 0;
    }
    if (snapshot != RESCAN_OK || queue_empty(*fd) ||
        create_empty(root, "fresh", fresh) || fresh_event(*fd, *wd))
        return 1;
    if (fstatat(root, "stable", &a, AT_SYMLINK_NOFOLLOW) != 0 ||
        !same_regular(&a, stable) ||
        fstatat(root, "late", &b, AT_SYMLINK_NOFOLLOW) != 0 ||
        !same_regular(&b, late) ||
        fstatat(root, "fresh", &c, AT_SYMLINK_NOFOLLOW) != 0 ||
        !S_ISREG(c.st_mode) || c.st_size != 0)
        return 1;
    puts("inotify_restart old_pending=1 old_instance_closed=1 new_queue_initially_empty=1 snapshot_valid=1 fresh_events=1");
    return 0;
}

int main(int argc, char **argv)
{
    int invalidate = argc == 2 && strcmp(argv[1], "--invalidate") == 0;
    int gap_mode = argc == 2 && strcmp(argv[1], "--gap") == 0;
    int restart = gap_mode || (argc == 2 && strcmp(argv[1], "--restart") == 0);
    int reconcile = invalidate || restart || (argc == 2 && strcmp(argv[1], "--rescan") == 0);
    if (argc != 1 && !reconcile)
        return 1;
    unsigned int limit = 0;
    int result = queue_limit(&limit);
    if (result != 0) {
        puts("NOT VERIFIED: queue limit unavailable or outside bounded range");
        return result;
    }
    if (limit == 0 || limit > EVENT_CAP)
        return 1;
    char path[] = "/tmp/fc-inotify-overflow-XXXXXX";
    if (!mkdtemp(path))
        return 1;
    int root = open(path, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    int fd = -1, wd = -1, churn = 0, late = 0;
    int stable_owned = 0, alias_owned = 0, subdir_owned = 0, fresh_owned = 0;
    int gap_owned = 0;
    struct interposition hook = { .late_owned = &late };
    struct stat stable = {0};
    result = 1;
    if (root < 0)
        goto cleanup;
    if (reconcile) {
        if (create_empty(root, "stable", &stable_owned) ||
            fstatat(root, "stable", &stable, AT_SYMLINK_NOFOLLOW) != 0 ||
            !S_ISREG(stable.st_mode) || stable.st_size != 0)
            goto cleanup;
        if (symlinkat("stable", root, "alias") != 0)
            goto cleanup;
        alias_owned = 1;
        if (mkdirat(root, "subdir", 0700) != 0)
            goto cleanup;
        subdir_owned = 1;
    }
    fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (fd < 0) {
        if (errno == ENOSYS || errno == EPERM || errno == EACCES ||
            errno == EMFILE || errno == ENFILE)
            result = 77;
        goto cleanup;
    }
    wd = inotify_add_watch(fd, path, IN_CREATE | IN_DELETE | IN_ONLYDIR | IN_DONT_FOLLOW);
    if (wd < 0) {
        if (errno == ENOSPC || errno == EPERM || errno == EACCES)
            result = 77;
        goto cleanup;
    }
    unsigned int pairs = limit / 2 + 2;
    for (unsigned int i = 0; i < pairs; ++i) {
        if (create_empty(root, "churn", &churn) ||
            unlinkat(root, "churn", 0) != 0)
            goto cleanup;
        churn = 0;
    }
    if (create_empty(root, "late", &late))
        goto cleanup;
    unsigned int overflow = 0, late_events = 0;
    struct stat value, late_identity = {0};
    if (observe(fd, wd, &overflow, &late_events) != 0 ||
        overflow == 0 || late_events != 0 ||
        fstatat(root, "late", &late_identity, AT_SYMLINK_NOFOLLOW) != 0 ||
        !S_ISREG(late_identity.st_mode) || late_identity.st_size != 0 ||
        fstatat(root, "churn", &value, AT_SYMLINK_NOFOLLOW) != -1 ||
        errno != ENOENT)
        goto cleanup;
    printf("inotify_overflow limit=%u generated=%u overflow=%u late_events=%u late_present=1\n",
           limit, 2 * pairs + 1, overflow, late_events);
    if (reconcile) {
        puts("inotify_rescan event_state=INCOMPLETE");
        if (rescan(root, &stable, &late_identity, NULL) != RESCAN_OK)
            goto cleanup;
        if (invalidate) {
            if (rescan(root, &stable, &late_identity, &hook) != RESCAN_STALE ||
                !hook.ran || !hook.displaced_owned || !late)
                goto cleanup;
            struct stat original, replacement, unchanged;
            if (fstatat(root, "displaced", &original, AT_SYMLINK_NOFOLLOW) != 0 ||
                !same_regular(&original, &late_identity) ||
                fstatat(root, "late", &replacement, AT_SYMLINK_NOFOLLOW) != 0 ||
                !S_ISREG(replacement.st_mode) || replacement.st_size != 0 ||
                (replacement.st_dev == original.st_dev &&
                 replacement.st_ino == original.st_ino) ||
                fstatat(root, "stable", &unchanged, AT_SYMLINK_NOFOLLOW) != 0 ||
                !same_regular(&unchanged, &stable))
                goto cleanup;
            puts("inotify_rescan fixture_snapshot=INCOMPLETE reason=STALE_IDENTITY original_preserved=1 replacement_preserved=1");
        } else if (restart) {
            if (restart_reference(root, path, &fd, &wd, &churn, &fresh_owned,
                                  &stable, &late_identity, gap_mode, &gap_owned) != 0)
                goto cleanup;
        } else {
            puts("inotify_rescan fixture_snapshot=COMPLETE regular=2 excluded_symlink=1 excluded_directory=1");
        }
    }
    result = 0;
cleanup:
    if (wd >= 0 && inotify_rm_watch(fd, wd) != 0)
        result = 1;
    if (fd >= 0 && close(fd) != 0)
        result = 1;
    if (churn && unlinkat(root, "churn", 0) != 0)
        result = 1;
    if (gap_owned && unlinkat(root, "gap", 0) != 0)
        result = 1;
    if (fresh_owned && unlinkat(root, "fresh", 0) != 0)
        result = 1;
    if (late && unlinkat(root, "late", 0) != 0)
        result = 1;
    if (hook.displaced_owned && unlinkat(root, "displaced", 0) != 0)
        result = 1;
    if (subdir_owned && unlinkat(root, "subdir", AT_REMOVEDIR) != 0)
        result = 1;
    if (alias_owned && unlinkat(root, "alias", 0) != 0)
        result = 1;
    if (stable_owned && unlinkat(root, "stable", 0) != 0)
        result = 1;
    if (root >= 0 && close(root) != 0)
        result = 1;
    if (rmdir(path) != 0)
        result = 1;
    if (result == 77)
        puts("NOT VERIFIED: inotify capability unavailable");
    return result;
}
