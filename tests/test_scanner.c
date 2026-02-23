
// // tests / test_scanner.c

// #include <stdio.h>
// #include <stdlib.h>
// #include <assert.h>
// #include "cleaner/core/scanner.h"

// int main(void)
// {
//     int result = cleaner_scan("test_temp");

//     assert(result == 0);

//     printf("Scanner test passed.\n");

//     return 0;
// }

// #include <stdio.h>
// #include <stdlib.h>
// #include <assert.h>
// #include <direct.h>  // _mkdir on Windows
// #include <windows.h> // DeleteFileA, RemoveDirectoryA

// #include "cleaner/core/scanner.h"

// int main(void)
// {
//     const char *dir = "test_temp";

//     // Create test directory
//     _mkdir(dir);

//     // Create a dummy file
//     char filepath[MAX_PATH];
//     snprintf(filepath, MAX_PATH, "%s\\file.txt", dir);

//     FILE *f = fopen(filepath, "w");
//     assert(f != NULL);
//     fprintf(f, "test");
//     fclose(f);

//     int result = cleaner_scan(dir);
//     assert(result == 0);

//     // Cleanup
//     DeleteFileA(filepath);
//     RemoveDirectoryA(dir);

//     printf("Scanner test passed.\n");
//     return 0;
// }
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define MKDIR(path) _mkdir(path)
#define SEP "\\"
#else
#include <sys/stat.h>
#include <unistd.h>
#define MKDIR(path) mkdir(path, 0755)
#define SEP "/"
#endif

#include "cleaner/core/scanner.h"

typedef struct
{
    int file_count;
    int dir_count;
} test_metrics;

static int test_callback(const cleaner_fs_entry *entry,
                         void *user_data)
{
    test_metrics *m = (test_metrics *)user_data;

    if (entry->is_directory)
        m->dir_count++;
    else
        m->file_count++;

    return 0;
}

static void create_file(const char *path)
{
    FILE *f = fopen(path, "w");
    assert(f != NULL);
    fprintf(f, "test");
    fclose(f);
}

int main(void)
{
    const char *root = "test_temp";
    MKDIR(root);

    char subdir[256];
    snprintf(subdir, sizeof(subdir), "%s%s%s",
             root, SEP, "sub");
    MKDIR(subdir);

    char file1[256];
    snprintf(file1, sizeof(file1), "%s%s%s",
             root, SEP, "file1.txt");
    create_file(file1);

    char file2[256];
    snprintf(file2, sizeof(file2), "%s%s%s",
             subdir, SEP, "file2.txt");
    create_file(file2);

    test_metrics metrics = {0};

    int result = cleaner_scan(root,
                              test_callback,
                              &metrics);

    assert(result == 0);
    assert(metrics.file_count == 2);
    assert(metrics.dir_count >= 1);

    printf("Files detected: %d\n", metrics.file_count);
    printf("Dirs detected: %d\n", metrics.dir_count);
    printf("Scanner behavioral test passed.\n");

    /* cleanup */
#ifdef _WIN32
    DeleteFileA(file2);
    RemoveDirectoryA(subdir);
    DeleteFileA(file1);
    RemoveDirectoryA(root);
#else
    remove(file2);
    rmdir(subdir);
    remove(file1);
    rmdir(root);
#endif

    return 0;
}