// src/platform/filesystem_posix.c
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "cleaner/platform/filesystem.h"

bool cleaner_fs_exists(const char *path_utf8)
{
    return access(path_utf8, F_OK) == 0;
}

bool cleaner_fs_is_directory(const char *path_utf8)
{
    struct stat st;
    if (stat(path_utf8, &st) != 0)
        return false;

    return S_ISDIR(st.st_mode);
}

bool cleaner_fs_create_directory(const char *path_utf8)
{
    if (mkdir(path_utf8, 0755) == 0)
        return true;

    if (errno == EEXIST)
        return true;

    return false;
}

bool cleaner_fs_rename(const char *old_path_utf8,
                       const char *new_path_utf8)
{
    return rename(old_path_utf8, new_path_utf8) == 0;
}

bool cleaner_fs_get_modification_time(
    const char *path_utf8,
    int *year,
    int *month,
    int *day)
{
    struct stat st;
    if (stat(path_utf8, &st) != 0)
        return false;

    struct tm tm;
    if (!localtime_r(&st.st_mtime, &tm))
        return false;

    *year = tm.tm_year + 1900;
    *month = tm.tm_mon + 1;
    *day = tm.tm_mday;

    return true;
}