#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "journal/fcj3_stage.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <linux/fs.h>
#include <linux/magic.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/vfs.h>
#include <unistd.h>

#ifndef O_TMPFILE
#define O_TMPFILE (020000000 | O_DIRECTORY)
#endif

#ifndef AT_SYMLINK_FOLLOW
#define AT_SYMLINK_FOLLOW 0x400
#endif

#ifndef RENAME_NOREPLACE
#define RENAME_NOREPLACE (1U << 0)
#endif

static int valid_component(const char *component, const char *prefix)
{
    if (component == NULL || prefix == NULL)
    {
        errno = EINVAL;
        return 0;
    }

    size_t length = strnlen(component, 256U);
    size_t prefix_length = strlen(prefix);
    if (length <= prefix_length || length >= 256U ||
        strcmp(component, ".") == 0 || strcmp(component, "..") == 0 ||
        strncmp(component, prefix, prefix_length) != 0)
    {
        errno = EINVAL;
        return 0;
    }

    for (size_t i = 0U; i < length; ++i)
    {
        unsigned char byte = (unsigned char)component[i];
        if (byte == '/' || byte < 0x20U || byte == 0x7fU)
        {
            errno = EINVAL;
            return 0;
        }
    }
    return 1;
}

static int verify_procfs(void)
{
    int fd = open("/proc/self/fd", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0)
        return -1;

    struct statfs filesystem;
    if (fstatfs(fd, &filesystem) != 0)
    {
        int saved_errno = errno;
        (void)close(fd);
        errno = saved_errno;
        return -1;
    }

    if ((unsigned long)filesystem.f_type != (unsigned long)PROC_SUPER_MAGIC)
    {
        (void)close(fd);
        errno = ENODEV;
        return -1;
    }
    return fd;
}

static int validate_directory(int directory_fd, uid_t expected_owner,
                              struct stat *metadata)
{
    if (fstat(directory_fd, metadata) != 0)
        return -1;
    if (!S_ISDIR(metadata->st_mode) || metadata->st_uid != expected_owner ||
        (metadata->st_mode & 0022) != 0)
    {
        errno = EPERM;
        return -1;
    }
    return 0;
}

static int validate_stage_directory(const Fcj3Stage *stage)
{
    struct stat current;
    if (fstat(stage->directory_fd, &current) != 0)
        return -1;
    if (!S_ISDIR(current.st_mode) || current.st_uid != stage->expected_owner ||
        (current.st_mode & 0022) != 0 ||
        current.st_dev != stage->directory_device ||
        current.st_ino != stage->directory_inode)
    {
        errno = ESTALE;
        return -1;
    }
    return 0;
}

static int verify_named_inode(int directory_fd, const char *component,
                              dev_t expected_device, ino_t expected_inode)
{
    struct stat actual;
    if (fstatat(directory_fd, component, &actual, AT_SYMLINK_NOFOLLOW) != 0)
        return -1;
    if (!S_ISREG(actual.st_mode) || actual.st_dev != expected_device ||
        actual.st_ino != expected_inode)
    {
        errno = ESTALE;
        return -1;
    }
    return 0;
}

