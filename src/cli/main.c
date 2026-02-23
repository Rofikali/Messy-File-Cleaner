
// // cleaner/src/main.c

// #include <windows.h>
// #include <wchar.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include "cleaner/system/logger.h"
// #include "cleaner/core/processor.h"
// #include "cleaner/system/threadpool.h"
// #include "cleaner/core/scanner.h"
// #include "cleaner/system/metrics.h"

// #define MAX_PATH_LONG 32767

// #define DEFAULT_MODE TIME_MODE_MODIFIED
// #define DEFAULT_DRY_RUN 0
// #define DEFAULT_TARGET L".\\sorted"
// #define DEFAULT_THREADS 4

// static void print_usage()
// {
//     wprintf(L"\nCleaner Enterprise\n");
//     wprintf(L"Usage:\n");
//     wprintf(L"  cleaner.exe --source <path> [--target <path>]\n");
//     wprintf(L"              [--mode creation|modified]\n");
//     wprintf(L"              [--dry-run]\n\n");
// }

// void print_help()
// {
//     wprintf(L"\nCleaner Enterprise v1.0.0\n");
//     wprintf(L"----------------------------------------\n");
//     wprintf(L"Usage:\n");
//     wprintf(L"  cleaner.exe --source <path> [options]\n\n");

//     wprintf(L"Core Options:\n");
//     wprintf(L"  --source <path>          Source directory (required)\n");
//     wprintf(L"  --target <path>          Target directory\n");
//     wprintf(L"  --mode creation|modified Time mode\n");
//     wprintf(L"  --dry-run                Simulation only\n");
//     wprintf(L"  --exclude <pattern>      Exclude pattern\n\n");

//     wprintf(L"Progress Options (optional):\n");
//     wprintf(L"  --progress               Enable progress bar\n");
//     wprintf(L"  --progress-percent       Show percentage\n");
//     wprintf(L"  --progress-eta           Show ETA\n");
//     wprintf(L"  --progress-speed         Show MB/s speed\n\n");

//     wprintf(L"Examples:\n");
//     wprintf(L"  cleaner.exe --source C:\\Test --progress\n");
//     wprintf(L"  cleaner.exe --source C:\\Test --progress --progress-eta\n");
// }

// static int directory_exists(const wchar_t *path)
// {
//     DWORD attr = GetFileAttributesW(path);
//     return (attr != INVALID_FILE_ATTRIBUTES &&
//             (attr & FILE_ATTRIBUTE_DIRECTORY));
// }
// static int cli_scan_callback(const cleaner_fs_entry *entry,
//                              void *user_data)
// {
//     ThreadPool *pool = (ThreadPool *)user_data;

//     if (!entry->is_directory)
//     {
//         // threadpool_submit(pool, entry->path);

//         // Convert narrow path to wide
//         wchar_t wide_path[MAX_PATH_LONG];
//         mbstowcs(wide_path, entry->path, MAX_PATH_LONG);

//         // threadpool_add_job(pool, wide_path);
//         threadpool_add_job(pool, entry->path);
//     }

//     return 0;
// }
// int wmain(int argc, wchar_t *argv[])

// {
//     ProgressOptions progress = {0};

//     wchar_t source[MAX_PATH_LONG] = L"";
//     wchar_t target[MAX_PATH_LONG] = DEFAULT_TARGET;

//     TimeMode mode = DEFAULT_MODE;
//     int dry_run = DEFAULT_DRY_RUN;

//     if (argc < 2)
//     {
//         print_usage();
//         return 1;
//     }

//     for (int i = 1; i < argc; i++)
//     {
//         if (wcscmp(argv[i], L"--help") == 0)
//         {
//             print_help();
//             exit(0);
//         }

//         else if (wcscmp(argv[i], L"--source") == 0 && i + 1 < argc)
//         {
//             wcscpy(source, argv[++i]);
//         }
//         else if (wcscmp(argv[i], L"--exclude") == 0 && i + 1 < argc)
//         {
//             if (g_config.exclude_count < MAX_EXCLUDES)
//             {
//                 wcscpy(
//                     g_config.exclude_patterns[g_config.exclude_count],
//                     argv[++i]);

