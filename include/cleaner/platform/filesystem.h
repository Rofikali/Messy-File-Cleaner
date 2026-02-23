// include/cleaner/platform/filesystem.h

#ifndef CLEANER_PLATFORM_FILESYSTEM_H
#define CLEANER_PLATFORM_FILESYSTEM_H

#include <stdbool.h>
#include <stdint.h>

/* ========================= */
/* Filesystem Entry          */
/* ========================= */

typedef struct
{
    const char *path; /* UTF-8 full path */
    int is_directory; /* 1 = dir, 0 = file */
} cleaner_fs_entry;

/* ========================= */
/* Filesystem Walk Callback  */
/* ========================= */

typedef int (*cleaner_fs_callback)(
    const cleaner_fs_entry *entry,
    void *user_data);

/* ========================= */
/* API                       */
/* ========================= */

int cleaner_fs_walk(
    const char *root_path,
    cleaner_fs_callback callback,
    void *user_data);

bool cleaner_fs_exists(const char *path);
bool cleaner_fs_is_directory(const char *path);
bool cleaner_fs_create_directory(const char *path);
bool cleaner_fs_rename(const char *old_path,
                       const char *new_path);
bool cleaner_fs_get_modification_time(
    const char *path_utf8,
    int *year,
    int *month,
    int *day);

bool cleaner_fs_get_creation_time(
    const char *path,
    int *year,
    int *month,
    int *day);

bool cleaner_fs_get_modified_time(
    const char *path,
    int *year,
    int *month,
    int *day);

#endif