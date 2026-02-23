// // cleaner/src/system/metrics.c
// /*
//  * metrics.c
//  *
//  * Platform-independent metrics implementation.
//  * Uses system abstraction layer for time.
//  *
//  * NO direct OS calls.
//  * NO clock_gettime.
//  * NO Windows APIs.
//  *
//  * Depends only on:
//  *   - cleaner_time_now_ns()
//  */

// #include "cleaner/system/metrics.h"
// #include "cleaner/platform/time.h"

// #include <stdint.h>
// #include <stdatomic.h>

// /* ============================================================
//  * Internal State
//  * ============================================================ */

// static atomic_uint_fast64_t g_files_processed = 0;
// static atomic_uint_fast64_t g_files_moved = 0;
// static atomic_uint_fast64_t g_errors = 0;

// static atomic_uint_fast64_t g_start_time_ns = 0;
// static atomic_uint_fast64_t g_end_time_ns = 0;

// /* ============================================================
//  * Lifecycle
//  * ============================================================ */

// void metrics_init(void)
// {
//     atomic_store(&g_files_processed, 0);
//     atomic_store(&g_files_moved, 0);
//     atomic_store(&g_errors, 0);

//     atomic_store(&g_start_time_ns, cleaner_time_now_ns());
//     atomic_store(&g_end_time_ns, 0);
// }

// void metrics_finish(void)
// {
//     atomic_store(&g_end_time_ns, cleaner_time_now_ns());
// }

// /* ============================================================
//  * Counters
//  * ============================================================ */

// void metrics_increment_processed(void)
// {
//     atomic_fetch_add(&g_files_processed, 1);
// }

// void metrics_increment_moved(void)
// {
//     atomic_fetch_add(&g_files_moved, 1);
// }

// void metrics_increment_error(void)
// {
//     atomic_fetch_add(&g_errors, 1);
// }

// /* ============================================================
//  * Getters
//  * ============================================================ */

// uint64_t metrics_get_files_processed(void)
// {
//     return atomic_load(&g_files_processed);
// }

// uint64_t metrics_get_files_moved(void)
// {
//     return atomic_load(&g_files_moved);
// }

// uint64_t metrics_get_errors(void)
// {
//     return atomic_load(&g_errors);
// }

// uint64_t metrics_get_elapsed_ns(void)
// {
//     uint64_t start = atomic_load(&g_start_time_ns);
//     uint64_t end = atomic_load(&g_end_time_ns);

//     if (end == 0)
//         end = cleaner_time_now_ns();

//     if (end < start)
//         return 0;

//     return end - start;
// }

// double metrics_get_elapsed_seconds(void)
// {
//     return (double)metrics_get_elapsed_ns() / 1e9;
// }

/*
 * metrics.c
 * Pure system layer.
 * NO platform includes.
 */

#include "cleaner/system/metrics.h"
#include "cleaner/platform/time.h"

#include <stdatomic.h>

/* ============================================================
 * Internal State
 * ============================================================ */

static atomic_uint_fast64_t g_files_processed = 0;
static atomic_uint_fast64_t g_files_moved = 0;
static atomic_uint_fast64_t g_errors = 0;

static atomic_uint_fast64_t g_start_time_ns = 0;
static atomic_uint_fast64_t g_end_time_ns = 0;

/* ============================================================
 * Lifecycle
 * ============================================================ */

void metrics_init(void)
{
    atomic_store(&g_files_processed, 0);
    atomic_store(&g_files_moved, 0);
    atomic_store(&g_errors, 0);

    atomic_store(&g_start_time_ns, cleaner_time_now_ns());
    atomic_store(&g_end_time_ns, 0);
}

void metrics_finish(void)
{
    atomic_store(&g_end_time_ns, cleaner_time_now_ns());
}

/* ============================================================
 * Counters
 * ============================================================ */

void metrics_increment_processed(void)
{
    atomic_fetch_add(&g_files_processed, 1);
}

void metrics_increment_moved(void)
{
    atomic_fetch_add(&g_files_moved, 1);
}

void metrics_increment_error(void)
{
    atomic_fetch_add(&g_errors, 1);
}

/* ============================================================
 * Getters
 * ============================================================ */

uint64_t metrics_get_files_processed(void)
{
    return atomic_load(&g_files_processed);
}

uint64_t metrics_get_files_moved(void)
{
    return atomic_load(&g_files_moved);
}

uint64_t metrics_get_errors(void)
{
    return atomic_load(&g_errors);
}

uint64_t metrics_get_elapsed_ns(void)
{
    uint64_t start = atomic_load(&g_start_time_ns);
    uint64_t end = atomic_load(&g_end_time_ns);

    if (end == 0)
        end = cleaner_time_now_ns();

    if (end < start)
        return 0;

    return end - start;
}

double metrics_get_elapsed_seconds(void)
{
    return (double)metrics_get_elapsed_ns() / 1e9;
}