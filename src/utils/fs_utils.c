#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

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
    int src_fd = open(src, O_RDONLY);
    if (src_fd < 0)
        return -1;

    int dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0)
    {
        close(src_fd);
        return -1;
    }

    char buf[8192];
    ssize_t n;

    while ((n = read(src_fd, buf, sizeof(buf))) > 0)
    {
        if (write(dest_fd, buf, n) != n)
        {
            close(src_fd);
            close(dest_fd);
            return -1;
        }
    }

    fsync(dest_fd);

    close(src_fd);
    close(dest_fd);
    return 0;
}