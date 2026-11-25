#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "scanner/scanner.h"
#include "utils/root_path.h"

typedef struct
{
    DIR *directory;
    char *path;
} ScanFrame;

static void record_failure(int *status, int *saved_errno)
{
    if (*status == 0)
    {
        *status = -1;
        *saved_errno = errno ? errno : EIO;
    }
}

#ifdef FC_SCANNER_TESTING
/* Single-threaded test hook; absent from production builds. */
static int test_next_stat_error;

void fc_test_fail_next_scanner_stat(int error)
{
    test_next_stat_error = error;
}
#endif

static int inspect_entry(int fd, const char *name, struct stat *metadata)
{
#ifdef FC_SCANNER_TESTING
    if (test_next_stat_error != 0)
    {
        errno = test_next_stat_error;
        test_next_stat_error = 0;
        return -1;
    }
#endif
    return fstatat(fd, name, metadata, AT_SYMLINK_NOFOLLOW);
}

static char *join_path(const char *parent, const char *name)
{
    size_t parent_len = strlen(parent);
    size_t name_len = strlen(name);
    size_t separator_len =
        parent_len > 0 && parent[parent_len - 1] != '/' ? 1U : 0U;

    if (parent_len > SIZE_MAX - separator_len - 1 ||
        name_len > SIZE_MAX - parent_len - separator_len - 1)
    {
        errno = EOVERFLOW;
        return NULL;
    }

    size_t total_len = parent_len + separator_len + name_len + 1;
    char *path = malloc(total_len);
    if (!path)
        return NULL;

    memcpy(path, parent, parent_len);

    size_t offset = parent_len;
    if (separator_len != 0)
        path[offset++] = '/';

    memcpy(path + offset, name, name_len);
    path[offset + name_len] = '\0';
    return path;
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

        ScanFrame *next_frames =
            realloc(*frames, next_capacity * sizeof(**frames));
        if (!next_frames)
            return -1;

        *frames = next_frames;
        *capacity = next_capacity;
    }

    (*frames)[*count].directory = directory;
    (*frames)[*count].path = path;
    (*count)++;
    return 0;
}

static int open_child_directory(int parent_fd, const char *name)
{
    return openat(parent_fd,
                  name,
                  O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
}

int scan_directory(const char *path, event_handler_fn handler,
                   const atomic_int *stop_signal)
{
    if (!path || path[0] == '\0' || !handler)
    {
        errno = EINVAL;
        return -1;
    }

    /* An already-cancelled valid request must not access the filesystem. */
    if (stop_signal && atomic_load_explicit(stop_signal, memory_order_relaxed) != 0)
    {
        errno = ECANCELED;
        return -1;
    }

    char *root_path = fc_copy_root_path(path);
    if (!root_path)
        return -1;

    int root_fd = open(root_path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (root_fd < 0)
    {
        int saved_errno = errno;
        free(root_path);
        errno = saved_errno;
        return -1;
    }

    DIR *root_directory = fdopendir(root_fd);
    if (!root_directory)
    {
        int saved_errno = errno;
        close(root_fd);
        free(root_path);
        errno = saved_errno;
        return -1;
    }

    ScanFrame *frames = NULL;
    size_t frame_count = 0;
    size_t frame_capacity = 0;

    if (push_frame(&frames, &frame_count, &frame_capacity,
                   root_directory, root_path) != 0)
    {
        int saved_errno = errno;
        closedir(root_directory);
        free(root_path);
        errno = saved_errno;
        return -1;
    }

    int status = 0;
    int saved_errno = 0;

    while (frame_count > 0)
    {
        if (stop_signal && atomic_load_explicit(stop_signal, memory_order_relaxed) != 0)
        {
            errno = ECANCELED;
            record_failure(&status, &saved_errno);
            break;
        }

        ScanFrame *frame = &frames[frame_count - 1];

        errno = 0;
        struct dirent *entry = readdir(frame->directory);
        if (!entry)
        {
            if (errno != 0)
                record_failure(&status, &saved_errno);

            if (closedir(frame->directory) != 0)
                record_failure(&status, &saved_errno);

            free(frame->path);
            frame_count--;
            continue;
        }

        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        struct stat metadata;
        if (inspect_entry(dirfd(frame->directory), entry->d_name, &metadata) != 0)
        {
            /* A file disappearing during a scan is a normal race. */
            if (errno != ENOENT)
                record_failure(&status, &saved_errno);
            continue;
        }

        /* Process regular files and directories only. */
        if (S_ISLNK(metadata.st_mode) ||
            (!S_ISDIR(metadata.st_mode) && !S_ISREG(metadata.st_mode)))
            continue;

        char *child_path = join_path(frame->path, entry->d_name);
        if (!child_path)
        {
            record_failure(&status, &saved_errno);
            continue;
        }

        if (S_ISDIR(metadata.st_mode))
        {
            int child_fd = open_child_directory(dirfd(frame->directory),
                                                entry->d_name);
            if (child_fd < 0)
            {
                int open_errno = errno;
                free(child_path);

                /* The entry may have disappeared or become a symlink. */
                if (open_errno != ENOENT && open_errno != ELOOP)
                {
                    errno = open_errno;
                    record_failure(&status, &saved_errno);
                }
                continue;
            }

            /* The name may have been replaced between fstatat() and openat().
             * Only descend into the same directory object we inspected.
             * This is not a guarantee that later pathname consumers are safe. */
            struct stat opened_metadata;
            int stat_status = fstat(child_fd, &opened_metadata);
            if (stat_status != 0 ||
                !S_ISDIR(opened_metadata.st_mode) ||
                opened_metadata.st_dev != metadata.st_dev ||
                opened_metadata.st_ino != metadata.st_ino)
            {
                int identity_errno = stat_status != 0 ? errno : ESTALE;
                close(child_fd);
                free(child_path);
                errno = identity_errno ? identity_errno : ESTALE;
                record_failure(&status, &saved_errno);
                continue;
            }

            DIR *child_directory = fdopendir(child_fd);
            if (!child_directory)
            {
                int open_errno = errno;
                close(child_fd);
                free(child_path);
                errno = open_errno;
                record_failure(&status, &saved_errno);
                continue;
            }

            if (push_frame(&frames, &frame_count, &frame_capacity,
                           child_directory, child_path) != 0)
            {
                int push_errno = errno;
                closedir(child_directory);
                free(child_path);
                errno = push_errno;
                record_failure(&status, &saved_errno);
            }

            continue;
        }

        /* Recheck after discovery, immediately before handing off the path. */
        if (stop_signal && atomic_load_explicit(stop_signal, memory_order_relaxed) != 0)
        {
            free(child_path);
            errno = ECANCELED;
            record_failure(&status, &saved_errno);
            break;
        }

        errno = 0;
        int handler_status = handler(child_path);
        int handler_errno = errno;
        free(child_path);
        if (handler_status != 0)
        {
            errno = handler_errno ? handler_errno : EIO;
            record_failure(&status, &saved_errno);
            break;
        }
    }

    while (frame_count > 0)
    {
        ScanFrame *frame = &frames[frame_count - 1];
        if (closedir(frame->directory) != 0)
            record_failure(&status, &saved_errno);
        free(frame->path);
        frame_count--;
    }

    free(frames);

    if (status != 0)
        errno = saved_errno;

    return status;
}
