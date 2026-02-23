
// cleaner/src/core/processor.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cleaner/platform/filesystem.h"
#include "cleaner/core/processor.h"
#include "cleaner/system/metrics.h"   // ← ADD THIS

/* ================================
   Global state
   ================================ */

static TimeMode g_mode = TIME_MODE_MODIFIED;
static char g_target[4096] = {0};
static int g_dry_run = 0;

/* ================================
   Configuration
   ================================ */

void processor_set_mode(TimeMode mode)
{
    g_mode = mode;
}

void processor_set_target(const char *target)
{
    strncpy(g_target, target, sizeof(g_target) - 1);
    g_target[sizeof(g_target) - 1] = '\0';
}

void processor_set_dry_run(int dry)
{
    g_dry_run = dry;
}

/* ================================
   Utilities
   ================================ */

// static const char *get_filename(const char *path)
// {
//     const char *slash1 = strrchr(path, '\\');
//     const char *slash2 = strrchr(path, '/');

//     const char *base = slash1 > slash2 ? slash1 : slash2;

//     return base ? base + 1 : path;
// }

/* ================================
   Core
   ================================ */

void processor_process_file(void *arg)
{
    char *path = (char *)arg;

    metrics_increment_processed();

    int year = 0, month = 0, day = 0;
    bool ok = false;

    if (g_mode == TIME_MODE_CREATION)
        ok = cleaner_fs_get_creation_time(path, &year, &month, &day);
    else
        ok = cleaner_fs_get_modified_time(path, &year, &month, &day);

    if (!ok)
    {
        metrics_increment_error();
        free(path);
        return;
    }

    const char *dot = strrchr(path, '.');
    if (!dot || dot == path)
    {
        free(path);
        return;
    }

    const char *ext = dot + 1;

    char dir[4096];

    int written = snprintf(dir, sizeof(dir),
                           "%s/%s_%d",
                           g_target,
                           ext,
                           year);

    if (written < 0 || written >= (int)sizeof(dir))
    {
        metrics_increment_error();
        free(path);
        return;
    }

    cleaner_fs_create_directory(dir);

    if (g_dry_run)
    {
        free(path);
        return;
    }

    const char *filename = strrchr(path, '\\');
    if (!filename)
        filename = strrchr(path, '/');

    filename = filename ? filename + 1 : path;

    char new_path[4096];

    written = snprintf(new_path, sizeof(new_path),
                       "%s/%s",
                       dir,
                       filename);

    if (written < 0 || written >= (int)sizeof(new_path))
    {
        metrics_increment_error();
        free(path);
        return;
    }

    if (cleaner_fs_rename(path, new_path))
        metrics_increment_moved();
    else
        metrics_increment_error();

    free(path);
}
