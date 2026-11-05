#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_FILE_COUNT UINT64_C(1000)
#define DEFAULT_THREAD_COUNT 4U
#define MAX_FILE_COUNT UINT64_C(1000000)
#define MAX_THREAD_COUNT 64U

static const char *const extensions[] = {
    ".jpg", ".jpeg", ".png", ".html", ".js", ".java"
};

typedef struct {
    int root_fd;
    uint64_t first;
    uint64_t limit;
    uint64_t seed;
    _Atomic int *stop;
    _Atomic int *first_error;
    _Atomic uint64_t *created;
} worker_args_t;

static int parse_u64(const char *text, uint64_t *value)
{
    char *end = NULL;
    unsigned long long parsed;

    if (text == NULL || text[0] == '\0' || text[0] == '-') {
        return -1;
    }

    errno = 0;
    parsed = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') {
        return -1;
    }

    *value = (uint64_t)parsed;
    return 0;
}

/* A deterministic integer mixer avoids shared PRNG state and lock contention. */
static uint64_t mix64(uint64_t value)
{
    value ^= value >> 30;
    value *= UINT64_C(0xbf58476d1ce4e5b9);
    value ^= value >> 27;
    value *= UINT64_C(0x94d049bb133111eb);
    value ^= value >> 31;
    return value;
}

static void record_error(worker_args_t *args, int error_code)
{
    int expected = 0;

    (void)atomic_compare_exchange_strong(args->first_error, &expected,
                                         error_code != 0 ? error_code : EIO);
    atomic_store(args->stop, 1);
}

static void *create_files(void *opaque)
{
    worker_args_t *args = opaque;
    uint64_t index;

    for (index = args->first; index < args->limit; ++index) {
        char filename[64];
        const size_t extension_count =
            sizeof(extensions) / sizeof(extensions[0]);
        const size_t extension_index =
            (size_t)(mix64(index ^ args->seed) % extension_count);
        int length;
        int file_fd;

        if (atomic_load(args->stop) != 0) {
            break;
        }

        length = snprintf(filename, sizeof(filename), "file_%010" PRIu64 "%s",
                          index, extensions[extension_index]);
        if (length < 0 || (size_t)length >= sizeof(filename)) {
            record_error(args, EOVERFLOW);
            break;
        }

        file_fd = openat(args->root_fd, filename,
                         O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
                         S_IRUSR | S_IWUSR);
        if (file_fd < 0) {
            record_error(args, errno);
            break;
        }

        if (close(file_fd) != 0) {
            record_error(args, errno);
            break;
        }

        (void)atomic_fetch_add(args->created, 1);
    }

    return NULL;
}

static void print_usage(const char *program)
{
    fprintf(stderr,
            "Usage: %s [--output DIR] [--files COUNT] [--threads COUNT] [--seed N]\n"
            "Creates empty files in a new directory. Existing output paths are never reused.\n"
            "  --output   Output directory (default: generated_files)\n"
            "  --files    Number of files, 1..%" PRIu64 " (default: %" PRIu64 ")\n"
            "  --threads  Worker threads, 1..%u (default: %u)\n"
            "  --seed     Deterministic extension-distribution seed (default: 1)\n",
            program, MAX_FILE_COUNT, DEFAULT_FILE_COUNT,
            MAX_THREAD_COUNT, DEFAULT_THREAD_COUNT);
}

