#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "journal/journal.h"
#include "mover/mover.h"

#define CHECK(expression) do { \
    if (!(expression)) { \
        int saved_errno = errno; \
        fprintf(stderr, "CHECK failed at %s:%d: %s (errno=%d: %s)\n", \
                __FILE__, __LINE__, #expression, saved_errno, strerror(saved_errno)); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

typedef struct {
    int stage;
    const char *label;
    int source_expected;
    int destination_expected;
    size_t expected_orphan_temp_files;
} CrashCase;

static void make_path(char output[PATH_MAX], const char *parent, const char *leaf)
{
    int n = snprintf(output, PATH_MAX, "%s/%s", parent, leaf);
    CHECK(n > 0 && n < PATH_MAX);
}

static void write_file(const char *path, const char *contents)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    CHECK(fd >= 0);
    size_t length = strlen(contents);
    size_t offset = 0;
    while (offset < length) {
        ssize_t count = write(fd, contents + offset, length - offset);
        if (count < 0 && errno == EINTR)
            continue;
        CHECK(count > 0);
        offset += (size_t)count;
    }
    CHECK(fsync(fd) == 0);
    CHECK(close(fd) == 0);
}

static void assert_file_contents(const char *path, const char *expected)
{
    char actual[256];
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    CHECK(fd >= 0);
    ssize_t count;
    do {
        count = read(fd, actual, sizeof(actual) - 1U);
    } while (count < 0 && errno == EINTR);
    CHECK(count >= 0);
    actual[count] = '\0';
    CHECK(close(fd) == 0);
    CHECK(strcmp(actual, expected) == 0);
}

static int path_exists(const char *path)
{
    struct stat st;
    if (lstat(path, &st) == 0)
        return 1;
    CHECK(errno == ENOENT);
    return 0;
}

static size_t count_recovered_records(const char *journal_path)
{
    FILE *stream = fopen(journal_path, "r");
    CHECK(stream != NULL);
    char *line = NULL;
    size_t capacity = 0;
    size_t count = 0;
    ssize_t length;
    while ((length = getline(&line, &capacity, stream)) >= 0) {
        (void)length;
        if (strstr(line, "|RECOVERED|") != NULL)
            count++;
    }
    CHECK(!ferror(stream));
    free(line);
    CHECK(fclose(stream) == 0);
    return count;
}


static size_t remove_child_temp_files(const char *directory, pid_t child_pid)
{
    char prefix[64];
    int n = snprintf(prefix, sizeof(prefix), ".fc-tmp-%ld-", (long)child_pid);
    CHECK(n > 0 && (size_t)n < sizeof(prefix));

    DIR *stream = opendir(directory);
    CHECK(stream != NULL);
    int directory_fd = dirfd(stream);
    CHECK(directory_fd >= 0);

    size_t count = 0;
    struct dirent *entry;
    while ((entry = readdir(stream)) != NULL) {
        if (strncmp(entry->d_name, prefix, strlen(prefix)) == 0) {
            CHECK(unlinkat(directory_fd, entry->d_name, 0) == 0);
            count++;
        }
    }
    CHECK(closedir(stream) == 0);
    return count;
}

static void run_crash_case(const char *root, const CrashCase *test_case)
{
    char source_dir[PATH_MAX], destination_dir[PATH_MAX];
    char source[PATH_MAX], destination[PATH_MAX], journal_path[PATH_MAX];
    char source_dir_name[32], destination_dir_name[32], source_name[64];

    int n = snprintf(source_dir_name, sizeof(source_dir_name), "source-%s", test_case->label);
    CHECK(n > 0 && (size_t)n < sizeof(source_dir_name));
    n = snprintf(destination_dir_name, sizeof(destination_dir_name), "destination-%s", test_case->label);
    CHECK(n > 0 && (size_t)n < sizeof(destination_dir_name));
    n = snprintf(source_name, sizeof(source_name), "payload-%s.bin", test_case->label);
    CHECK(n > 0 && (size_t)n < sizeof(source_name));

    make_path(source_dir, root, source_dir_name);
    make_path(destination_dir, root, destination_dir_name);
    make_path(journal_path, root, test_case->label);
    CHECK(mkdir(source_dir, 0700) == 0);
    CHECK(mkdir(destination_dir, 0700) == 0);
    make_path(source, source_dir, source_name);
    make_path(destination, destination_dir, source_name);
    write_file(source, "crash-recovery-payload-v1");

    pid_t child = fork();
    CHECK(child >= 0);
    if (child == 0) {
        MoveJournal journal;
        MoveResult result;
        if (journal_open(&journal, journal_path) != 0)
            _exit(101);
        fc_test_mover_stop_at_crash_stage(test_case->stage);
        (void)move_file_journaled(source, destination_dir, &journal, &result);
        (void)journal_close(&journal);
        _exit(102); /* Reaching this means the requested checkpoint did not fire. */
    }

    int status = 0;
    pid_t waited = waitpid(child, &status, WUNTRACED);
    CHECK(waited == child);
    CHECK(WIFSTOPPED(status));
    CHECK(kill(child, SIGKILL) == 0);
    CHECK(waitpid(child, &status, 0) == child);
    CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL);
    CHECK(remove_child_temp_files(destination_dir, child) ==
          test_case->expected_orphan_temp_files);

    /* A fresh process context reopens the journal after the killed process
       releases its flock. Recovery is deliberately run twice. */
    CHECK(journal_recover(journal_path) == 0);
    CHECK(path_exists(source) == test_case->source_expected);
    CHECK(path_exists(destination) == test_case->destination_expected);

    if (test_case->source_expected)
        assert_file_contents(source, "crash-recovery-payload-v1");
    if (test_case->destination_expected)
        assert_file_contents(destination, "crash-recovery-payload-v1");

    CHECK(count_recovered_records(journal_path) == 1U);
    struct stat before, after;
    CHECK(stat(journal_path, &before) == 0);
    CHECK(journal_recover(journal_path) == 0);
    CHECK(stat(journal_path, &after) == 0);
    CHECK(before.st_size == after.st_size);
    CHECK(count_recovered_records(journal_path) == 1U);

    CHECK(unlink(journal_path) == 0);
    if (test_case->source_expected)
        CHECK(unlink(source) == 0);
    if (test_case->destination_expected)
        CHECK(unlink(destination) == 0);
    CHECK(rmdir(source_dir) == 0);
    CHECK(rmdir(destination_dir) == 0);
}

int main(void)
{
    char root_template[] = "/tmp/fc-process-crash-XXXXXX";
    char *root = mkdtemp(root_template);
    CHECK(root != NULL);
    (void)alarm(30);

    static const CrashCase cases[] = {
        {FC_TEST_CRASH_AFTER_STARTED, "started", 1, 0, 1},
        {FC_TEST_CRASH_AFTER_PUBLISHED, "published", 1, 1, 0},
        {FC_TEST_CRASH_AFTER_SOURCE_RETIRED, "retired", 0, 1, 0}
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
        run_crash_case(root, &cases[i]);

    CHECK(rmdir(root) == 0);
    puts("process crash/restart recovery tests passed");
    return EXIT_SUCCESS;
}
