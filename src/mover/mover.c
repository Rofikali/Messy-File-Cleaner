#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <linux/fs.h>

#include "mover/mover.h"

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#ifndef RENAME_NOREPLACE
#define RENAME_NOREPLACE (1U << 0)
#endif

/* Static-duration atomic objects are zero-initialized in C11 and later. */
static atomic_ulong temp_sequence;

#ifdef FC_TESTING
static atomic_int test_write_fault;
static atomic_long test_write_budget;
static atomic_int test_temp_create_error;
static atomic_uint test_fsync_count;
static atomic_uint test_fsync_fail_call;
static atomic_int test_fsync_error;
static atomic_int test_publish_error;
static atomic_int test_retire_error;

void fc_test_mover_set_write_fault(int fault, long bytes_before_failure)
{
    atomic_store_explicit(&test_write_budget, bytes_before_failure, memory_order_relaxed);
    atomic_store_explicit(&test_write_fault, fault, memory_order_release);
}

void fc_test_mover_set_temp_create_error(int error_code)
{
    atomic_store_explicit(&test_temp_create_error, error_code, memory_order_release);
}

void fc_test_mover_set_fsync_fault(unsigned int call_number, int error_code)
{
    atomic_store_explicit(&test_fsync_count, 0, memory_order_relaxed);
    atomic_store_explicit(&test_fsync_error, error_code, memory_order_relaxed);
    atomic_store_explicit(&test_fsync_fail_call, call_number, memory_order_release);
}

void fc_test_mover_set_publish_error(int error_code)
{
    atomic_store_explicit(&test_publish_error, error_code, memory_order_release);
}

void fc_test_mover_set_retire_error(int error_code)
{
    atomic_store_explicit(&test_retire_error, error_code, memory_order_release);
}

static int mover_fsync(int fd)
{
    unsigned int call_number =
        atomic_fetch_add_explicit(&test_fsync_count, 1, memory_order_relaxed) + 1U;
    unsigned int fail_call =
        atomic_load_explicit(&test_fsync_fail_call, memory_order_acquire);
    if (fail_call != 0U && call_number == fail_call) {
        atomic_store_explicit(&test_fsync_fail_call, 0U, memory_order_release);
        errno = atomic_load_explicit(&test_fsync_error, memory_order_relaxed);
        return -1;
    }
    return fsync(fd);
}

static int consume_injected_error(atomic_int *error_slot)
{
    return atomic_exchange_explicit(error_slot, 0, memory_order_acq_rel);
}

static ssize_t mover_write(int fd, const void *buffer, size_t count)
{
    int fault = atomic_load_explicit(&test_write_fault, memory_order_acquire);
    if (fault == FC_TEST_WRITE_EINTR_ONCE) {
        int expected = FC_TEST_WRITE_EINTR_ONCE;
        if (atomic_compare_exchange_strong_explicit(&test_write_fault, &expected,
                                                    FC_TEST_WRITE_NONE,
                                                    memory_order_acq_rel,
                                                    memory_order_relaxed)) {
            errno = EINTR;
            return -1;
        }
    } else if (fault == FC_TEST_WRITE_ENOSPC_AFTER_PARTIAL ||
               fault == FC_TEST_WRITE_EIO_AFTER_PARTIAL) {
        long budget = atomic_load_explicit(&test_write_budget, memory_order_relaxed);
        if (budget <= 0) {
            errno = fault == FC_TEST_WRITE_ENOSPC_AFTER_PARTIAL ? ENOSPC : EIO;
            return -1;
        }
        if ((size_t)budget < count)
            count = (size_t)budget;
        ssize_t written = write(fd, buffer, count);
        if (written > 0)
            atomic_fetch_sub_explicit(&test_write_budget, written, memory_order_relaxed);
        return written;
    }
    return write(fd, buffer, count);
}
static int mover_fsync(int fd)
{
    return fsync(fd);
}
#endif

#ifdef FC_TESTING
static int publish_temp_file(int directory_fd, const char *temp_name,
                             const char *destination_name)
{
    int injected_error = consume_injected_error(&test_publish_error);
    if (injected_error != 0) {
        errno = injected_error;
        return -1;
    }
    return rename_noreplace(directory_fd, temp_name,
                            directory_fd, destination_name);
}
#else
static int publish_temp_file(int directory_fd, const char *temp_name,
                             const char *destination_name)
{
    return rename_noreplace(directory_fd, temp_name,
                            directory_fd, destination_name);
}
#endif