int main(int argc, char **argv)
{
    const char *output_dir = "generated_files";
    uint64_t file_count = DEFAULT_FILE_COUNT;
    uint64_t requested_threads = DEFAULT_THREAD_COUNT;
    uint64_t seed = 1;
    uint64_t created = 0;
    uint64_t next_index = 0;
    unsigned int thread_count;
    unsigned int started = 0;
    pthread_t *threads = NULL;
    worker_args_t *workers = NULL;
    _Atomic int stop = 0;
    _Atomic int first_error = 0;
    _Atomic uint64_t created_atomic = 0;
    struct timespec start_time;
    struct timespec end_time;
    int root_fd = -1;
    int exit_status = EXIT_FAILURE;
    int option_index;

    for (option_index = 1; option_index < argc; ++option_index) {
        const char *option = argv[option_index];
        uint64_t parsed;

        if (strcmp(option, "--help") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        }
        if (option_index + 1 >= argc) {
            fprintf(stderr, "Missing value for %s\n", option);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }

        if (strcmp(option, "--output") == 0) {
            output_dir = argv[++option_index];
        } else if (strcmp(option, "--files") == 0) {
            if (parse_u64(argv[++option_index], &parsed) != 0 ||
                parsed == 0 || parsed > MAX_FILE_COUNT) {
                fprintf(stderr, "Invalid --files value\n");
                return EXIT_FAILURE;
            }
            file_count = parsed;
        } else if (strcmp(option, "--threads") == 0) {
            if (parse_u64(argv[++option_index], &parsed) != 0 ||
                parsed == 0 || parsed > MAX_THREAD_COUNT) {
                fprintf(stderr, "Invalid --threads value\n");
                return EXIT_FAILURE;
            }
            requested_threads = parsed;
        } else if (strcmp(option, "--seed") == 0) {
            if (parse_u64(argv[++option_index], &parsed) != 0) {
                fprintf(stderr, "Invalid --seed value\n");
                return EXIT_FAILURE;
            }
            seed = parsed;
        } else {
            fprintf(stderr, "Unknown option: %s\n", option);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (output_dir[0] == '\0') {
        fprintf(stderr, "Output directory must not be empty\n");
        return EXIT_FAILURE;
    }

    thread_count = (unsigned int)requested_threads;
    if ((uint64_t)thread_count > file_count) {
        thread_count = (unsigned int)file_count;
    }

    /* mkdir() intentionally fails with EEXIST: never reuse or truncate a fixture. */
    if (mkdir(output_dir, S_IRWXU) != 0) {
        fprintf(stderr, "Cannot create output directory '%s': %s\n",
                output_dir, strerror(errno));
        return EXIT_FAILURE;
    }

    root_fd = open(output_dir, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (root_fd < 0) {
        fprintf(stderr, "Cannot safely open output directory '%s': %s\n",
                output_dir, strerror(errno));
        goto cleanup;
    }

    threads = calloc(thread_count, sizeof(*threads));
    workers = calloc(thread_count, sizeof(*workers));
    if (threads == NULL || workers == NULL) {
        fprintf(stderr, "Unable to allocate worker state\n");
        goto cleanup;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &start_time) != 0) {
        fprintf(stderr, "clock_gettime failed: %s\n", strerror(errno));
        goto cleanup;
    }

    for (unsigned int i = 0; i < thread_count; ++i) {
        const uint64_t remaining = file_count - next_index;
        const uint64_t workers_left = (uint64_t)(thread_count - i);
        const uint64_t chunk = remaining / workers_left;

        workers[i].root_fd = root_fd;
        workers[i].first = next_index;
        workers[i].limit = next_index + chunk;
        workers[i].seed = seed;
        workers[i].stop = &stop;
        workers[i].first_error = &first_error;
        workers[i].created = &created_atomic;
        next_index += chunk;

        {
            const int thread_error = pthread_create(&threads[i], NULL,
                                                    create_files, &workers[i]);
            if (thread_error != 0) {
                fprintf(stderr, "pthread_create failed: %s\n",
                        strerror(thread_error));
                atomic_store(&first_error, thread_error);
                atomic_store(&stop, 1);
                break;
            }
        }
        ++started;
    }

    for (unsigned int i = 0; i < started; ++i) {
        const int join_error = pthread_join(threads[i], NULL);
        if (join_error != 0) {
            fprintf(stderr, "pthread_join failed: %s\n", strerror(join_error));
            if (atomic_load(&first_error) == 0) {
                atomic_store(&first_error, join_error);
            }
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end_time) != 0) {
        fprintf(stderr, "clock_gettime failed: %s\n", strerror(errno));
        goto cleanup;
    }

    created = atomic_load(&created_atomic);
    if (atomic_load(&first_error) != 0 || created != file_count ||
        started != thread_count) {
        const int error_code = atomic_load(&first_error);
        fprintf(stderr,
                "Generation incomplete: created=%" PRIu64 "/%" PRIu64
                ", workers_started=%u/%u, error=%s\n",
                created, file_count, started, thread_count,
                error_code != 0 ? strerror(error_code) : "incomplete work");
        fprintf(stderr, "Partial output was left untouched at '%s'\n", output_dir);
        goto cleanup;
    }

    {
        int64_t elapsed_seconds = (int64_t)end_time.tv_sec -
                                  (int64_t)start_time.tv_sec;
        int64_t elapsed_nanoseconds = (int64_t)end_time.tv_nsec -
                                      (int64_t)start_time.tv_nsec;
        uint64_t elapsed_ns;

        if (elapsed_nanoseconds < 0) {
            --elapsed_seconds;
            elapsed_nanoseconds += INT64_C(1000000000);
        }
        elapsed_ns = (uint64_t)elapsed_seconds * UINT64_C(1000000000) +
                     (uint64_t)elapsed_nanoseconds;

        printf("Created %" PRIu64 " empty files in '%s' using %u threads\n",
               created, output_dir, thread_count);
        printf("Elapsed: %" PRIu64 " ns (%.6f s); throughput: %.2f files/s\n",
               elapsed_ns, (double)elapsed_ns / 1e9,
               elapsed_ns != 0
                   ? (double)created * 1e9 / (double)elapsed_ns
                   : 0.0);
    }

    exit_status = EXIT_SUCCESS;

cleanup:
    free(workers);
    free(threads);
    if (root_fd >= 0 && close(root_fd) != 0) {
        fprintf(stderr, "close(output directory) failed: %s\n", strerror(errno));
        exit_status = EXIT_FAILURE;
    }
    return exit_status;
}
