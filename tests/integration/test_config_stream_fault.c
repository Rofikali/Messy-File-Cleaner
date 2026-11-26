/* Test-only GNU fault adapters. Production loader and map are unchanged;
 * open, fdopen and staged allocation are wrapped in this test only. */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config/config.h"

static struct {
    int armed, fd, error, close_error, stream_error;
    unsigned int stream_injected;
    unsigned int eof_seen, close_injected;
    size_t limit, delivered;
    unsigned int wrapped, injected, closed;
    int close_ok;
} fault;

/* Linker wrapping affects direct references in this executable, not
 * libc's internal allocations. Arm only around a single load_config call. */
static struct {
    int armed, fail;
    unsigned int calls, injected, frees, unexpected_frees;
    size_t bytes;
    void *live;
} allocation;

void *__real_malloc(size_t bytes);
void *__wrap_malloc(size_t bytes);
void __real_free(void *ptr);
void __wrap_free(void *ptr);

void *__wrap_malloc(size_t bytes)
{
    if (!allocation.armed)
        return __real_malloc(bytes);
    ++allocation.calls;
    allocation.bytes = bytes;
    if (allocation.fail)
    {
        ++allocation.injected;
        errno = ENOMEM;
        return NULL;
    }
    void *ptr = __real_malloc(bytes);
    allocation.live = ptr;
    return ptr;
}

void __wrap_free(void *ptr)
{
    if (allocation.armed)
    {
        if (ptr != NULL && ptr == allocation.live)
        {
            ++allocation.frees;
            allocation.live = NULL;
        }
        else
            ++allocation.unexpected_frees;
    }
    __real_free(ptr);
}

static struct {
    int armed, error, fd, flags;
    const char *path;
    unsigned int calls, injected, wrong_path;
} acquisition;

int __real_open(const char *path, int flags, ...);
int __wrap_open(const char *path, int flags, ...);

