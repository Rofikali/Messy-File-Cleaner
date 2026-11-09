#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "journal/fcj3_stage.h"

#include <errno.h>
#include <fcntl.h>
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

    struct stat staging_metadata;
    if (fstat(staging_fd, &staging_metadata) != 0 ||
        !S_ISREG(staging_metadata.st_mode) ||
        staging_metadata.st_uid != expected_owner ||
        staging_metadata.st_nlink != 0 ||
        staging_metadata.st_dev != directory_metadata.st_dev)
    {
        int saved_errno = errno != 0 ? errno : EPERM;
        (void)close(staging_fd);
        (void)close(procfs_fd);
        errno = saved_errno;
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
    if (stage == NULL || !stage->initialized || stage->staging_fd < 0)
    {
        errno = EINVAL;
        return -1;
    }
    return stage->staging_fd;
}

int fcj3_stage_sync(Fcj3Stage *stage, struct stat *staging_metadata)
{
    if (stage == NULL || !stage->initialized || staging_metadata == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    if (validate_stage_directory(stage) != 0)
        return -1;

    if (fsync(stage->staging_fd) != 0)
        return -1;
    if (fstat(stage->staging_fd, staging_metadata) != 0)
        return -1;

    if (!S_ISREG(staging_metadata->st_mode) ||
        staging_metadata->st_uid != stage->expected_owner ||
        staging_metadata->st_dev != stage->directory_device ||
        staging_metadata->st_ino != stage->staging_inode ||
        staging_metadata->st_nlink != 0)
    {
        errno = ESTALE;
        return -1;
    }
    stage->synced = 1;
    return 0;
}

int fcj3_stage_link(Fcj3Stage *stage)
{
    if (stage == NULL || !stage->initialized || !stage->synced ||
        stage->linked || stage->published)
    {
        errno = EINVAL;
        return -1;
    }

    if (validate_stage_directory(stage) != 0)
        return -1;

    struct stat current;
    if (fstat(stage->staging_fd, &current) != 0)
        return -1;
    if (!S_ISREG(current.st_mode) || current.st_uid != stage->expected_owner ||
        current.st_nlink != 0 || current.st_dev != stage->staging_device ||
        current.st_ino != stage->staging_inode)
    {
        errno = ESTALE;
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
        return -1;

    /* From this point, never remove the name on an identity uncertainty. */
    stage->linked = 1;
    if (verify_named_inode(stage->directory_fd, stage->staging_component,
                           stage->staging_device, stage->staging_inode) != 0)
        return -1;
    if (fsync(stage->directory_fd) != 0)
        return -1;

    return 0;
}

int fcj3_stage_publish(Fcj3Stage *stage, const char *final_component)
{
    if (stage == NULL || !stage->initialized || !stage->synced ||
        !stage->linked || stage->published ||
        !valid_component(final_component, ""))
    {
        if (errno == 0)
            errno = EINVAL;
        return -1;
    }

    if (validate_stage_directory(stage) != 0)
        return -1;
    if (verify_named_inode(stage->directory_fd, stage->staging_component,
                           stage->staging_device, stage->staging_inode) != 0)
        return -1;

    if (syscall(SYS_renameat2, stage->directory_fd, stage->staging_component,
                stage->directory_fd, final_component, RENAME_NOREPLACE) != 0)
        return -1;

    /* The destination must still be the exact anonymous staging inode. */
    if (verify_named_inode(stage->directory_fd, final_component,
                           stage->staging_device, stage->staging_inode) != 0)
        return -1;
    if (fsync(stage->directory_fd) != 0)
        return -1;

    stage->published = 1;
    stage->linked = 0;
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
