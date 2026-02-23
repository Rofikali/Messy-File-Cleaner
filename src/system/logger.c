
// cleaner/src/system/logger.c

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "cleaner/system/logger.h"

#define LOGGER_MAX_PATH 32767

static FILE *log_file = NULL;
static HANDLE log_mutex = NULL;

static int utf8_to_wide(const char *utf8,
                        wchar_t *wide,
                        size_t wide_size)
{
    return MultiByteToWideChar(
               CP_UTF8,
               0,
               utf8,
               -1,
               wide,
               (int)wide_size) > 0
               ? 0
               : -1;
}

static const char *level_to_string(LogLevel level)
{
    switch (level)
    {
    case LOG_INFO:
        return "INFO";
    case LOG_WARN:
        return "WARN";
    case LOG_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

void logger_init(const char *log_dir)
{
    if (!log_dir)
        return;

    wchar_t wide_dir[LOGGER_MAX_PATH];
    utf8_to_wide(log_dir, wide_dir, LOGGER_MAX_PATH);

    CreateDirectoryW(wide_dir, NULL);

    wchar_t wide_path[LOGGER_MAX_PATH];
    swprintf(wide_path,
             LOGGER_MAX_PATH,
             L"%ls\\cleaner.log",
             wide_dir);

    log_file = _wfopen(wide_path, L"a+, ccs=UTF-8");

    log_mutex = CreateMutex(NULL, FALSE, NULL);
}

void logger_log(LogLevel level,
                const char *event,
                const char *file_path)
{
    if (!log_file)
        return;

    WaitForSingleObject(log_mutex, INFINITE);

    SYSTEMTIME st;
    GetLocalTime(&st);

    fprintf(log_file,
            "[%04d-%02d-%02d %02d:%02d:%02d] [%s] %s : %s\n",
            st.wYear,
            st.wMonth,
            st.wDay,
            st.wHour,
            st.wMinute,
            st.wSecond,
            level_to_string(level),
            event ? event : "",
            file_path ? file_path : "");

    fflush(log_file);

    ReleaseMutex(log_mutex);
}

void logger_close()
{
    if (log_file)
        fclose(log_file);

    if (log_mutex)
        CloseHandle(log_mutex);

    log_file = NULL;
    log_mutex = NULL;
}