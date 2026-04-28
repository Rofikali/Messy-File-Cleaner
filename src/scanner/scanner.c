#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include "scanner/scanner.h"

void scan_directory(const char *path)
{
    struct dirent *entry;
    DIR *dir = opendir(path);

    if (!dir)
    {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == -1)
            continue;

        if (S_ISDIR(st.st_mode))
        {
            scan_directory(full_path);
        }
        else
        {
            printf("File: %s\n", full_path);
        }
    }

    closedir(dir);
}