// ./src/utils/fs_utils.c
// src/utils/fs_utils.c

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/sendfile.h>

int file_exists(const char *path)
{
    return access(path, F_OK) == 0;
}

int ensure_dir(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0)
        return 0;
    return mkdir(path, 0755);
}

int copy_file_safe(const char *src, const char *dest)
{
    int src_fd;
    do
    {
        src_fd = open(src, O_RDONLY);
    } while (src_fd < 0 && errno == EINTR);

    if (src_fd < 0)
        return -1;

    int dest_fd;
    do
    {
        dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    } while (dest_fd < 0 && errno == EINTR);

    if (dest_fd < 0)
    {
        close(src_fd);
        return -1;
    }

    struct stat st;
    if (fstat(src_fd, &st) != 0)
    {
        close(src_fd);
        close(dest_fd);
        return -1;
    }

    off_t offset = 0;
    off_t remaining = st.st_size;

    while (remaining > 0)
    {
        ssize_t copied = sendfile(dest_fd, src_fd, &offset, (size_t)remaining);
        if (copied < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
                continue;

            close(src_fd);
            close(dest_fd);
            return -1;
        }

        if (copied == 0)
            break;

        remaining -= copied;
    }

    if (remaining != 0)
    {
        errno = EIO;
        close(src_fd);
        close(dest_fd);
        return -1;
    }

    int sync_rc;
    do
    {
        sync_rc = fsync(dest_fd);
    } while (sync_rc != 0 && errno == EINTR);

    close(src_fd);
    close(dest_fd);

    return sync_rc == 0 ? 0 : -1;
}

// int copy_file_safe(const char *src, const char *dest)
// {
//     int src_fd = open(src, O_RDONLY);
//     if (src_fd < 0)
//         return -1;

//     int dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
//     if (dest_fd < 0)
//     {
//         close(src_fd);
//         return -1;
//     }

//     char buf[8192];
//     ssize_t n;

//     while ((n = read(src_fd, buf, sizeof(buf))) > 0)
//     {
//         if (write(dest_fd, buf, n) != n)
//         {
//             close(src_fd);
//             close(dest_fd);
//             return -1;
//         }
//     }

//     fsync(dest_fd);

//     close(src_fd);
//     close(dest_fd);
//     return 0;
// }