static int rename_noreplace(int old_dirfd, const char *old_name,
                            int new_dirfd, const char *new_name)
{
#ifdef SYS_renameat2
    int rc;
    do
    {
        rc = (int)syscall(SYS_renameat2, old_dirfd, old_name,
                          new_dirfd, new_name, RENAME_NOREPLACE);
    } while (rc != 0 && errno == EINTR);
    return rc;
#else
    (void)old_dirfd;
    (void)old_name;
    (void)new_dirfd;
    (void)new_name;
    errno = ENOTSUP;
    return -1;
#endif
}

static int close_checked(int fd)
{
    /* Do not retry close(): on Linux the descriptor may already be released. */
    return close(fd);
}

static int split_path(const char *path, char **parent_out, char **name_out)
{
    if (!path || !path[0] || !parent_out || !name_out)
    {
        errno = EINVAL;
        return -1;
    }

    const char *slash = strrchr(path, '/');
    const char *name = slash ? slash + 1 : path;
    if (!name[0] || strcmp(name, ".") == 0 || strcmp(name, "..") == 0 ||
        strlen(name) > NAME_MAX)
    {
        errno = ENAMETOOLONG;
        return -1;
    }

    size_t parent_len = slash ? (size_t)(slash - path) : 1U;
    const char *parent = slash ? path : ".";
    if (slash && parent_len == 0)
    {
        parent = "/";
        parent_len = 1;
    }

    char *parent_copy = malloc(parent_len + 1);
    char *name_copy = strdup(name);
    if (!parent_copy || !name_copy)
    {
        free(parent_copy);
        free(name_copy);
        return -1;
    }

    memcpy(parent_copy, parent, parent_len);
    parent_copy[parent_len] = '\0';
    *parent_out = parent_copy;
    *name_out = name_copy;
    return 0;
}

/* Resolve each directory component relative to an already-open directory.
 * Symlink ancestors and dot traversal are rejected. Missing destination
 * components are created with mkdirat(), never path-based mkdir(). */