//                 g_config.exclude_count++;
//             }
//         }

//         else if (wcscmp(argv[i], L"--target") == 0 && i + 1 < argc)
//         {
//             wcscpy(target, argv[++i]);
//         }
//         else if (wcscmp(argv[i], L"--mode") == 0 && i + 1 < argc)
//         {
//             i++;
//             if (wcscmp(argv[i], L"creation") == 0)
//                 mode = TIME_MODE_CREATION;
//             else if (wcscmp(argv[i], L"modified") == 0)
//                 mode = TIME_MODE_MODIFIED;
//             else
//             {
//                 wprintf(L"Invalid mode.\n");
//                 return 1;
//             }
//         }
//         else if (wcscmp(argv[i], L"--dry-run") == 0)
//         {
//             dry_run = 1;
//         }
//         else if (wcscmp(argv[i], L"--progress") == 0)
//         {
//             progress.show_bar = 1;
//         }
//         else if (wcscmp(argv[i], L"--progress-percent") == 0)
//         {
//             progress.show_percent = 1;
//         }
//         else if (wcscmp(argv[i], L"--progress-eta") == 0)
//         {
//             progress.show_eta = 1;
//         }
//         else if (wcscmp(argv[i], L"--progress-speed") == 0)
//         {
//             progress.show_speed = 1;
//         }

//         else
//         {
//             print_usage();
//             return 1;
//         }
//     }

//     if (wcslen(source) == 0)
//     {
//         wprintf(L"--source is required.\n");
//         return 1;
//     }

//     DWORD attr = GetFileAttributesW(source);

//     if (attr == INVALID_FILE_ATTRIBUTES ||
//         !(attr & FILE_ATTRIBUTE_DIRECTORY))
//     {
//         fwprintf(stderr, L"Invalid source directory\n");
//         return 1;
//     }

//     if (!directory_exists(source))
//     {
//         wprintf(L"Source directory does not exist.\n");
//         return 1;
//     }

//     if (!directory_exists(target))
//     {
//         CreateDirectoryW(target, NULL);
//     }
//     if (_wcsicmp(source, target) == 0)
//     {
//         wprintf(L"Warning: source and target are the same directory.\n");
//     }

//     logger_init(L"logs");

//     ThreadPool pool;

//     threadpool_init(&pool,
//                     target,
//                     DEFAULT_THREADS,
//                     dry_run,
//                     mode);

//     // size_t total_files = count_files_recursive(source);
//     // metrics_set_options(&progress);
//     // metrics_init(total_files);
//     // scan_directory(&pool,
//     //                source); /* recursive */

//     char narrow_source[MAX_PATH_LONG];
//     // wcstombs(narrow_source, source, MAX_PATH_LONG);
//     threadpool_init(&pool,
//                     narrow_target,
//                     DEFAULT_THREADS,
//                     dry_run,
//                     mode);

//     // cleaner_scan(narrow_source);
//     cleaner_scan(narrow_source,
//                  cli_scan_callback,
//                  &pool);

//     threadpool_destroy(&pool);

//     metrics_print();
//     logger_close();

//     return 0;
// }

// cleaner/src/main.c

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cleaner/system/logger.h"
#include "cleaner/system/threadpool.h"
#include "cleaner/core/scanner.h"
#include "cleaner/system/metrics.h"
#include "cleaner/core/processor.h"

#define DEFAULT_THREADS 4
#define DEFAULT_TARGET "./sorted"

static void print_usage()
{
    printf("\nCleaner Enterprise\n");
    printf("Usage:\n");
    printf("  cleaner.exe --source <path> [--target <path>]\n");
    printf("              [--mode creation|modified]\n");
    printf("              [--dry-run]\n\n");
}

static int directory_exists(const char *path)
{
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES &&
            (attr & FILE_ATTRIBUTE_DIRECTORY));
}

