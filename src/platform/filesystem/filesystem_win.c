// // src/platform/filesystem_win.c

#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "cleaner/platform/filesystem.h"

static int walk_internal(
    const char *root,
    cleaner_fs_callback callback,
    void *user_data)
{
    char search_path[4096];
    snprintf(search_path, sizeof(search_path),
             "%s\\*", root);

    WIN32_FIND_DATAA find;
    HANDLE h = FindFirstFileA(search_path, &find);
    if (h == INVALID_HANDLE_VALUE)
        return -1;

    do
    {
        if (strcmp(find.cFileName, ".") == 0 ||
            strcmp(find.cFileName, "..") == 0)
            continue;

        char full_path[4096];
        snprintf(full_path, sizeof(full_path),
                 "%s\\%s", root, find.cFileName);

        cleaner_fs_entry entry = {
            .path = full_path,
            .is_directory =
                (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0};

        callback(&entry, user_data);

        if (entry.is_directory)
            walk_internal(full_path, callback, user_data);

    } while (FindNextFileA(h, &find));

    FindClose(h);
    return 0;
}

int cleaner_fs_walk(const char *root_path,
                    cleaner_fs_callback callback,
                    void *user_data)

{
    return walk_internal(root_path, callback, user_data);
}

bool cleaner_fs_get_modification_time(
    const char *path_utf8,
    int *year,
    int *month,
    int *day)

{
    WIN32_FILE_ATTRIBUTE_DATA data;

    if (!GetFileAttributesExA(path_utf8,
                              GetFileExInfoStandard,
                              &data))
        return false;

    FILETIME ft = data.ftLastWriteTime;

    FILETIME local_ft;
    FileTimeToLocalFileTime(&ft, &local_ft);

    SYSTEMTIME st;
    FileTimeToSystemTime(&local_ft, &st);

    *year = st.wYear;
    *month = st.wMonth;
    *day = st.wDay;

    return true;
}

/* =============================== */
/* Directory Exists               */
/* =============================== */

bool cleaner_fs_exists(const char *path)
{
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES);
}

bool cleaner_fs_is_directory(const char *path)
{
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES)
        return false;

    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool cleaner_fs_create_directory(const char *path)
{
    if (CreateDirectoryA(path, NULL))
        return true;

    /* already exists */
    return GetLastError() == ERROR_ALREADY_EXISTS;
}

bool cleaner_fs_rename(const char *old_path,
                       const char *new_path)
{
    return MoveFileA(old_path, new_path) != 0;
}

/* =============================== */
/* Creation Time                  */
/* =============================== */

bool cleaner_fs_get_creation_time(
    const char *path,
    int *year,
    int *month,
    int *day)
{
    WIN32_FILE_ATTRIBUTE_DATA data;

    if (!GetFileAttributesExA(path,
                              GetFileExInfoStandard,
                              &data))
        return false;

    FILETIME ft = data.ftCreationTime;

    FILETIME local_ft;
    FileTimeToLocalFileTime(&ft, &local_ft);

    SYSTEMTIME st;
    FileTimeToSystemTime(&local_ft, &st);

    *year = st.wYear;
    *month = st.wMonth;
    *day = st.wDay;

    return true;
}

bool cleaner_fs_get_modified_time(
    const char *path,
    int *year,
    int *month,
    int *day)
{
    return cleaner_fs_get_modification_time(
        path, year, month, day);
}