int __wrap_open(const char *path, int flags, ...)
{
    /* This test exercises read-only opens. Preserve the variadic ABI
     * for other direct open references when not injecting a failure. */
    const int has_mode = (flags & O_CREAT) != 0 ||
                         (flags & O_TMPFILE) == O_TMPFILE;
    mode_t mode = 0;
    if (has_mode)
    {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    if (acquisition.armed)
    {
        ++acquisition.calls;
        acquisition.flags = flags;
        if (strcmp(path, acquisition.path) != 0)
        {
            ++acquisition.wrong_path;
            errno = EINVAL;
            return -1;
        }
        if (acquisition.error)
        {
            ++acquisition.injected;
            errno = acquisition.error;
            return -1;
        }
    }
    int fd = has_mode ? __real_open(path, flags, mode) : __real_open(path, flags);
    if (acquisition.armed)
        acquisition.fd = fd;
    return fd;
}

FILE *__real_fdopen(int fd, const char *mode);
FILE *__wrap_fdopen(int fd, const char *mode);

static ssize_t fault_read(void *cookie, char *buffer, size_t size)
{
    (void)cookie;
    if (fault.error && fault.delivered >= fault.limit)
    {
        ++fault.injected;
        errno = fault.error;
        return -1;
    }
    if (fault.error)
    {
        size_t remaining = fault.limit - fault.delivered;
        if (size > remaining)
            size = remaining;
    }
    ssize_t count;
    do {
        count = read(fault.fd, buffer, size);
    } while (count < 0 && errno == EINTR);
    if (count > 0)
        fault.delivered += (size_t)count;
    else if (count == 0)
        ++fault.eof_seen;
    return count;
}

static int fault_close(void *cookie)
{
    (void)cookie;
    ++fault.closed;
    int result = close(fault.fd);
    fault.close_ok = result == 0;
    if (result == 0 && fault.close_error)
    {
        ++fault.close_injected;
        errno = fault.close_error;
        return -1;
    }
    return result;
}

FILE *__wrap_fdopen(int fd, const char *mode)
{
    if (!fault.armed)
        return __real_fdopen(fd, mode);
    ++fault.wrapped;
    fault.fd = fd;
    if (fault.stream_error)
    {
        ++fault.stream_injected;
        errno = fault.stream_error;
        return NULL; /* Loader still owns the real descriptor. */
    }
    cookie_io_functions_t io = {
        .read = fault_read, .write = NULL, .seek = NULL, .close = fault_close
    };
    /* If creation fails, the loader still owns fd and must close it. */
    return fopencookie(&fault, mode, io);
}

static int unchanged(const Config *cfg, const HashEntry *entries,
                     const HashEntry *snapshot)
{
    return cfg->ext_map.entries == entries && cfg->ext_map.capacity == 64 &&
           cfg->ext_map.size == 1 && cfg->batch_size == 77 &&
           cfg->worker_threads == 3 && cfg->idle_sleep_ms == 9 &&
           cfg->mode == MODE_BATCH &&
           memcmp(entries, snapshot, 64 * sizeof *entries) == 0;
}

/* Real loader allocation at three valid map capacities, including a
 * two-entry map which becomes full on a successful recovery. */
static int allocation_tests(const char *path, size_t length)
{
    const size_t capacities[] = {2, 8, 64};
    unsigned int cases = 0;
    for (size_t i = 0; i < sizeof capacities / sizeof capacities[0]; ++i)
    {
        const size_t capacity = capacities[i];
        HashEntry entries[64], snapshot[64];
        memset(entries, 0, sizeof entries);
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, capacity);
        init_config(&cfg);
        cfg.batch_size = 77;
        cfg.worker_threads = 3;
        cfg.idle_sleep_ms = 9;
        if (hashmap_put(&cfg.ext_map, ".png", "Existing") != 0)
            return 1;
        memcpy(snapshot, entries, sizeof entries);
        memset(&fault, 0, sizeof fault);
        fault.armed = 1;
        fault.fd = -1;
        memset(&allocation, 0, sizeof allocation);
        allocation.armed = 1;
        allocation.fail = 1;
        int status = load_config(path, &cfg);
        allocation.armed = 0;
        fault.armed = 0;
        if (status != -1 || allocation.calls != 1 || allocation.injected != 1 ||
            allocation.bytes != capacity * sizeof(HashEntry) ||
            allocation.frees != 0 || allocation.unexpected_frees != 0 ||
            allocation.live != NULL || fault.wrapped != 0 ||
            cfg.ext_map.entries != entries || cfg.ext_map.capacity != capacity ||
            cfg.ext_map.size != 1 || cfg.batch_size != 77 ||
            cfg.worker_threads != 3 || cfg.idle_sleep_ms != 9 ||
            cfg.mode != MODE_BATCH || memcmp(entries, snapshot, sizeof entries) != 0)
            return 1;

        memset(&fault, 0, sizeof fault);
        fault.armed = 1;
        fault.fd = -1;
        memset(&allocation, 0, sizeof allocation);
        allocation.armed = 1;
        status = load_config(path, &cfg);
        allocation.armed = 0;
        fault.armed = 0;
        if (status != 0 || allocation.calls != 1 || allocation.injected != 0 ||
            allocation.bytes != capacity * sizeof(HashEntry) ||
            allocation.frees != 1 || allocation.unexpected_frees != 0 ||
            allocation.live != NULL || fault.wrapped != 1 || fault.injected != 0 ||
            fault.delivered != length || fault.eof_seen == 0 ||
            fault.closed != 1 || fault.close_injected != 0 || !fault.close_ok ||
            fault.fd < 0 || cfg.ext_map.entries != entries ||
            cfg.ext_map.capacity != capacity || cfg.ext_map.size != 2 ||
            cfg.batch_size != 123 || cfg.worker_threads != 4 ||
            cfg.idle_sleep_ms != 9 || cfg.mode != MODE_DAEMON ||
            memcmp(entries + capacity, snapshot + capacity,
                   (64U - capacity) * sizeof(HashEntry)) != 0)
            return 1;
        errno = 0;
        if (fcntl(fault.fd, F_GETFD) != -1 || errno != EBADF)
            return 1;
        const char *png = hashmap_get(&cfg.ext_map, ".png");
        const char *jpg = hashmap_get(&cfg.ext_map, ".jpg");
        if (!png || !jpg || strcmp(png, "Existing") || strcmp(jpg, "NewFolder"))
            return 1;
        ++cases;
    }
    return cases == 3U ? 0 : 1;
}

static void arm_acquisition(const char *path, int open_error, int stream_error)
{
    memset(&acquisition, 0, sizeof acquisition);
    acquisition.armed = 1;
    acquisition.fd = -1;
    acquisition.path = path;
    acquisition.error = open_error;
    memset(&fault, 0, sizeof fault);
    fault.armed = 1;
    fault.fd = -1;
    fault.stream_error = stream_error;
    memset(&allocation, 0, sizeof allocation);
    allocation.armed = 1;
}

static void disarm_acquisition(void)
{
    acquisition.armed = 0;
    fault.armed = 0;
    allocation.armed = 0;
}

static int acquired_once(void)
{
    const int required = O_NONBLOCK | O_CLOEXEC | O_NOCTTY;
    return acquisition.calls == 1 && acquisition.wrong_path == 0 &&
           (acquisition.flags & O_ACCMODE) == O_RDONLY &&
           (acquisition.flags & required) == required &&
           allocation.calls == 1 && allocation.injected == 0 &&
           allocation.bytes == 64U * sizeof(HashEntry) &&
           allocation.frees == 1 && allocation.unexpected_frees == 0 &&
           allocation.live == NULL;
}

