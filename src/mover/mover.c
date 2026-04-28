#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include "mover/mover.h"
#include "utils/fs_utils.h"

static void extract_name(const char *path, char *name)
{
    const char *slash = strrchr(path, '/');
    strcpy(name, slash ? slash + 1 : path);
}

static void split_name(const char *filename, char *base, char *ext)
{
    const char *dot = strrchr(filename, '.');
    if (!dot)
    {
        strcpy(base, filename);
        ext[0] = '\0';
    }
    else
    {
        strncpy(base, filename, dot - filename);
        base[dot - filename] = '\0';
        strcpy(ext, dot);
    }
}

static int generate_unique_path(const char *dir, const char *filename, char *out)
{
    char base[256], ext[64];
    split_name(filename, base, ext);

    for (int i = 1; i < 1000000; i++)
    {
        if (i == 1)
            snprintf(out, PATH_MAX, "%s/%s%s", dir, base, ext);
        else
            snprintf(out, PATH_MAX, "%s/%s_%d%s", dir, base, i, ext);

        if (!file_exists(out))
            return 0;
    }

    return -1;
}

int move_file(const char *src, const char *dest_dir)
{
    char filename[256];
    extract_name(src, filename);

    if (ensure_dir(dest_dir) != 0)
    {
        perror("mkdir");
        return -1;
    }

    char dest_path[PATH_MAX];
    if (generate_unique_path(dest_dir, filename, dest_path) != 0)
    {
        fprintf(stderr, "Failed to resolve name collision\n");
        return -1;
    }

    // 🔹 FAST PATH (same filesystem)
    if (rename(src, dest_path) == 0)
    {
        return 0;
    }

    // 🔹 CROSS-FS HANDLING
    if (errno == EXDEV)
    {
        char temp_path[PATH_MAX];
        // snprintf(temp_path, PATH_MAX, "%s.tmp", dest_path);
        if (strlen(dest_path) + 4 >= PATH_MAX)
        {
            fprintf(stderr, "Path too long\n");
            return -1;
        }

        snprintf(temp_path, PATH_MAX, "%s.tmp", dest_path);

        // Step 1: Copy to temp
        if (copy_file_safe(src, temp_path) != 0)
        {
            unlink(temp_path);
            return -1;
        }

        // Step 2: Atomic rename temp → final
        if (rename(temp_path, dest_path) != 0)
        {
            unlink(temp_path);
            return -1;
        }

        // Step 3: Delete source
        if (unlink(src) != 0)
        {
            fprintf(stderr, "Warning: failed to delete source\n");
            return -1;
        }

        return 0;
    }

    perror("rename");
    return -1;
}