int fcj3_stage_open_at(Fcj3Stage *stage, int directory_fd,
                       const char *staging_component, uid_t expected_owner)
{
    if (stage == NULL || directory_fd < 0 ||
        !valid_component(staging_component, ".fc-tmp-"))
    {
        errno = EINVAL;
        return -1;
    }
    if (expected_owner != geteuid())
    {
        errno = EPERM;
        return -1;
    }

    memset(stage, 0, sizeof(*stage));
    stage->directory_fd = -1;
    stage->staging_fd = -1;
    stage->procfs_fd = -1;

    struct stat directory_metadata;
    if (validate_directory(directory_fd, expected_owner, &directory_metadata) != 0)
        return -1;

    int procfs_fd = verify_procfs();
    if (procfs_fd < 0)
        return -1;

    int staging_fd = openat(directory_fd, ".",
                            O_TMPFILE | O_RDWR | O_CLOEXEC, 0600);
    if (staging_fd < 0)
    {
        int saved_errno = errno;
        (void)close(procfs_fd);
        errno = saved_errno;
        return -1;
    }
    if (fchmod(staging_fd, 0600) != 0)
    {
        int saved_errno = errno;
        (void)close(staging_fd);
        (void)close(procfs_fd);
        errno = saved_errno;
        return -1;
    }

    struct stat staging_metadata;
    if (fstat(staging_fd, &staging_metadata) != 0)
    {
        int saved_errno = errno;
        (void)close(staging_fd);
        (void)close(procfs_fd);
        errno = saved_errno;
        return -1;
    }
    if (!S_ISREG(staging_metadata.st_mode) ||
        staging_metadata.st_uid != expected_owner ||
        (staging_metadata.st_mode & 0777) != 0600 ||
        staging_metadata.st_nlink != 0 ||
        staging_metadata.st_dev != directory_metadata.st_dev)
    {
        (void)close(staging_fd);
        (void)close(procfs_fd);
        errno = EPERM;
        return -1;
    }

    stage->directory_fd = directory_fd;
    stage->staging_fd = staging_fd;
    stage->procfs_fd = procfs_fd;
    stage->directory_device = directory_metadata.st_dev;
    stage->directory_inode = directory_metadata.st_ino;
    stage->staging_device = staging_metadata.st_dev;
    stage->staging_inode = staging_metadata.st_ino;
    stage->expected_owner = expected_owner;
    memcpy(stage->staging_component, staging_component,
           strlen(staging_component) + 1U);
    stage->initialized = 1;
    return 0;
}

int fcj3_stage_fd(const Fcj3Stage *stage)
{
    if (stage == NULL || !stage->initialized || stage->staging_fd < 0 ||
        stage->synced || stage->copied || stage->poisoned)
    {
        errno = EINVAL;
        return -1;
    }
    return stage->staging_fd;
}



static int same_timespec(const struct timespec *left,
                         const struct timespec *right)
{
    return left->tv_sec == right->tv_sec && left->tv_nsec == right->tv_nsec;
}

static int stage_matches_copy_snapshot(const Fcj3Stage *stage,
                                       const struct stat *metadata,
                                       nlink_t expected_links)
{
    return S_ISREG(metadata->st_mode) &&
           metadata->st_uid == stage->expected_owner &&
           (metadata->st_mode & 0777) == 0600 &&
           metadata->st_dev == stage->staging_device &&
           metadata->st_ino == stage->staging_inode &&
           metadata->st_nlink == expected_links &&
           metadata->st_size == stage->copied_size &&
           same_timespec(&metadata->st_mtim, &stage->copied_mtime) &&
           same_timespec(&metadata->st_ctim, &stage->copied_ctime);
}

static int same_source_snapshot(const struct stat *before,
                                const struct stat *after)
{
    return before->st_dev == after->st_dev &&
           before->st_ino == after->st_ino &&
           before->st_mode == after->st_mode &&
           before->st_uid == after->st_uid &&
           before->st_gid == after->st_gid &&
           before->st_size == after->st_size &&
           before->st_mtim.tv_sec == after->st_mtim.tv_sec &&
           before->st_mtim.tv_nsec == after->st_mtim.tv_nsec &&
           before->st_ctim.tv_sec == after->st_ctim.tv_sec &&
           before->st_ctim.tv_nsec == after->st_ctim.tv_nsec;
}

#ifdef FC_TESTING
static size_t test_fail_after_bytes = SIZE_MAX;
static size_t test_accepted_bytes;
static int test_write_error = EIO;
static int test_mutate_source_fd = -1;
static int test_mutate_stage_fd = -1;
static int test_mutate_before_publish_fd = -1;
static int test_fsync_error = 0;

void fcj3_stage_test_fail_write_after(size_t after_bytes, int error_code)
{
    test_fail_after_bytes = after_bytes;
    test_accepted_bytes = 0U;
    test_write_error = error_code != 0 ? error_code : EIO;
}

void fcj3_stage_test_grow_source_after_copy(int writable_source_fd)
{
    test_mutate_source_fd = writable_source_fd;
}

void fcj3_stage_test_mutate_stage_after_copy(int stage_fd)
{
    test_mutate_stage_fd = stage_fd;
}

void fcj3_stage_test_mutate_stage_before_publish(int stage_fd)
{
    test_mutate_before_publish_fd = stage_fd;
}

void fcj3_stage_test_fail_fsync(int error_code)
{
    test_fsync_error = error_code != 0 ? error_code : EIO;
}
#endif


