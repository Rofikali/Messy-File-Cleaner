#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdatomic.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>

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
    atomic_fetch_add(&ctx.metrics.signals_received, 1);
    atomic_store(&ctx.running, 0);
}

static void install_signal_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
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

static int is_dangerous_root(const char *root)
{
    static const char *blocked[] = {
        "/", "/bin", "/boot", "/dev", "/etc", "/lib", "/lib64",
        "/proc", "/root", "/run", "/sbin", "/sys", "/usr", "/var",
        NULL,
    };

    for (int i = 0; blocked[i]; i++)
    {
        if (strcmp(root, blocked[i]) == 0)
            return 1;
    }

    const char *home = getenv("HOME");
    return home && strcmp(root, home) == 0;
}

void *watcher_thread(void *arg)
{
    start_watcher((const char *)arg, &ctx);
    return NULL;
}

void enqueue_event(const char *path)
{
    atomic_fetch_add(&ctx.metrics.events_seen, 1);

    FileEvent e;

    strncpy(e.path, path, sizeof(e.path) - 1);
    e.path[sizeof(e.path) - 1] = '\0';
    e.type = FILE_CREATED;

    int idx = dispatch(&ctx, path);

    int queued = 0;
    while (atomic_load(&ctx.running))
    {
        queued = lf_enqueue(&ctx.queues[idx], &e);
        if (queued)
            break;
        queue_backoff();
    }

    if (!queued)
        return;

    atomic_fetch_add(&ctx.metrics.events_enqueued, 1);

    uint64_t one = 1;
    write(ctx.efd[idx], &one, sizeof(one));
}

int queues_empty()
{
    for (int i = 0; i < ctx.num_workers; i++)
    {
        if (!lf_is_empty(&ctx.queues[i]))
            return 0;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    CliOptions opts;
    if (parse_cli(argc, argv, &opts) != 0)
    {
        print_usage(argv[0]);
        return 1;
    }

    if (!opts.force && is_dangerous_root(opts.root))
    {
        fprintf(stderr, "Refusing dangerous root '%s'. Pass --force only if you really intend this.\n", opts.root);
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

    install_signal_handlers();

    if (!opts.dry_run)
    {
        if (journal_recover(DEFAULT_JOURNAL_PATH) != 0)
            fprintf(stderr, "Warning: journal recovery failed for %s\n", DEFAULT_JOURNAL_PATH);

        if (journal_open(&ctx.journal, DEFAULT_JOURNAL_PATH) != 0)
        {
            fprintf(stderr, "Failed to open move journal: %s\n", DEFAULT_JOURNAL_PATH);
            return 1;
        }
    }

    classifier_init(&ctx.classifier, &ctx.cfg.ext_map, "Others/");

    for (int i = 0; i < ctx.cfg.worker_threads; i++)
        lf_init(&ctx.queues[i]);

    start_workers(&ctx, ctx.cfg.worker_threads);

    if (ctx.cfg.mode == MODE_BATCH)
    {
        printf("[MODE] BATCH%s\n", opts.dry_run ? " (dry-run)" : "");
        fflush(stdout);

        scan_directory(opts.root, enqueue_event);

        atomic_store(&ctx.scanning_done, 1);

        while (1)
        {
            if (atomic_load(&ctx.scanning_done) &&
                atomic_load(&ctx.active_workers) == 0 &&
                queues_empty())
                break;
        }

        atomic_store(&ctx.running, 0);

        for (int i = 0; i < ctx.num_workers; i++)
        {
            uint64_t one = 1;
            write(ctx.efd[i], &one, sizeof(one));
        }

        join_workers(&ctx);

        print_metrics(&ctx);
        printf("✅ Batch complete\n");
    }
    else
    {
        printf("[MODE] DAEMON%s\n", opts.dry_run ? " (dry-run)" : "");
        fflush(stdout);

        pthread_t tid;
        pthread_create(&tid, NULL, watcher_thread, (void *)opts.root);
        pthread_join(tid, NULL);

        atomic_store(&ctx.running, 0);
        for (int i = 0; i < ctx.num_workers; i++)
        {
            uint64_t one = 1;
            write(ctx.efd[i], &one, sizeof(one));
        }

        join_workers(&ctx);
        print_metrics(&ctx);
    }

    journal_close(&ctx.journal);
    free_config(&ctx.cfg);

    return 0;
}