// static int cli_scan_callback(const cleaner_fs_entry *entry,
//                              void *user_data)
// {
//     ThreadPool *pool = (ThreadPool *)user_data;

//     if (!entry->is_directory)
//     {
//         threadpool_submit(pool,
//                           processor_process_file,
//                           (void *)entry->path);
//     }

//     return 0;
// }

static int cli_scan_callback(const cleaner_fs_entry *entry,
                             void *user_data)
{
    ThreadPool *pool = (ThreadPool *)user_data;

    if (!entry->is_directory)
    {
        char *path_copy = _strdup(entry->path);
        if (!path_copy)
            return -1;

        threadpool_submit(pool,
                          processor_process_file,
                          path_copy);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    char source[4096] = {0};
    char target[4096] = DEFAULT_TARGET;

    TimeMode mode = TIME_MODE_MODIFIED;
    int dry_run = 0;

    if (argc < 2)
    {
        print_usage();
        return 1;
    }

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--source") == 0 && i + 1 < argc)
        {
            strncpy(source, argv[++i], sizeof(source) - 1);
        }
        else if (strcmp(argv[i], "--target") == 0 && i + 1 < argc)
        {
            strncpy(target, argv[++i], sizeof(target) - 1);
        }
        else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc)
        {
            i++;
            if (strcmp(argv[i], "creation") == 0)
                mode = TIME_MODE_CREATION;
            else if (strcmp(argv[i], "modified") == 0)
                mode = TIME_MODE_MODIFIED;
            else
            {
                printf("Invalid mode\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "--dry-run") == 0)
        {
            dry_run = 1;
        }
        else
        {
            print_usage();
            return 1;
        }
    }

    if (strlen(source) == 0)
    {
        printf("--source is required\n");
        return 1;
    }

    if (!directory_exists(source))
    {
        printf("Invalid source directory\n");
        return 1;
    }

    if (!directory_exists(target))
    {
        CreateDirectoryA(target, NULL);
    }

    // logger_init("logs");

    // /* Configure processor */
    // processor_set_mode(mode);
    // processor_set_target(target);
    // processor_set_dry_run(dry_run);

    // /* Create thread pool */
    // ThreadPool *pool = threadpool_create(DEFAULT_THREADS);

    // /* Scan and submit jobs */
    // cleaner_scan(source,
    //              cli_scan_callback,
    //              pool);

    // /* Wait and destroy */
    // threadpool_destroy(pool);

    // // metrics_print();
    // metrics_init();
    // logger_close();

    // return 0;
    logger_init("logs");
    metrics_init();

    processor_set_mode(mode);
    processor_set_target(target);
    processor_set_dry_run(dry_run);

    ThreadPool *pool = threadpool_create(DEFAULT_THREADS);

    cleaner_scan(source,
                 cli_scan_callback,
                 pool);

    threadpool_destroy(pool);

    metrics_finish();

    printf("\nProcessed: %llu\n",
           (unsigned long long)metrics_get_files_processed());
    printf("Moved: %llu\n",
           (unsigned long long)metrics_get_files_moved());
    printf("Errors: %llu\n",
           (unsigned long long)metrics_get_errors());
    printf("Time: %.3f sec\n",
           metrics_get_elapsed_seconds());

    logger_close();
}

// // 🏁 Next Possible Evolution

// // If you want to go even further:

// // Threadpool dynamic sizing

// // Work-stealing queue

// // IO batching

// // Atomic journaling

// // Rollback on failure

// // Structured logging

// // File hash duplicate detection

// // SIMD filename parsing

// // That’s senior systems territory.

// // // https://pinkclips.mobi/download/video/66e356ff6021d/lovers-enjoying-each-other-at-sunset
// // // https://pinkclips.mobi/download/video/6836a19249c06/slow-real-and-full-of-love-%E2%80%94-husband-and-wife-making-love

// // i am  building the same architecture used in:

// // Backup systems

// // Antivirus quarantine engines

// // Media organizers

// // Digital forensics pipelines