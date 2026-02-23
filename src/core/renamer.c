
// cleaner/src/core/renamer.c

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "cleaner/core/renamer.h"

#define MAX_EXTENSIONS 128

typedef struct
{
    char extension[64];
    LONG64 counter;
} ExtensionCounter;

static ExtensionCounter counters[MAX_EXTENSIONS];
static int counter_count = 0;
static CRITICAL_SECTION counter_lock;

void renamer_init()
{
    InitializeCriticalSection(&counter_lock);
}

static LONG64 get_next_number(const char *ext)
{
    EnterCriticalSection(&counter_lock);

    for (int i = 0; i < counter_count; i++)
    {
        if (_stricmp(counters[i].extension, ext) == 0)
        {
            LONG64 value = ++counters[i].counter;
            LeaveCriticalSection(&counter_lock);
            return value;
        }
    }

    strcpy(counters[counter_count].extension, ext);
    counters[counter_count].counter = 1;
    counter_count++;

    LeaveCriticalSection(&counter_lock);

    return 1;
}

void build_timestamped_name(const char *original_name,
                            const char *extension,
                            char *output,
                            size_t size)
{
    const char *dot = strrchr(original_name, '.');

    char base[512];

    if (dot)
    {
        // size_t len = dot - original_name;
        size_t len = (size_t)(dot - original_name);
        strncpy(base, original_name, len);
        base[len] = '\0';
    }
    else
    {
        strcpy(base, original_name);
    }

    SYSTEMTIME st;
    GetLocalTime(&st);

    char datetime[64];
    snprintf(datetime,
             sizeof(datetime),
             "%04d-%02d-%02d_%02d-%02d-%02d",
             st.wYear,
             st.wMonth,
             st.wDay,
             st.wHour,
             st.wMinute,
             st.wSecond);

    LONG64 number = get_next_number(extension);

    snprintf(output,
             size,
             "%s_%s_%lld.%s",
             base,
             datetime,
             number,
             extension);
}