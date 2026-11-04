#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <stdatomic.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>

_Static_assert(ATOMIC_INT_LOCK_FREE == 2, "signal handler requires lock-free atomic_int");

#include "scanner/scanner.h"
#include "watcher/watcher.h"
#include "core/thread_pool.h"
#include "core/context.h"
#include "config/config.h"
#include "utils/hashmap.h"
#include "journal/journal.h"

#define DEFAULT_JOURNAL_PATH "journal/move.log"

AppContext ctx;

typedef struct
{
    const char *root;
    const char *config_path;
    int dry_run;
    int force;
    int mode_override;
    Mode mode;
    int workers_override;
} CliOptions;

static void handle_signal(int signo)
{
    (void)signo;
    atomic_store_explicit(&ctx.stop_signal, signo, memory_order_relaxed);
    atomic_store_explicit(&ctx.running, 0, memory_order_relaxed);
}

static int install_signal_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) != 0)
        return -1;
    if (sigaction(SIGTERM, &sa, NULL) != 0)
        return -1;
    return 0;
}

static void queue_backoff(void)
{
    atomic_fetch_add(&ctx.metrics.queue_backpressure_waits, 1);

    const struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = 1000000,
    };
    nanosleep(&delay, NULL);
}

static void init_metrics(AppContext *app_ctx)
{
    atomic_store(&app_ctx->metrics.events_seen, 0);
    atomic_store(&app_ctx->metrics.events_enqueued, 0);
    atomic_store(&app_ctx->metrics.queue_backpressure_waits, 0);
    atomic_store(&app_ctx->metrics.dry_run_moves, 0);
    atomic_store(&app_ctx->metrics.files_moved, 0);
    atomic_store(&app_ctx->metrics.move_failures, 0);
    atomic_store(&app_ctx->metrics.watcher_events, 0);
    atomic_store(&app_ctx->metrics.watcher_idle_polls, 0);
    atomic_store(&app_ctx->metrics.signals_received, 0);
    atomic_store(&app_ctx->worker_error, 0);
    atomic_store(&app_ctx->watcher_error, 0);
    atomic_store(&app_ctx->stop_signal, 0);
}

static void print_metrics(const AppContext *app_ctx)
{
    fprintf(stderr,
            "\n[METRICS]\n"
            "events_seen=%lu\n"
            "events_enqueued=%lu\n"
            "watcher_events=%lu\n"
            "dry_run_moves=%lu\n"
            "files_moved=%lu\n"
            "move_failures=%lu\n"
            "queue_backpressure_waits=%lu\n"
            "watcher_idle_polls=%lu\n"
            "signals_received=%lu\n",
            atomic_load(&app_ctx->metrics.events_seen),
            atomic_load(&app_ctx->metrics.events_enqueued),
            atomic_load(&app_ctx->metrics.watcher_events),
            atomic_load(&app_ctx->metrics.dry_run_moves),
            atomic_load(&app_ctx->metrics.files_moved),
            atomic_load(&app_ctx->metrics.move_failures),
            atomic_load(&app_ctx->metrics.queue_backpressure_waits),
            atomic_load(&app_ctx->metrics.watcher_idle_polls),
            atomic_load(&app_ctx->metrics.signals_received));
}

static void print_usage(const char *program)
{
    fprintf(stderr,
            "Usage: %s <root> [--config <file>] [--dry-run] [--once|--watch] [--workers <n>] [--force]\n"
            "       %s --root <root> [--config <file>] [--dry-run] [--once|--watch] [--workers <n>] [--force]\n",
            program, program);
}

static int parse_cli(int argc, char **argv, CliOptions *opts)
{
    opts->root = NULL;
    opts->config_path = "configs/default.conf";
    opts->dry_run = 0;
    opts->force = 0;
    opts->mode_override = 0;
    opts->mode = MODE_BATCH;
    opts->workers_override = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--root") == 0)
        {
            if (++i >= argc)
                return -1;
            opts->root = argv[i];
        }
        else if (strcmp(argv[i], "--config") == 0)
        {
            if (++i >= argc)
                return -1;
            opts->config_path = argv[i];
        }
        else if (strcmp(argv[i], "--dry-run") == 0)
            opts->dry_run = 1;
        else if (strcmp(argv[i], "--force") == 0)
            opts->force = 1;
        else if (strcmp(argv[i], "--once") == 0)
        {
            opts->mode_override = 1;
            opts->mode = MODE_BATCH;
        }
        else if (strcmp(argv[i], "--watch") == 0)
        {
            opts->mode_override = 1;
            opts->mode = MODE_DAEMON;
        }
        else if (strcmp(argv[i], "--workers") == 0)
        {
            if (++i >= argc)
                return -1;
            opts->workers_override = atoi(argv[i]);
        }
        else if (argv[i][0] == '-')
            return -1;
        else if (!opts->root)
            opts->root = argv[i];
        else
            return -1;
    }

    return opts->root ? 0 : -1;
}

