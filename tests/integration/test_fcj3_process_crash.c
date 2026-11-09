#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "journal/fcj3_journal.h"
#include "journal/fcj3_parser.h"
#include "journal/fcj3_stage.h"
#include "journal/fcj3_writer.h"

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

#define CHECK(expr) do { \
    if (!(expr)) { \
        int saved = errno; \
        fprintf(stderr, "CHECK failed %s:%d: %s (errno=%d: %s)\n", \
                __FILE__, __LINE__, #expr, saved, strerror(saved)); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

#define PAYLOAD "fcj3-process-crash-payload-v1"
#define JOURNAL_NAME "fixture.fcj3"
#define SOURCE_NAME "source.bin"
#define STAGE_NAME ".fc-tmp-crash"
#define DEST_NAME "destination.bin"

typedef enum {
    CRASH_AFTER_STAGED = 1,
    CRASH_AFTER_LINK,
    CRASH_AFTER_STARTED,
    CRASH_AFTER_PUBLISH,
    CRASH_AFTER_PUBLISHED
} CrashPoint;

typedef struct {
    CrashPoint point;
    Fcj3State expected_state;
    int stage_expected;
    int destination_expected;
} CrashCase;

static void write_payload_at(int dirfd, const char *name)
{
    int fd = openat(dirfd, name,
                    O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
                    0600);
    CHECK(fd >= 0);
    const size_t length = sizeof(PAYLOAD) - 1U;
    size_t offset = 0U;
    while (offset < length) {
        ssize_t n = write(fd, PAYLOAD + offset, length - offset);
        if (n < 0 && errno == EINTR)
            continue;
        CHECK(n > 0);
        offset += (size_t)n;
    }
    CHECK(fsync(fd) == 0);
    CHECK(close(fd) == 0);
}

static void assert_payload_at(int dirfd, const char *name)
{
    char actual[sizeof(PAYLOAD) + 8U];
    int fd = openat(dirfd, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    CHECK(fd >= 0);
    size_t offset = 0U;
    while (offset < sizeof(actual) - 1U) {
        ssize_t n = read(fd, actual + offset, sizeof(actual) - 1U - offset);
        if (n < 0 && errno == EINTR)
            continue;
        CHECK(n >= 0);
        if (n == 0)
            break;
        offset += (size_t)n;
    }
    actual[offset] = '\0';
    CHECK(close(fd) == 0);
    CHECK(strcmp(actual, PAYLOAD) == 0);
}

static int entry_exists(int dirfd, const char *name, struct stat *metadata)
{
    if (fstatat(dirfd, name, metadata, AT_SYMLINK_NOFOLLOW) == 0)
        return 1;
    CHECK(errno == ENOENT);
    return 0;
}

static void checkpoint(CrashPoint requested, CrashPoint current)
{
    if (requested == current) {
        (void)kill(getpid(), SIGSTOP);
        _exit(90);
    }
}

static void append_state(Fcj3Journal *journal,
                         Fcj3OperationContext *context,
                         Fcj3State state, uint64_t timestamp)
{
    Fcj3PreparedRecord prepared;
    CHECK(fcj3_operation_prepare_transition(context, state, 0,
                                             timestamp, &prepared) == 0);
    CHECK(fcj3_journal_append_transition(journal, context, &prepared) == 0);
}

static void child_transaction(const char *root, CrashPoint requested)
{
    int rootfd = open(root, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (rootfd < 0)
        _exit(10);
    int sourcefd = openat(rootfd, SOURCE_NAME,
                          O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (sourcefd < 0)
        _exit(11);

    struct stat directory_metadata;
    if (fstat(rootfd, &directory_metadata) != 0)
        _exit(12);

    Fcj3Stage stage;
    if (fcj3_stage_open_at(&stage, rootfd, STAGE_NAME, geteuid()) != 0)
        _exit(13);

    struct stat source_snapshot;
    uint64_t bytes_copied = 0U;
    if (fcj3_stage_copy_from(&stage, sourcefd, &source_snapshot,
                             &bytes_copied) != 0)
        _exit(14);
    if (bytes_copied != sizeof(PAYLOAD) - 1U)
        _exit(15);

    struct stat stage_metadata;
    if (fcj3_stage_sync(&stage, &stage_metadata) != 0)
        _exit(16);

    char source_path[PATH_MAX];
    char destination_path[PATH_MAX];
    int source_length = snprintf(source_path, sizeof(source_path), "%s/%s",
                                 root, SOURCE_NAME);
    int destination_length = snprintf(destination_path, sizeof(destination_path),
                                      "%s/%s", root, DEST_NAME);
    if (source_length <= 0 || (size_t)source_length >= sizeof(source_path) ||
        destination_length <= 0 ||
        (size_t)destination_length >= sizeof(destination_path))
        _exit(17);

    Fcj3OperationContext context;
    if (fcj3_operation_init(&context, source_path, destination_path,
                            &directory_metadata, STAGE_NAME,
                            &stage_metadata) != 0)
        _exit(18);

    Fcj3Journal journal;
    if (fcj3_journal_open_at(&journal, rootfd, JOURNAL_NAME, geteuid()) != 0)
        _exit(18);

    append_state(&journal, &context, FCJ3_STATE_STAGED, 100U);
    checkpoint(requested, CRASH_AFTER_STAGED);

    if (fcj3_stage_link(&stage) != 0)
        _exit(19);
    checkpoint(requested, CRASH_AFTER_LINK);

    append_state(&journal, &context, FCJ3_STATE_STARTED, 200U);
    checkpoint(requested, CRASH_AFTER_STARTED);

    if (fcj3_stage_publish(&stage, DEST_NAME) != 0)
        _exit(20);
    checkpoint(requested, CRASH_AFTER_PUBLISH);

    append_state(&journal, &context, FCJ3_STATE_PUBLISHED, 300U);
    checkpoint(requested, CRASH_AFTER_PUBLISHED);

    _exit(21); /* Every requested checkpoint must stop the child first. */
}

static void load_replay(const char *journal_path, Fcj3Replay *replay)
{
    FILE *stream = fopen(journal_path, "r");
    CHECK(stream != NULL);
    fcj3_replay_init(replay);

    char *line = NULL;
    size_t capacity = 0U;
    ssize_t length;
    while ((length = getline(&line, &capacity, stream)) >= 0)
        CHECK(fcj3_replay_add_line(replay, line, (size_t)length) == 0);

    CHECK(!ferror(stream));
    free(line);
    CHECK(fclose(stream) == 0);
}

static void run_case(const char *root, const CrashCase *test_case)
{
    char source_path[PATH_MAX];
    char journal_path[PATH_MAX];
    int n = snprintf(source_path, sizeof(source_path), "%s/%s",
                     root, SOURCE_NAME);
    CHECK(n > 0 && (size_t)n < sizeof(source_path));
    n = snprintf(journal_path, sizeof(journal_path), "%s/%s",
                 root, JOURNAL_NAME);
    CHECK(n > 0 && (size_t)n < sizeof(journal_path));

    int rootfd = open(root, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    CHECK(rootfd >= 0);
    write_payload_at(rootfd, SOURCE_NAME);
    CHECK(close(rootfd) == 0);

    pid_t child = fork();
    CHECK(child >= 0);
    if (child == 0)
        child_transaction(root, test_case->point);

    int status = 0;
    CHECK(waitpid(child, &status, WUNTRACED) == child);
    CHECK(WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP);
    CHECK(kill(child, SIGKILL) == 0);
    CHECK(waitpid(child, &status, 0) == child);
    CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL);

    rootfd = open(root, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    CHECK(rootfd >= 0);
    struct stat stage_stat;
    struct stat destination_stat;
    int stage_exists = entry_exists(rootfd, STAGE_NAME, &stage_stat);
    int destination_exists = entry_exists(rootfd, DEST_NAME, &destination_stat);
    CHECK(stage_exists == test_case->stage_expected);
    CHECK(destination_exists == test_case->destination_expected);
    assert_payload_at(rootfd, SOURCE_NAME);
    if (stage_exists)
        assert_payload_at(rootfd, STAGE_NAME);
    if (destination_exists)
        assert_payload_at(rootfd, DEST_NAME);

    Fcj3Replay first;
    load_replay(journal_path, &first);
    CHECK(first.count == 1U);
    CHECK(first.transactions[0].state == test_case->expected_state);
    CHECK(first.transactions[0].sequence ==
          (test_case->expected_state == FCJ3_STATE_STAGED ? 1U :
           test_case->expected_state == FCJ3_STATE_STARTED ? 2U :
           3U));

    /* A visible name is trusted only when it is the exact recorded inode. */
    if (stage_exists) {
        CHECK(S_ISREG(stage_stat.st_mode));
        CHECK(stage_stat.st_dev == first.transactions[0].staging_dev);
        CHECK(stage_stat.st_ino == first.transactions[0].staging_ino);
    }
    if (destination_exists) {
        CHECK(S_ISREG(destination_stat.st_mode));
        CHECK(destination_stat.st_dev == first.transactions[0].staging_dev);
        CHECK(destination_stat.st_ino == first.transactions[0].staging_ino);
    }

    struct stat journal_before;
    struct stat journal_after;
    CHECK(stat(journal_path, &journal_before) == 0);
    Fcj3Replay second;
    load_replay(journal_path, &second);
    CHECK(second.count == first.count);
    CHECK(second.transactions[0].state == first.transactions[0].state);
    CHECK(second.transactions[0].sequence == first.transactions[0].sequence);
    CHECK(stat(journal_path, &journal_after) == 0);
    CHECK(journal_before.st_size == journal_after.st_size);

    /* Test-fixture teardown removes only exact names created by this case. */
    if (stage_exists)
        CHECK(unlinkat(rootfd, STAGE_NAME, 0) == 0);
    if (destination_exists)
        CHECK(unlinkat(rootfd, DEST_NAME, 0) == 0);
    CHECK(unlinkat(rootfd, SOURCE_NAME, 0) == 0);
    CHECK(unlinkat(rootfd, JOURNAL_NAME, 0) == 0);
    CHECK(close(rootfd) == 0);

    fcj3_replay_destroy(&first);
    fcj3_replay_destroy(&second);
}

int main(void)
{
    char root_template[] = "/tmp/fcj3-crash-matrix-XXXXXX";
    char *root = mkdtemp(root_template);
    CHECK(root != NULL);
    (void)alarm(45);

    static const CrashCase cases[] = {
        {CRASH_AFTER_STAGED, FCJ3_STATE_STAGED, 0, 0},
        {CRASH_AFTER_LINK, FCJ3_STATE_STAGED, 1, 0},
        {CRASH_AFTER_STARTED, FCJ3_STATE_STARTED, 1, 0},
        {CRASH_AFTER_PUBLISH, FCJ3_STATE_STARTED, 0, 1},
        {CRASH_AFTER_PUBLISHED, FCJ3_STATE_PUBLISHED, 0, 1}
    };

    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i)
        run_case(root, &cases[i]);

    CHECK(rmdir(root) == 0);
    puts("FCJ3 process-crash matrix passed");
    return EXIT_SUCCESS;
}