static int stage_fsync(int fd)
{
#ifdef FC_TESTING
    if (test_fsync_error != 0)
    {
        int error_code = test_fsync_error;
        test_fsync_error = 0;
        errno = error_code;
        return -1;
    }
#endif
    return fsync(fd);
}

static ssize_t stage_pwrite(int fd, const void *buffer, size_t length,
                            off_t offset)
{
#ifdef FC_TESTING
    if (test_fail_after_bytes != SIZE_MAX)
    {
        if (test_accepted_bytes >= test_fail_after_bytes)
        {
            test_fail_after_bytes = SIZE_MAX;
            errno = test_write_error;
            return -1;
        }
        size_t remaining = test_fail_after_bytes - test_accepted_bytes;
        if (length > remaining)
            length = remaining;
    }
#endif
    ssize_t count = pwrite(fd, buffer, length, offset);
#ifdef FC_TESTING
    if (count > 0 && test_fail_after_bytes != SIZE_MAX)
        test_accepted_bytes += (size_t)count;
#endif
    return count;
}

int fcj3_stage_copy_from(Fcj3Stage *stage, int source_fd,
                         struct stat *source_snapshot, uint64_t *bytes_copied)
{
    if (stage == NULL || !stage->initialized || source_fd < 0 ||
        source_snapshot == NULL || bytes_copied == NULL ||
        stage->synced || stage->linked || stage->published ||
        stage->copied || stage->poisoned)
    {
        errno = EINVAL;
        return -1;
    }

    if (source_fd == stage->staging_fd)
    {
        errno = EINVAL;
        return -1;
    }

    int source_flags = fcntl(source_fd, F_GETFL);
    if (source_flags < 0)
        return -1;
    if ((source_flags & O_ACCMODE) == O_WRONLY
#ifdef O_PATH
        || (source_flags & O_PATH) == O_PATH
#endif
       )
    {
        errno = EBADF;
        return -1;
    }

    struct stat source_before;
    struct stat stage_before;
    if (fstat(source_fd, &source_before) != 0)
        return -1;
    if (!S_ISREG(source_before.st_mode) || source_before.st_size < 0)
    {
        errno = EINVAL;
        return -1;
    }
    if (fstat(stage->staging_fd, &stage_before) != 0)
        return -1;
    if (!S_ISREG(stage_before.st_mode) || stage_before.st_size != 0 ||
        (source_before.st_dev == stage_before.st_dev &&
         source_before.st_ino == stage_before.st_ino) ||
        stage_before.st_nlink != 0 ||
        stage_before.st_dev != stage->staging_device ||
        stage_before.st_ino != stage->staging_inode)
    {
        errno = ESTALE;
        stage->poisoned = 1;
        return -1;
    }

    unsigned char buffer[65536];
    off_t offset = 0;
    uint64_t copied = 0U;
    while (offset < source_before.st_size)
    {
        off_t remaining = source_before.st_size - offset;
        size_t request = remaining > (off_t)sizeof(buffer)
            ? sizeof(buffer) : (size_t)remaining;
        ssize_t read_count;
        do
        {
            read_count = pread(source_fd, buffer, request, offset);
        } while (read_count < 0 && errno == EINTR);
        if (read_count <= 0)
        {
            if (read_count == 0)
                errno = ESTALE;
            stage->poisoned = 1;
            return -1;
        }

        size_t written = 0U;
        while (written < (size_t)read_count)
        {
            ssize_t write_count = stage_pwrite(stage->staging_fd,
                buffer + written, (size_t)read_count - written,
                offset + (off_t)written);
            if (write_count < 0 && errno == EINTR)
                continue;
            if (write_count <= 0)
            {
                if (write_count == 0)
                    errno = EIO;
                stage->poisoned = 1;
                return -1;
            }
            written += (size_t)write_count;
        }
        offset += (off_t)read_count;
        copied += (uint64_t)read_count;
    }

#ifdef FC_TESTING
    if (test_mutate_source_fd >= 0)
    {
        int mutation_fd = test_mutate_source_fd;
        test_mutate_source_fd = -1;
        if (ftruncate(mutation_fd, source_before.st_size + 1) != 0)
        {
            stage->poisoned = 1;
            return -1;
        }
    }
#endif