static int is_closed(int fd)
{
    errno = 0;
    return fd >= 0 && fcntl(fd, F_GETFD) == -1 && errno == EBADF;
}

static int acquisition_tests(const char *path, size_t length)
{
    const int open_errors[] = {EACCES, EMFILE, ENFILE};
    const int stream_errors[] = {ENOMEM, EMFILE, EINVAL};
    unsigned int cases = 0;
    for (unsigned int stage = 0; stage < 2; ++stage)
        for (size_t i = 0; i < 3; ++i)
        {
            HashEntry entries[64], snapshot[64];
            memset(entries, 0, sizeof entries);
            Config cfg;
            hashmap_init(&cfg.ext_map, entries, 64);
            init_config(&cfg);
            cfg.batch_size = 77;
            cfg.worker_threads = 3;
            cfg.idle_sleep_ms = 9;
            if (hashmap_put(&cfg.ext_map, ".png", "Existing") != 0)
                return 1;
            memcpy(snapshot, entries, sizeof entries);
            arm_acquisition(path, stage == 0 ? open_errors[i] : 0,
                            stage == 1 ? stream_errors[i] : 0);
            int status = load_config(path, &cfg);
            disarm_acquisition();
            if (status != -1 || !acquired_once() ||
                !unchanged(&cfg, entries, snapshot) ||
                fault.delivered != 0 || fault.eof_seen != 0 ||
                fault.closed != 0 || fault.injected != 0 || fault.close_injected != 0)
                return 1;
            if (stage == 0)
            {
                if (acquisition.injected != 1 || acquisition.fd != -1 ||
                    fault.wrapped != 0 || fault.stream_injected != 0 || fault.fd != -1)
                    return 1;
            }
            else
            {
                if (acquisition.injected != 0 || fault.wrapped != 1 ||
                    fault.stream_injected != 1 || fault.fd != acquisition.fd ||
                    !is_closed(acquisition.fd))
                    return 1;
            }

            arm_acquisition(path, 0, 0);
            status = load_config(path, &cfg);
            disarm_acquisition();
            if (status != 0 || !acquired_once() || acquisition.injected != 0 ||
                fault.wrapped != 1 || fault.stream_injected != 0 ||
                fault.fd != acquisition.fd || fault.injected != 0 ||
                fault.delivered != length || fault.eof_seen == 0 ||
                fault.closed != 1 || fault.close_injected != 0 || !fault.close_ok ||
                !is_closed(acquisition.fd) || cfg.ext_map.entries != entries ||
                cfg.ext_map.capacity != 64 || cfg.ext_map.size != 2 ||
                cfg.batch_size != 123 || cfg.worker_threads != 4 ||
                cfg.idle_sleep_ms != 9 || cfg.mode != MODE_DAEMON)
                return 1;
            const char *png = hashmap_get(&cfg.ext_map, ".png");
            const char *jpg = hashmap_get(&cfg.ext_map, ".jpg");
            if (!png || !jpg || strcmp(png, "Existing") || strcmp(jpg, "NewFolder"))
                return 1;
            ++cases;
        }
    return cases == 6U ? 0 : 1;
}