static int open_directory_path(const char *path, int create_missing)
{
    if (!path || !path[0] || strlen(path) >= PATH_MAX) {
        errno = EINVAL;
        return -1;
    }

    char copy[PATH_MAX];
    memcpy(copy, path, strlen(path) + 1);
    int absolute = copy[0] == '/';
    int fd;
    do {
        fd = open(absolute ? "/" : ".",
                  O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    } while (fd < 0 && errno == EINTR);
    if (fd < 0)
        return -1;

    if (strcmp(copy, ".") == 0 || strcmp(copy, "/") == 0)
        return fd;

    char *save = NULL;
    for (char *part = strtok_r(copy, "/", &save);
         part; part = strtok_r(NULL, "/", &save)) {
        if (strcmp(part, ".") == 0 || strcmp(part, "..") == 0) {
            close(fd);
            errno = EINVAL;
            return -1;
        }

        int next;
        do {
            next = openat(fd, part,
                          O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        } while (next < 0 && errno == EINTR);

        if (next < 0 && errno == ENOENT && create_missing) {
            if (mkdirat(fd, part, 0755) != 0 && errno != EEXIST) {
                int saved_errno = errno;
                close(fd);
                errno = saved_errno;
                return -1;
            }
            do {
                next = openat(fd, part,
                              O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
            } while (next < 0 && errno == EINTR);
        }
        if (next < 0) {
            int saved_errno = errno;
            close(fd);
            errno = saved_errno;
            return -1;
        }
        if (close(fd) != 0) {
            int saved_errno = errno;
            close(next);
            errno = saved_errno;
            return -1;
        }
        fd = next;
    }
    return fd;
}

static int build_candidate(const char *original, unsigned int index,
                           char output[NAME_MAX + 1])
{
    if (index == 0)
    {
        int n = snprintf(output, NAME_MAX + 1, "%s", original);
        if (n < 0 || n > NAME_MAX)
        {
            errno = ENAMETOOLONG;
            return -1;
        }
        return 0;
    }

    const char *dot = strrchr(original, '.');
    if (dot == original)
        dot = NULL;

    size_t base_len = dot ? (size_t)(dot - original) : strlen(original);
    const char *extension = dot ? dot : "";
    int n = snprintf(output, NAME_MAX + 1, "%.*s_%u%s",
                     (int)base_len, original, index, extension);
    if (n < 0 || n > NAME_MAX)
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    return 0;
}

static int create_temp_file(int directory_fd, char name[NAME_MAX + 1])
{
#ifdef FC_TESTING
    int injected_error = atomic_exchange_explicit(&test_temp_create_error, 0,
                                                   memory_order_acq_rel);
    if (injected_error != 0) {
        errno = injected_error;
        return -1;
    }
#endif
    for (unsigned int attempt = 0; attempt < 1000; attempt++)
    {
        unsigned long sequence = atomic_fetch_add_explicit(&temp_sequence, 1, memory_order_relaxed) + 1UL;
        int n = snprintf(name, NAME_MAX + 1, ".fc-tmp-%ld-%lu",
                         (long)getpid(), sequence);
        if (n < 0 || n > NAME_MAX)
        {
            errno = ENAMETOOLONG;
            return -1;
        }

        int fd = openat(directory_fd, name,
                        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
                        0600);
        if (fd >= 0)
            return fd;
        if (errno != EEXIST)
            return -1;
    }

    errno = EEXIST;
    return -1;
}

static int copy_fd(int source_fd, int destination_fd, const struct stat *initial)
{
    char buffer[128 * 1024];

    for (;;)
    {
        ssize_t count = read(source_fd, buffer, sizeof(buffer));
        if (count < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (count == 0)
            break;

        ssize_t offset = 0;
        while (offset < count)
        {
            ssize_t written = mover_write(destination_fd, buffer + offset,
                                    (size_t)(count - offset));
            if (written < 0)
            {
                if (errno == EINTR)
                    continue;
                return -1;
            }
            if (written == 0)
            {
                errno = EIO;
                return -1;
            }
            offset += written;
        }
    }

    struct stat after;
    if (fstat(source_fd, &after) != 0)
        return -1;

    if (after.st_dev != initial->st_dev || after.st_ino != initial->st_ino ||
        after.st_size != initial->st_size ||
        after.st_mtim.tv_sec != initial->st_mtim.tv_sec ||
        after.st_mtim.tv_nsec != initial->st_mtim.tv_nsec ||
        after.st_ctim.tv_sec != initial->st_ctim.tv_sec ||
        after.st_ctim.tv_nsec != initial->st_ctim.tv_nsec)
    {
        errno = EBUSY;
        return -1;
    }

    if (fchmod(destination_fd, initial->st_mode & 0777) != 0)
        return -1;

    int rc;
    do
    {
        rc = mover_fsync(destination_fd);
    } while (rc != 0 && errno == EINTR);
    return rc;
}

/*
 * Move only the inode opened and validated above. First rename the source
 * directory entry to a private, unique quarantine name, then verify that the
 * quarantined entry is still the opened inode. If it is not, restore it
 * without replacing anything. This prevents deleting a replacement that
 * raced with the scan/copy.
 */
static int retire_source(int source_dirfd, const char *source_name,
                         const struct stat *opened_stat)
{
#ifdef FC_TESTING
    int injected_error = consume_injected_error(&test_retire_error);
    if (injected_error != 0) {
        errno = injected_error;
        return -1;
    }
#endif
    char quarantine[NAME_MAX + 1];

    for (unsigned int attempt = 0; attempt < 1000; attempt++)
    {
        unsigned long sequence = atomic_fetch_add_explicit(&temp_sequence, 1, memory_order_relaxed) + 1UL;
        int n = snprintf(quarantine, sizeof(quarantine), ".fc-retire-%ld-%lu",
                         (long)getpid(), sequence);
        if (n < 0 || (size_t)n >= sizeof(quarantine))
        {
            errno = ENAMETOOLONG;
            return -1;
        }

        if (rename_noreplace(source_dirfd, source_name,
                             source_dirfd, quarantine) != 0)
        {
            if (errno == EEXIST)
                continue;
            return -1;
        }

        struct stat staged;
        if (fstatat(source_dirfd, quarantine, &staged, AT_SYMLINK_NOFOLLOW) != 0)
        {
            int saved_errno = errno;
            /* Leave the entry recoverable under its quarantine name. */
            errno = saved_errno;
            return -1;
        }

        if (staged.st_dev != opened_stat->st_dev ||
            staged.st_ino != opened_stat->st_ino ||
            !S_ISREG(staged.st_mode))
        {
            int saved_errno = EBUSY;
            if (rename_noreplace(source_dirfd, quarantine,
                                 source_dirfd, source_name) != 0)
                saved_errno = errno;
            errno = saved_errno;
            return -1;
        }

        if (unlinkat(source_dirfd, quarantine, 0) != 0)
            return -1;

        int rc;
        do
        {
            rc = mover_fsync(source_dirfd);
        } while (rc != 0 && errno == EINTR);
        return rc;
    }

    errno = EEXIST;
    return -1;
}

static int move_file_internal(const char *src, const char *dest_dir,
                              MoveJournal *journal, MoveResult *result)
{
    if (result)
    {
        result->destination[0] = '\0';
        result->error_code = 0;
    }

    if (!src || !dest_dir || !dest_dir[0])
    {
        errno = EINVAL;
        goto fail_without_resources;
    }

    char *source_parent = NULL;
    char *source_name = NULL;
    if (split_path(src, &source_parent, &source_name) != 0)
        goto fail_without_resources;

    int source_dirfd = open_directory_path(source_parent, 0);
    if (source_dirfd < 0)
        goto fail;

    int destination_dirfd = open_directory_path(dest_dir, 1);
    if (destination_dirfd < 0)
        goto fail_with_source_dir;

    int source_fd;
    do
    {
        source_fd = openat(source_dirfd, source_name,
                           O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    } while (source_fd < 0 && errno == EINTR);
    if (source_fd < 0)
        goto fail_with_both_dirs;

    struct stat source_stat;
    if (fstat(source_fd, &source_stat) != 0)
        goto fail_with_source_fd;
    if (!S_ISREG(source_stat.st_mode))
    {
        errno = EINVAL;
        goto fail_with_source_fd;
    }

    char temp_name[NAME_MAX + 1];
    int temp_fd = create_temp_file(destination_dirfd, temp_name);
    if (temp_fd < 0)
        goto fail_with_source_fd;

    if (copy_fd(source_fd, temp_fd, &source_stat) != 0)
        goto fail_with_temp;

    if (close_checked(temp_fd) != 0)
    {
        temp_fd = -1;
        goto fail_with_temp_closed;
    }
    temp_fd = -1;

    char destination_name[NAME_MAX + 1];
    char destination_path[PATH_MAX];
    int published = 0;
    for (unsigned int candidate = 0; candidate < 100000; candidate++)
    {
        if (build_candidate(source_name, candidate, destination_name) != 0)
            goto fail_with_temp_closed;

        int path_len = snprintf(destination_path, sizeof(destination_path),
                                "%s/%s", dest_dir, destination_name);
        if (path_len < 0 || (size_t)path_len >= sizeof(destination_path))
        {
            errno = ENAMETOOLONG;
            goto fail_with_temp_closed;
        }

        if (journal_record_started(journal, src, destination_path) != 0)
            goto fail_with_temp_closed;

        if (publish_temp_file(destination_dirfd, temp_name, destination_name) == 0)
        {
            published = 1;
            if (result)
                (void)snprintf(result->destination,
                               sizeof(result->destination), "%s", destination_path);
            break;
        }

        if (errno != EEXIST)
            goto fail_with_temp_closed;

        if (journal_record_failed(journal, src, destination_path, EEXIST) != 0)
            goto fail_with_temp_closed;
    }

    if (!published)
    {
        errno = EEXIST;
        goto fail_with_temp_closed;
    }

    int sync_rc;
    do
    {
        sync_rc = mover_fsync(destination_dirfd);
    } while (sync_rc != 0 && errno == EINTR);
    if (sync_rc != 0)
        goto fail_after_publish;

    if (journal_record_published(journal, src, destination_path) != 0)
        goto fail_after_publish;

    if (retire_source(source_dirfd, source_name, &source_stat) != 0)
        goto fail_after_publish;

    if (journal_record_completed(journal, src, destination_path) != 0)
        goto fail_after_publish;

    close_checked(source_fd);
    close_checked(destination_dirfd);
    close_checked(source_dirfd);
    free(source_parent);
    free(source_name);
    return 0;

fail_with_temp:
    {
        int saved_errno = errno;
        close_checked(temp_fd);
        unlinkat(destination_dirfd, temp_name, 0);
        errno = saved_errno;
    }
    goto fail_with_source_fd;

fail_with_temp_closed:
    {
        int saved_errno = errno;
        unlinkat(destination_dirfd, temp_name, 0);
        errno = saved_errno;
    }
    goto fail_with_source_fd;

fail_after_publish:
    {
        int saved_errno = errno;
        /* The published destination is intentionally retained; source remains
         * available unless retire_source already completed. */
        (void)journal_record_failed(journal, src,
                                    destination_path, saved_errno);
        errno = saved_errno;
    }
    goto fail_with_source_fd;

fail_with_source_fd:
    {
        int saved_errno = errno;
        close_checked(source_fd);
        errno = saved_errno;
    }
fail_with_both_dirs:
    {
        int saved_errno = errno;
        close_checked(destination_dirfd);
        errno = saved_errno;
    }
fail_with_source_dir:
    {
        int saved_errno = errno;
        close_checked(source_dirfd);
        errno = saved_errno;
    }
fail:
    {
        int saved_errno = errno;
        free(source_parent);
        free(source_name);
        errno = saved_errno;
    }
fail_without_resources:
    if (result)
        result->error_code = errno;
    return -1;
}

int move_file(const char *src, const char *dest_dir)
{
    return move_file_internal(src, dest_dir, NULL, NULL);
}

int move_file_journaled(const char *src, const char *dest_dir,
                        MoveJournal *journal, MoveResult *result)
{
    return move_file_internal(src, dest_dir, journal, result);
}
