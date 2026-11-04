#define _POSIX_C_SOURCE 200809L

#include "utils/fs_utils.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

int file_exists(const char *path)
{
    struct stat st;
    return path && lstat(path, &st) == 0;
}

int ensure_dir(const char *path)
{
    if (!path || !path[0])
    {
        errno = EINVAL;
        return -1;
    }

    struct stat st;
    if (lstat(path, &st) == 0)
    {
        if (S_ISDIR(st.st_mode))
            return 0;
        errno = ENOTDIR;
        return -1;
    }

    if (errno != ENOENT)
        return -1;

    if (mkdir(path, 0755) == 0)
        return 0;

    if (errno != EEXIST)
        return -1;

    if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode))
        return 0;

    errno = ENOTDIR;
    return -1;
}

int copy_file_safe(const char *src, const char *dest)
{
    if (!src || !dest)
    {
        errno = EINVAL;
        return -1;
    }

    int source_fd;
    do
    {
        source_fd = open(src, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    } while (source_fd < 0 && errno == EINTR);
    if (source_fd < 0)
        return -1;

    struct stat source_stat;
    if (fstat(source_fd, &source_stat) != 0)
    {
        int saved_errno = errno;
        close(source_fd);
        errno = saved_errno;
        return -1;
    }
    if (!S_ISREG(source_stat.st_mode))
    {
        int saved_errno = EINVAL;
        close(source_fd);
        errno = saved_errno;
        return -1;
    }

    int destination_fd;
    do
    {
        destination_fd = open(dest,
                              O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
                              0600);
    } while (destination_fd < 0 && errno == EINTR);
    if (destination_fd < 0)
    {
        int saved_errno = errno;
        close(source_fd);
        errno = saved_errno;
        return -1;
    }

    char buffer[64 * 1024];
    int status = 0;
    for (;;)
    {
        ssize_t count = read(source_fd, buffer, sizeof(buffer));
        if (count < 0)
        {
            if (errno == EINTR)
                continue;
            status = -1;
            break;
        }
        if (count == 0)
            break;

        ssize_t offset = 0;
        while (offset < count)
        {
            ssize_t written = write(destination_fd, buffer + offset,
                                    (size_t)(count - offset));
            if (written < 0 && errno == EINTR)
                continue;
            if (written <= 0)
            {
                if (written == 0)
                    errno = EIO;
                status = -1;
                break;
            }
            offset += written;
        }
        if (status != 0)
            break;
    }

    if (status == 0 && fchmod(destination_fd, source_stat.st_mode & 0777) != 0)
        status = -1;

    if (status == 0)
    {
        int rc;
        do
        {
            rc = fsync(destination_fd);
        } while (rc != 0 && errno == EINTR);
        if (rc != 0)
            status = -1;
    }

    int saved_errno = errno;
    if (close(destination_fd) != 0 && status == 0)
    {
        status = -1;
        saved_errno = errno;
    }
    if (close(source_fd) != 0 && status == 0)
    {
        status = -1;
        saved_errno = errno;
    }

    if (status != 0)
    {
        unlink(dest);
        errno = saved_errno;
        return -1;
    }

    return 0;
}