    struct stat source_after;
    struct stat stage_after;
    if (fstat(source_fd, &source_after) != 0)
    {
        stage->poisoned = 1;
        return -1;
    }
    if (!same_source_snapshot(&source_before, &source_after))
    {
        errno = ESTALE;
        stage->poisoned = 1;
        return -1;
    }
    if (fstat(stage->staging_fd, &stage_after) != 0)
    {
        stage->poisoned = 1;
        return -1;
    }
    if (!S_ISREG(stage_after.st_mode) ||
        stage_after.st_uid != stage->expected_owner ||
        (stage_after.st_mode & 0777) != 0600 ||
        stage_after.st_dev != stage->staging_device ||
        stage_after.st_ino != stage->staging_inode ||
        stage_after.st_nlink != 0 ||
        stage_after.st_size != source_before.st_size)
    {
        errno = ESTALE;
        stage->poisoned = 1;
        return -1;
    }

    *source_snapshot = source_before;
    *bytes_copied = copied;
    stage->copied_size = stage_after.st_size;
    stage->copied_mtime = stage_after.st_mtim;
    stage->copied_ctime = stage_after.st_ctim;
    stage->copied = 1;
    return 0;
}

int fcj3_stage_sync(Fcj3Stage *stage, struct stat *staging_metadata)
{
    if (stage == NULL || !stage->initialized || staging_metadata == NULL ||
        stage->poisoned)
    {
        errno = EINVAL;
        return -1;
    }

    if (validate_stage_directory(stage) != 0)
    {
        stage->poisoned = 1;
        return -1;
    }

#ifdef FC_TESTING
    if (test_mutate_stage_fd >= 0)
    {
        int mutation_fd = test_mutate_stage_fd;
        test_mutate_stage_fd = -1;
        const unsigned char replacement = 'X';
        if (pwrite(mutation_fd, &replacement, 1U, 0) != 1)
        {
            if (errno == 0)
                errno = EIO;
            stage->poisoned = 1;
            return -1;
        }
    }
#endif

    if (stage_fsync(stage->staging_fd) != 0)
    {
        stage->poisoned = 1;
        return -1;
    }
    if (fstat(stage->staging_fd, staging_metadata) != 0)
    {
        stage->poisoned = 1;
        return -1;
    }

    if (!S_ISREG(staging_metadata->st_mode) ||
        staging_metadata->st_uid != stage->expected_owner ||
        (staging_metadata->st_mode & 0777) != 0600 ||
        staging_metadata->st_dev != stage->directory_device ||
        staging_metadata->st_ino != stage->staging_inode ||
        staging_metadata->st_nlink != 0 ||
        (stage->copied &&
         !stage_matches_copy_snapshot(stage, staging_metadata, 0)))
    {
        errno = ESTALE;
        stage->poisoned = 1;
        return -1;
    }
    if (stage->copied)
    {
        stage->copied_mtime = staging_metadata->st_mtim;
        stage->copied_ctime = staging_metadata->st_ctim;
    }
    stage->synced = 1;
    return 0;
}

int fcj3_stage_link(Fcj3Stage *stage)
{
    if (stage == NULL || !stage->initialized || !stage->synced ||
        stage->poisoned || stage->linked || stage->published)
    {
        errno = EINVAL;
        return -1;
    }

    if (validate_stage_directory(stage) != 0)
    {
        stage->poisoned = 1;
        return -1;
    }

    struct stat current;
    if (fstat(stage->staging_fd, &current) != 0)
    {
        stage->poisoned = 1;
        return -1;
    }
    if (!S_ISREG(current.st_mode) || current.st_uid != stage->expected_owner ||
        (current.st_mode & 0777) != 0600 || current.st_nlink != 0 ||
        current.st_dev != stage->staging_device ||
        current.st_ino != stage->staging_inode ||
        (stage->copied && !stage_matches_copy_snapshot(stage, &current, 0)))
    {
        errno = ESTALE;
        stage->poisoned = 1;
        return -1;
    }

    char proc_path[64];
    int length = snprintf(proc_path, sizeof(proc_path), "/proc/self/fd/%d",
                          stage->staging_fd);
    if (length < 0 || (size_t)length >= sizeof(proc_path))
    {
        errno = EOVERFLOW;
        return -1;
    }

    if (linkat(AT_FDCWD, proc_path, stage->directory_fd,
               stage->staging_component, AT_SYMLINK_FOLLOW) != 0)
    {
        stage->poisoned = 1;
        return -1;
    }

    /* From this point, never remove the name on an identity uncertainty. */
    stage->linked = 1;
    if (verify_named_inode(stage->directory_fd, stage->staging_component,
                           stage->staging_device, stage->staging_inode) != 0)
    {
        stage->poisoned = 1;
        return -1;
    }
    if (stage->copied)
    {
        struct stat linked_metadata;
        if (fstat(stage->staging_fd, &linked_metadata) != 0 ||
            !S_ISREG(linked_metadata.st_mode) ||
            linked_metadata.st_uid != stage->expected_owner ||
            (linked_metadata.st_mode & 0777) != 0600 ||
            linked_metadata.st_dev != stage->staging_device ||
            linked_metadata.st_ino != stage->staging_inode ||
            linked_metadata.st_nlink != 1 ||
            linked_metadata.st_size != stage->copied_size ||
            !same_timespec(&linked_metadata.st_mtim, &stage->copied_mtime))
        {
            errno = ESTALE;
            stage->poisoned = 1;
            return -1;
        }
        stage->copied_ctime = linked_metadata.st_ctim;
    }
    if (stage_fsync(stage->directory_fd) != 0)
    {
        stage->poisoned = 1;
        return -1;
    }

    return 0;
}

