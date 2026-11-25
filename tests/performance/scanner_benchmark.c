#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/resource.h>
#include <time.h>

#include "scanner/scanner.h"

static size_t visited;

static int count_file(const char *path)
{
    (void)path;
    if (visited == SIZE_MAX)
    {
        errno = EOVERFLOW;
        return -1;
    }
    ++visited;
    return 0;
}

static double seconds_between(struct timespec start, struct timespec end)
{
    return (double)(end.tv_sec - start.tv_sec) +
           (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
}

static double cpu_seconds(struct timeval value)
{
    return (double)value.tv_sec + (double)value.tv_usec / 1000000.0;
}

/* Benchmark-only binary. Read-only discovery, no worker, journal or mover. */
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fputs("Usage: scanner-benchmark DISPOSABLE_ROOT\n", stderr);
        return 2;
    }

    struct timespec start, end;
    struct rusage before, after;
    if (getrusage(RUSAGE_SELF, &before) != 0 ||
        clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        return 1;

    int result = scan_directory(argv[1], count_file, NULL);
    int scan_errno = errno;
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0 ||
        getrusage(RUSAGE_SELF, &after) != 0)
        return 1;
    if (result != 0)
    {
        fprintf(stderr, "Scanner benchmark failed (errno=%d)\n", scan_errno);
        return 1;
    }

    double elapsed = seconds_between(start, end);
    double user = cpu_seconds(after.ru_utime) - cpu_seconds(before.ru_utime);
    double system = cpu_seconds(after.ru_stime) - cpu_seconds(before.ru_stime);
    if (elapsed <= 0 || user < 0 || system < 0 || after.ru_maxrss < 0)
        return 1;

    printf("{\"files\":%zu,\"elapsed_seconds\":%.9f,"
           "\"user_cpu_seconds\":%.6f,\"system_cpu_seconds\":%.6f,"
           "\"process_peak_rss_kib\":%ld}\n",
           visited, elapsed, user, system, after.ru_maxrss);
    return 0;
}
