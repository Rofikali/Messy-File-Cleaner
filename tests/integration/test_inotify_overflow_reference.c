#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
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

int main(void)
{
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
    result = 1;
    if (root < 0)
        goto cleanup;
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
    struct stat value;
    if (observe(fd, wd, &overflow, &late_events) != 0 ||
        overflow == 0 || late_events != 0 ||
        fstatat(root, "late", &value, AT_SYMLINK_NOFOLLOW) != 0 ||
        !S_ISREG(value.st_mode) || value.st_size != 0 ||
        fstatat(root, "churn", &value, AT_SYMLINK_NOFOLLOW) != -1 ||
        errno != ENOENT)
        goto cleanup;
    printf("inotify_overflow limit=%u generated=%u overflow=%u late_events=%u late_present=1\n",
           limit, 2 * pairs + 1, overflow, late_events);
    result = 0;
cleanup:
    if (wd >= 0 && inotify_rm_watch(fd, wd) != 0)
        result = 1;
    if (fd >= 0 && close(fd) != 0)
        result = 1;
    if (churn && unlinkat(root, "churn", 0) != 0)
        result = 1;
    if (late && unlinkat(root, "late", 0) != 0)
        result = 1;
    if (root >= 0 && close(root) != 0)
        result = 1;
    if (rmdir(path) != 0)
        result = 1;
    if (result == 77)
        puts("NOT VERIFIED: inotify capability unavailable");
    return result;
}