int main(void)
{
    static const char input[] =
        "batch_size=123\next.jpg=NewFolder\nworker_threads=4\nmode=daemon\n";
    const size_t cuts[] = {0, 1, sizeof("batch_size=123\n") - 1,
        sizeof("batch_size=123\next.jpg=NewFolder\n") - 1,
        sizeof input - 2, sizeof input - 1};
    const int errors[] = {EIO, EACCES};
    char path[] = "/tmp/fc-config-stream-XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0)
        return 1;
    int result = 1;
    const size_t length = sizeof input - 1;
    ssize_t written = write(fd, input, length);
    int closed = close(fd);
    if (written != (ssize_t)length || closed != 0)
        goto cleanup;
    unsigned int cases = 0;
    for (size_t e = 0; e < sizeof errors / sizeof errors[0]; ++e)
        for (size_t c = 0; c < sizeof cuts / sizeof cuts[0]; ++c)
        {
            HashEntry entries[64], snapshot[64];
            memset(entries, 0, sizeof entries);
            Config cfg;
            hashmap_init(&cfg.ext_map, entries, 64);
            init_config(&cfg);
            cfg.batch_size = 77;
            cfg.worker_threads = 3;
            cfg.idle_sleep_ms = 9;
            if (hashmap_put(&cfg.ext_map, ".png", "Existing") != 0)
                goto cleanup;
            memcpy(snapshot, entries, sizeof entries);
            memset(&fault, 0, sizeof fault);
            fault.armed = 1;
            fault.fd = -1;
            fault.limit = cuts[c];
            fault.error = errors[e];
            int status = load_config(path, &cfg);
            fault.armed = 0;
            if (status != -1 || fault.wrapped != 1 || fault.injected != 1 ||
                fault.delivered != cuts[c] || fault.eof_seen != 0 ||
                fault.close_injected != 0 || fault.closed != 1 || !fault.close_ok ||
                fault.fd < 0 || !unchanged(&cfg, entries, snapshot))
                goto cleanup;
            errno = 0;
            if (fcntl(fault.fd, F_GETFD) != -1 || errno != EBADF)
                goto cleanup;
            /* Same actual file must load successfully without the adapter. */
            if (load_config(path, &cfg) != 0 || cfg.ext_map.entries != entries ||
                cfg.ext_map.capacity != 64 || cfg.ext_map.size != 2 ||
                cfg.batch_size != 123 || cfg.worker_threads != 4 ||
                cfg.idle_sleep_ms != 9 || cfg.mode != MODE_DAEMON)
                goto cleanup;
            const char *png = hashmap_get(&cfg.ext_map, ".png");
            const char *jpg = hashmap_get(&cfg.ext_map, ".jpg");
            if (!png || !jpg || strcmp(png, "Existing") || strcmp(jpg, "NewFolder"))
                goto cleanup;
            ++cases;
        }
    if (cases != 12U)
        goto cleanup;
    /* A successful EOF followed by a close failure must not commit any
     * staged settings. The cookie actually releases the descriptor first. */
    const int close_errors[] = {EIO, ENOSPC, EINTR};
    unsigned int close_cases = 0;
    for (size_t i = 0; i < sizeof close_errors / sizeof close_errors[0]; ++i)
    {
        HashEntry entries[64], snapshot[64];
        memset(entries, 0, sizeof entries);
        Config cfg;
        hashmap_init(&cfg.ext_map, entries, 64);
        init_config(&cfg);
        cfg.batch_size = 77;
        cfg.worker_threads = 3;
        cfg.idle_sleep_ms = 9;
        if (hashmap_put(&cfg.ext_map, ".png", "Existing") != 0)
            goto cleanup;
        memcpy(snapshot, entries, sizeof entries);
        memset(&fault, 0, sizeof fault);
        fault.armed = 1;
        fault.fd = -1;
        fault.close_error = close_errors[i];
        int status = load_config(path, &cfg);
        fault.armed = 0;
        if (status != -1 || fault.wrapped != 1 || fault.injected != 0 ||
            fault.delivered != length || fault.eof_seen == 0 ||
            fault.closed != 1 || fault.close_injected != 1 || !fault.close_ok ||
            fault.fd < 0 || !unchanged(&cfg, entries, snapshot))
            goto cleanup;
        errno = 0;
        if (fcntl(fault.fd, F_GETFD) != -1 || errno != EBADF)
            goto cleanup;
        /* Same adapter, same bytes, no fault: positive commit control. */
        memset(&fault, 0, sizeof fault);
        fault.armed = 1;
        fault.fd = -1;
        status = load_config(path, &cfg);
        fault.armed = 0;
        if (status != 0 || fault.wrapped != 1 || fault.injected != 0 ||
            fault.delivered != length || fault.eof_seen == 0 ||
            fault.closed != 1 || fault.close_injected != 0 || !fault.close_ok ||
            fault.fd < 0 || cfg.ext_map.entries != entries ||
            cfg.ext_map.capacity != 64 || cfg.ext_map.size != 2 ||
            cfg.batch_size != 123 || cfg.worker_threads != 4 ||
            cfg.idle_sleep_ms != 9 || cfg.mode != MODE_DAEMON)
            goto cleanup;
        errno = 0;
        if (fcntl(fault.fd, F_GETFD) != -1 || errno != EBADF)
            goto cleanup;
        const char *png = hashmap_get(&cfg.ext_map, ".png");
        const char *jpg = hashmap_get(&cfg.ext_map, ".jpg");
        if (!png || !jpg || strcmp(png, "Existing") || strcmp(jpg, "NewFolder"))
            goto cleanup;
        ++close_cases;
    }
    if (close_cases != 3U || allocation_tests(path, length) != 0 ||
        acquisition_tests(path, length) != 0)
        goto cleanup;
    result = 0;
cleanup:
    if (unlink(path) != 0)
        result = 1;
    if (result == 0)
    {
        puts("config_stream_fault=PASS cases=12 cuts=6 errors=2 rollback=12 closed=12 recovery=12");
        puts("config_close_fault=PASS cases=3 eof=3 rollback=3 closed=3 adapter_recovery=3");
        puts("config_alloc_fault=PASS cases=3 capacities=2,8,64 rollback=3 no_stream=3 recovery=3 staged_free=3");
        puts("config_acquire_fault=PASS cases=6 open=3 stream=3 rollback=6 raw_fd_closed=3 staged_free=6 recovery=6");
    }
    return result;
}