static int path_is_within(const char *parent, const char *child)
{
    if (strcmp(parent, child) == 0)
        return 1;

    size_t parent_length = strlen(parent);
    if (parent_length == 1 && parent[0] == '/')
        return child[0] == '/';

    return strncmp(parent, child, parent_length) == 0 &&
           child[parent_length] == '/';
}

static int paths_overlap(const char *left, const char *right)
{
    return path_is_within(left, right) || path_is_within(right, left);
}

static int is_dangerous_root(const char *root)
{
    static const char *blocked[] = {
        "/", "/bin", "/boot", "/dev", "/etc", "/lib", "/lib64",
        "/proc", "/root", "/run", "/sbin", "/sys", "/usr", "/var",
        NULL,
    };

    if (strcmp(root, "/") == 0)
        return 1;

    for (int i = 1; blocked[i]; i++)
    {
        if (paths_overlap(root, blocked[i]))
            return 1;
    }

    const char *home = getenv("HOME");
    if (home)
    {
        char *canonical_home = realpath(home, NULL);
        if (canonical_home)
        {
            int overlaps_home = strcmp(root, canonical_home) == 0 ||
                                path_is_within(root, canonical_home);
            free(canonical_home);
            if (overlaps_home)
                return 1;
        }
    }

    return 0;
}

