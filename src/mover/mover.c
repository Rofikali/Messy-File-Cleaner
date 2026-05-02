// ./src/mover/mover.c

#define _GNU_SOURCE             // for O_DIRECTORY
#define _POSIX_C_SOURCE 200809L // for POSIX APIs

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

#include "mover/mover.h"
#include "utils/fs_utils.h"

// ── Fallbacks (portable safety) ───────────────────────────────
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

// ── extract_name ─────────────────────────────────────────────
static int extract_name(const char *path, char *name, size_t name_size)
{
    const char *slash = strrchr(path, '/');
    const char *fname = slash ? slash + 1 : path;

    int written = snprintf(name, name_size, "%s", fname);
    if (written < 0 || (size_t)written >= name_size)
    {
        fprintf(stderr, "Error: filename too long: %s\n", path);
        return -1;
    }
    return 0;
}

// ── split_name ───────────────────────────────────────────────
static int split_name(const char *filename,
                      char *base, size_t base_size,
                      char *ext, size_t ext_size)
{
    const char *dot = strrchr(filename, '.');

    if (!dot)
    {
        if (snprintf(base, base_size, "%s", filename) >= (int)base_size)
            return -1;
        ext[0] = '\0';
    }
    else
    {
        size_t base_len = (size_t)(dot - filename);
        if (base_len >= base_size)
            return -1;

        memcpy(base, filename, base_len);
        base[base_len] = '\0';

        if (snprintf(ext, ext_size, "%s", dot) >= (int)ext_size)
            return -1;
    }
    return 0;
}

// ── generate_unique_path ─────────────────────────────────────
static int generate_unique_path(const char *dir,
                                const char *filename,
                                char *out, size_t out_size)
{
    char base[256], ext[64];

    if (split_name(filename, base, sizeof(base), ext, sizeof(ext)) != 0)
    {
        fprintf(stderr, "Error: filename too long: %s\n", filename);
        return -1;
    }

    for (int i = 1; i < 100000; i++)
    {
        int written;

        if (i == 1)
            written = snprintf(out, out_size, "%s/%s%s", dir, base, ext);
        else
            written = snprintf(out, out_size, "%s/%s_%d%s", dir, base, i, ext);

        if (written < 0 || (size_t)written >= out_size)
        {
            fprintf(stderr, "Error: path too long\n");
            return -1;
        }

        int fd = open(out, O_CREAT | O_EXCL | O_WRONLY, 0644);
        if (fd >= 0)
            return fd;

        if (errno != EEXIST)
        {
            perror("open");
            return -1;
        }
    }

    fprintf(stderr, "Error: too many collisions\n");
    return -1;
}

// ── move_file ───────────────────────────────────────────────
int move_file(const char *src, const char *dest_dir)
{
    char filename[NAME_MAX + 1];

    if (extract_name(src, filename, sizeof(filename)) != 0)
        return -1;

    if (ensure_dir(dest_dir) != 0)
    {
        perror("ensure_dir");
        return -1;
    }

    char dest_path[PATH_MAX];

    int dest_fd = generate_unique_path(dest_dir, filename,
                                       dest_path, sizeof(dest_path));
    if (dest_fd < 0)
        return -1;

    close(dest_fd);

    // ── FAST PATH ─────────────────────────
    if (rename(src, dest_path) == 0)
        return 0;

    // ── CROSS-FS ─────────────────────────
    if (errno != EXDEV)
    {
        perror("rename");
        return -1;
    }

    char temp_path[PATH_MAX];

    int len = snprintf(temp_path, sizeof(temp_path), "%s.tmp", dest_path);
    if (len < 0 || (size_t)len >= sizeof(temp_path))
    {
        unlink(dest_path);
        return -1;
    }

    if (copy_file_safe(src, temp_path) != 0)
    {
        unlink(temp_path);
        unlink(dest_path);
        return -1;
    }

    if (rename(temp_path, dest_path) != 0)
    {
        unlink(temp_path);
        unlink(dest_path);
        return -1;
    }

    // fsync directory (durability)
    int dir_fd = open(dest_dir, O_RDONLY | O_DIRECTORY);
    if (dir_fd >= 0)
    {
        fsync(dir_fd);
        close(dir_fd);
    }

    if (unlink(src) != 0)
    {
        fprintf(stderr, "Warning: could not delete %s\n", src);
    }

    return 0;
}
