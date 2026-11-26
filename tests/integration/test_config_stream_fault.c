/* Test-only GNU stream adapter. The actual loader, map and file open are
 * unchanged; only fdopen in this isolated executable is wrapped. */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config/config.h"

static struct {
    int armed, fd, error, close_error;
    unsigned int eof_seen, close_injected;
    size_t limit, delivered;
    unsigned int wrapped, injected, closed;
    int close_ok;
} fault;

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
    if (close_cases != 3U)
        goto cleanup;
    result = 0;
cleanup:
    if (unlink(path) != 0)
        result = 1;
    if (result == 0)
    {
        puts("config_stream_fault=PASS cases=12 cuts=6 errors=2 rollback=12 closed=12 recovery=12");
        puts("config_close_fault=PASS cases=3 eof=3 rollback=3 closed=3 adapter_recovery=3");
    }
    return result;
}