static int validate_root_output_overlap(const char *root, const char *cwd,
                                       const char *component)
{
    size_t cwd_length = strlen(cwd);
    size_t component_length = strlen(component);
    if (cwd_length > SIZE_MAX - component_length - 2)
    {
        errno = EOVERFLOW;
        return -1;
    }

    size_t output_capacity = cwd_length + component_length + 2;
    char *output_path = malloc(output_capacity);
    if (!output_path)
        return -1;

    int written = snprintf(output_path, output_capacity, "%s/%s", cwd, component);
    if (written < 0 || (size_t)written >= output_capacity)
    {
        free(output_path);
        errno = EOVERFLOW;
        return -1;
    }

    struct stat st;
    char *canonical_output = NULL;
    if (lstat(output_path, &st) == 0)
    {
        if (S_ISLNK(st.st_mode) || !S_ISDIR(st.st_mode))
        {
            fprintf(stderr,
                    "Refusing output path '%s': it must be a real directory, not a symlink or file.\n",
                    output_path);
            free(output_path);
            errno = ELOOP;
            return -1;
        }
        canonical_output = realpath(output_path, NULL);
        if (!canonical_output)
        {
            int saved_errno = errno;
            free(output_path);
            errno = saved_errno;
            return -1;
        }
    }
    else if (errno == ENOENT)
    {
        canonical_output = strdup(output_path);
        if (!canonical_output)
        {
            free(output_path);
            return -1;
        }
    }
    else
    {
        int saved_errno = errno;
        free(output_path);
        errno = saved_errno;
        return -1;
    }

    int overlaps = paths_overlap(root, canonical_output);
    if (overlaps)
    {
        fprintf(stderr,
                "Refusing input root '%s': it overlaps output directory '%s'. Choose a separate input folder or working directory.\n",
                root, canonical_output);
    }

    free(canonical_output);
    free(output_path);
    if (overlaps)
    {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

static int validate_root_and_outputs(CliOptions *opts, Config *cfg,
                                    char **canonical_root_out)
{
    if (!opts || !cfg || !canonical_root_out)
    {
        errno = EINVAL;
        return -1;
    }

    *canonical_root_out = NULL;

    char *root_argument = strdup(opts->root);
    if (!root_argument)
        return -1;
    size_t root_argument_length = strlen(root_argument);
    while (root_argument_length > 1 &&
           root_argument[root_argument_length - 1] == '/')
        root_argument[--root_argument_length] = '\0';

    struct stat root_lstat;
    if (lstat(root_argument, &root_lstat) != 0)
    {
        int saved_errno = errno;
        fprintf(stderr, "Input root '%s' is not accessible: %s\n",
                opts->root, strerror(saved_errno));
        free(root_argument);
        errno = saved_errno;
        return -1;
    }
    if (S_ISLNK(root_lstat.st_mode))
    {
        fprintf(stderr, "Refusing symlink input root '%s'. Use its real directory path.\n",
                opts->root);
        free(root_argument);
        errno = ELOOP;
        return -1;
    }

    char *canonical_root = realpath(root_argument, NULL);
    free(root_argument);
    if (!canonical_root)
        return -1;

    struct stat root_stat;
    if (stat(canonical_root, &root_stat) != 0 || !S_ISDIR(root_stat.st_mode))
    {
        fprintf(stderr, "Input root '%s' must be a directory.\n", canonical_root);
        free(canonical_root);
        errno = ENOTDIR;
        return -1;
    }

    if (!opts->force && is_dangerous_root(canonical_root))
    {
        fprintf(stderr, "Refusing dangerous input root '%s'. Pass --force only if you really intend this.\n",
                canonical_root);
        free(canonical_root);
        errno = EPERM;
        return -1;
    }

    char *canonical_cwd = realpath(".", NULL);
    if (!canonical_cwd)
    {
        int saved_errno = errno;
        free(canonical_root);
        errno = saved_errno;
        return -1;
    }

    int status = 0;
    /* Journal data is a protected output path even in dry-run mode. */
    if (validate_root_output_overlap(canonical_root, canonical_cwd, "journal") != 0)
        status = -1;

    for (size_t i = 0; status == 0 && i < cfg->ext_map.capacity; i++)
    {
        const HashEntry *entry = &cfg->ext_map.entries[i];
        if (!entry->used)
            continue;

        char folder[MAX_VAL_LEN];
        size_t length = strlen(entry->value);
        if (length > 0 && entry->value[length - 1] == '/')
            length--;
        if (length == 0 || length >= sizeof(folder))
        {
            errno = EINVAL;
            status = -1;
            break;
        }
        memcpy(folder, entry->value, length);
        folder[length] = '\0';

        if (validate_root_output_overlap(canonical_root, canonical_cwd, folder) != 0)
            status = -1;
    }

    if (status == 0 &&
        validate_root_output_overlap(canonical_root, canonical_cwd, "Others") != 0)
        status = -1;

    free(canonical_cwd);
    if (status != 0)
    {
        free(canonical_root);
        return -1;
    }

    *canonical_root_out = canonical_root;
    return 0;
}

void *watcher_thread(void *arg)
{
    if (start_watcher((const char *)arg, &ctx) != 0)
    {
        int saved_errno = errno;
        fprintf(stderr, "Watcher failed: %s\n", strerror(saved_errno));
        atomic_store(&ctx.watcher_error, saved_errno ? saved_errno : EIO);
        atomic_store(&ctx.running, 0);
    }
    return NULL;
}

int enqueue_event(const char *path)
{
    atomic_fetch_add(&ctx.metrics.events_seen, 1);

    if (atomic_load(&ctx.stop_signal) != 0)
    {
        errno = ECANCELED;
        return -1;
    }

    if (!path || strlen(path) >= sizeof(((FileEvent *)0)->path))
    {
        errno = ENAMETOOLONG;
        return -1;
    }

    FileEvent event = {0};
    memcpy(event.path, path, strlen(path) + 1);
    event.type = FILE_CREATED;

    int index = dispatch(&ctx, path);
    if (index < 0)
        return -1;

    int queued = 0;
    while (atomic_load(&ctx.running) && atomic_load(&ctx.stop_signal) == 0)
    {
        queued = lf_enqueue(&ctx.queues[index], &event);
        if (queued)
            break;
        queue_backoff();
    }

    if (!queued)
    {
        errno = ECANCELED;
        return -1;
    }

    atomic_fetch_add(&ctx.metrics.events_enqueued, 1);
    if (notify_worker(&ctx, index) != 0)
    {
        int saved_errno = errno;
        fprintf(stderr, "Worker notification failed: %s\n", strerror(saved_errno));
        atomic_store(&ctx.worker_error, 1);
        atomic_store(&ctx.running, 0);
        errno = saved_errno;
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    CliOptions opts;
    if (parse_cli(argc, argv, &opts) != 0)
    {
        print_usage(argv[0]);
        return 1;
    }

    static HashEntry buffer[64];
    hashmap_init(&ctx.cfg.ext_map, buffer, 64);
    init_config(&ctx.cfg);

    if (load_config(opts.config_path, &ctx.cfg) != 0)
    {
        printf("Config load failed\n");
        return 1;
    }

    char *canonical_root = NULL;
    if (validate_root_and_outputs(&opts, &ctx.cfg, &canonical_root) != 0)
    {
        fprintf(stderr, "Input/output path validation failed: %s\n", strerror(errno));
        free_config(&ctx.cfg);
        return 1;
    }
    opts.root = canonical_root;

    if (opts.mode_override)
        ctx.cfg.mode = opts.mode;
    if (opts.workers_override > 0)
        ctx.cfg.worker_threads = opts.workers_override;
    if (ctx.cfg.worker_threads > MAX_WORKERS)
        ctx.cfg.worker_threads = MAX_WORKERS;

    atomic_store(&ctx.running, 1);
    atomic_store(&ctx.active_workers, 0);
    atomic_store(&ctx.scanning_done, 0);
    atomic_store(&ctx.dry_run, opts.dry_run);
    init_metrics(&ctx);

    if (install_signal_handlers() != 0)
    {
        fprintf(stderr, "Failed to install signal handlers: %s\n", strerror(errno));
        free(canonical_root);
        free_config(&ctx.cfg);
        return 1;
    }

    if (!opts.dry_run)
    {
        if (journal_recover(DEFAULT_JOURNAL_PATH) != 0)
        {
            fprintf(stderr, "Journal recovery failed for %s: %s\n",
                    DEFAULT_JOURNAL_PATH, strerror(errno));
            free(canonical_root);
            free_config(&ctx.cfg);
            return 1;
        }

        if (journal_open(&ctx.journal, DEFAULT_JOURNAL_PATH) != 0)
        {
            fprintf(stderr, "Failed to open move journal: %s\n", DEFAULT_JOURNAL_PATH);
            free(canonical_root);
            free_config(&ctx.cfg);
            return 1;
        }
    }

    classifier_init(&ctx.classifier, &ctx.cfg.ext_map, "Others/");

    for (int i = 0; i < ctx.cfg.worker_threads; i++)
        lf_init(&ctx.queues[i]);

    if (start_workers(&ctx, ctx.cfg.worker_threads) != 0)
    {
        fprintf(stderr, "Failed to start worker pool: %s\n", strerror(errno));
        (void)journal_close(&ctx.journal);
        free(canonical_root);
        free_config(&ctx.cfg);
        return 1;
    }

    int exit_status = 0;

    if (ctx.cfg.mode == MODE_BATCH)
    {
        printf("[MODE] BATCH%s\n", opts.dry_run ? " (dry-run)" : "");
        fflush(stdout);

        int scan_status = scan_directory(opts.root, enqueue_event, &ctx.stop_signal);
        if (scan_status != 0)
        {
            int scan_error = errno;
            if (atomic_load(&ctx.stop_signal) != 0 && scan_error == ECANCELED)
                exit_status = 128 + atomic_load(&ctx.stop_signal);
            else
            {
                fprintf(stderr, "Directory scan failed: %s\n", strerror(scan_error));
                exit_status = 1;
            }
        }
        if (atomic_load(&ctx.stop_signal) != 0)
        {
            atomic_store(&ctx.metrics.signals_received, 1);
            if (exit_status == 0)
                exit_status = 128 + atomic_load(&ctx.stop_signal);
        }

        atomic_store(&ctx.scanning_done, 1);

        if (join_workers(&ctx) != 0)
        {
            fprintf(stderr, "Worker shutdown failed: %s\n", strerror(errno));
            exit_status = 1;
        }
        if (atomic_load(&ctx.worker_error) != 0)
            exit_status = 1;

        print_metrics(&ctx);
        printf("✅ Batch complete\n");
    }
    else
    {
        printf("[MODE] DAEMON%s\n", opts.dry_run ? " (dry-run)" : "");
        fflush(stdout);

        pthread_t tid;
        int create_error = pthread_create(&tid, NULL, watcher_thread, (void *)opts.root);
        if (create_error != 0)
        {
            fprintf(stderr, "Failed to start watcher thread: %s\n", strerror(create_error));
            exit_status = 1;
        }
        else
        {
            int join_error = pthread_join(tid, NULL);
            if (join_error != 0)
            {
                fprintf(stderr, "Failed to join watcher thread: %s\n", strerror(join_error));
                exit_status = 1;
            }
        }

        if (join_workers(&ctx) != 0)
        {
            fprintf(stderr, "Worker shutdown failed: %s\n", strerror(errno));
            exit_status = 1;
        }
        if (atomic_load(&ctx.worker_error) != 0 ||
            atomic_load(&ctx.watcher_error) != 0)
            exit_status = 1;
        if (atomic_load(&ctx.stop_signal) != 0)
        {
            atomic_store(&ctx.metrics.signals_received, 1);
            exit_status = 128 + atomic_load(&ctx.stop_signal);
        }
        print_metrics(&ctx);
    }

    if (journal_close(&ctx.journal) != 0)
    {
        fprintf(stderr, "Failed to flush move journal: %s\n", strerror(errno));
        exit_status = 1;
    }
    free(canonical_root);
    free_config(&ctx.cfg);

    return exit_status;
}