int fcj3_stage_publish(Fcj3Stage *stage, const char *final_component)
{
    if (stage == NULL || !stage->initialized || !stage->synced ||
        stage->poisoned || !stage->linked || stage->published ||
        !valid_component(final_component, ""))
    {
        errno = EINVAL;
        return -1;
    }

    if (validate_stage_directory(stage) != 0)
    {
        stage->poisoned = 1;
        return -1;
    }
#ifdef FC_TESTING
    if (test_mutate_before_publish_fd >= 0)
    {
        int mutation_fd = test_mutate_before_publish_fd;
        test_mutate_before_publish_fd = -1;
        const unsigned char replacement = 'Y';
        if (pwrite(mutation_fd, &replacement, 1U, 0) != 1)
        {
            if (errno == 0)
                errno = EIO;
            stage->poisoned = 1;
            return -1;
        }
    }
#endif
    if (verify_named_inode(stage->directory_fd, stage->staging_component,
                           stage->staging_device, stage->staging_inode) != 0)
    {
        stage->poisoned = 1;
        return -1;
    }
    if (stage->copied)
    {
        struct stat current;
        if (fstat(stage->staging_fd, &current) != 0 ||
            !stage_matches_copy_snapshot(stage, &current, 1))
        {
            errno = ESTALE;
            stage->poisoned = 1;
            return -1;
        }
    }

    if (syscall(SYS_renameat2, stage->directory_fd, stage->staging_component,
                stage->directory_fd, final_component, RENAME_NOREPLACE) != 0)
    {
        stage->poisoned = 1;
        return -1;
    }

    /* Rename succeeded: preserve the visible destination even if later
     * verification or directory durability is uncertain. Never retry.
     */
    stage->published = 1;
    stage->linked = 0;
    if (verify_named_inode(stage->directory_fd, final_component,
                           stage->staging_device, stage->staging_inode) != 0)
    {
        stage->poisoned = 1;
        return -1;
    }
    if (stage->copied)
    {
        struct stat published_metadata;
        if (fstat(stage->staging_fd, &published_metadata) != 0 ||
            !S_ISREG(published_metadata.st_mode) ||
            published_metadata.st_dev != stage->staging_device ||
            published_metadata.st_ino != stage->staging_inode ||
            published_metadata.st_size != stage->copied_size ||
            !same_timespec(&published_metadata.st_mtim, &stage->copied_mtime))
        {
            errno = ESTALE;
            stage->poisoned = 1;
            return -1;
        }
    }
    if (stage_fsync(stage->directory_fd) != 0)
    {
        stage->poisoned = 1;
        return -1;
    }
    return 0;
}

int fcj3_stage_close(Fcj3Stage *stage)
{
    if (stage == NULL || !stage->initialized)
    {
        errno = EINVAL;
        return -1;
    }

    int result = 0;
    int saved_errno = 0;
    if (close(stage->staging_fd) != 0)
    {
        result = -1;
        saved_errno = errno;
    }
    if (close(stage->procfs_fd) != 0 && result == 0)
    {
        result = -1;
        saved_errno = errno;
    }

    stage->staging_fd = -1;
    stage->procfs_fd = -1;
    stage->initialized = 0;
    if (result != 0)
        errno = saved_errno;
    return result;
}
