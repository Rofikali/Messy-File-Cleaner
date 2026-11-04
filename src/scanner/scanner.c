#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "scanner/scanner.h"

typedef struct
{
    DIR *directory;
    char *path;
} ScanFrame;

static char *join_path(const char *parent, const char *name)
{
    size_t parent_len = strlen(parent);
    size_t name_len = strlen(name);
    int needs_separator = parent_len > 0 && parent[parent_len - 1] != '/';

    if (name_len > SIZE_MAX - parent_len - (size_t)needs_separator - 1)
    {
        errno = EOVERFLOW;
        return NULL;
    }

    size_t total = parent_len + (size_t)needs_separator + name_len + 1;
    char *joined = malloc(total);
    if (!joined)
        return NULL;

    memcpy(joined, parent, parent_len);
    size_t offset = parent_len;
    if (needs_separator)
        joined[offset++] = '/';
    memcpy(joined + offset, name, name_len);
    joined[offset + name_len] = '\0';
    return joined;
}

static int push_frame(ScanFrame **frames,
                      size_t *count,
                      size_t *capacity,
                      DIR *directory,
                      char *path)
{
    if (*count == *capacity)
    {
        size_t next_capacity = *capacity == 0 ? 16 : *capacity * 2;
        if (next_capacity < *capacity ||
            next_capacity > SIZE_MAX / sizeof(**frames))
        {
            errno = EOVERFLOW;
            return -1;
        }

        ScanFrame *next = realloc(*frames, next_capacity * sizeof(**frames));
        if (!next)
            return -1;

        *frames = next;
        *capacity = next_capacity;
    }

    (*frames)[*count].directory = directory;
    (*frames)[*count].path = path;
    (*count)++;
    return 0;
}

int scan_directory(const char *path, event_handler_fn handler)
{
    if (!path || path[0] == '\0' || !handler)
    {
        errno = EINVAL;
        return -1;
    }

    int root_fd = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (root_fd < 0)
        return -1;

    DIR *root = fdopendir(root_fd);
    if (!root)
    {
        int saved_errno = errno;
        close(root_fd);
        errno = saved_errno;
        return -1;
    }

    char *root_path = strdup(path);
    if (!root_path)
    {
        int saved_errno = errno;
        closedir(root);
        errno = saved_errno;
        return -1;
    }

    ScanFrame *frames = NULL;
    size_t count = 0;
    size_t capacity = 0;
    int status = 0;

    if (push_frame(&frames, &count, &capacity, root, root_path) != 0)
    {
        int saved_errno = errno;
        closedir(root);
        free(root_path);
        errno = saved_errno;
        return -1;
    }

    while (count > 0)
    {
        ScanFrame *frame = &frames[count - 1];
        errno = 0;
        struct dirent *entry = readdir(frame->directory);

        if (!entry)
        {
            if (errno != 0)
                status = -1;

            if (closedir(frame->directory) != 0)
                status = -1;
            free(frame->path);
            count--;
            continue;
        }

        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        struct stat metadata;
        if (fstatat(dirfd(frame->directory), entry->d_name,
                    &metadata, AT_SYMLINK_NOFOLLOW) != 0)
        {
            if (errno != ENOENT)
                status = -1;
            continue;
        }

        /* Symlinks and special files are intentionally not processed. */
        if (S_ISLNK(metadata.st_mode) || !S_ISDIR(metadata.st_mode) &&
            !S_ISREG(metadata.st_mode))
            continue;

        char *child_path = join_path(frame->path, entry->d_name);
        if (!child_path)
        {
            status = -1;
            continue;
        }

        if (S_ISDIR(metadata.st_mode))
        {
            int child_fd = openat(dirfd(frame->directory), entry->d_name,
                                  O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
            if (child_fd < 0)
            {
                int saved_errno = errno;
                free(child_path);
                if (saved_errno != ENOENT && saved_errno != ELOOP)
                    status = -1;
                continue;
            }

            DIR *child = fdopendir(child_fd);
            if (!child)
            {
                int saved_errno = errno;
                close(child_fd);
                free(child_path);
                errno = saved_errno;
                status = -1;
                continue;
            }

            if (push_frame(&frames, &count, &capacity, child, child_path) != 0)
            {
                int saved_errno = errno;
                closedir(child);
                free(child_path);
                errno = saved_errno;
                status = -1;
            }
            continue;
        }

        handler(child_path);
        free(child_path);
    }

    free(frames);
    return status;
}
