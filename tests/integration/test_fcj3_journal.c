#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "journal/fcj3_journal.h"
#include "journal/fcj3_parser.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHECK(call) do { if ((call) != 0) { \
    fprintf(stderr, "FAIL: %s at %s:%d: %s\\n", #call, __FILE__, __LINE__, strerror(errno)); \
    abort(); \
} } while (0)

typedef struct
{
    char root[PATH_MAX];
    int directory_fd;
    struct stat directory_stat;
} Fixture;

static void fixture_init(Fixture *fixture)
{
    memset(fixture, 0, sizeof(*fixture));
    fixture->directory_fd = -1;
    memcpy(fixture->root, "/tmp/fcj3-journal-test-XXXXXX",
           sizeof("/tmp/fcj3-journal-test-XXXXXX"));
    assert(mkdtemp(fixture->root) != NULL);
    fixture->directory_fd = open(fixture->root,
        O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    assert(fixture->directory_fd >= 0);
    CHECK(fstat(fixture->directory_fd, &fixture->directory_stat));
    assert(S_ISDIR(fixture->directory_stat.st_mode));
}

static void fixture_destroy(Fixture *fixture, const char *journal_name)
{
    if (journal_name != NULL)
        assert(unlinkat(fixture->directory_fd, journal_name, 0) == 0);
    if (fixture->directory_fd >= 0)
        assert(close(fixture->directory_fd) == 0);
    assert(rmdir(fixture->root) == 0);
}

static void make_context(Fixture *fixture, Fcj3OperationContext *context,
                         const char *source_name, const char *destination_name)
{
    struct stat staging;
    memset(&staging, 0, sizeof(staging));
    staging.st_mode = S_IFREG | 0600;
    staging.st_uid = geteuid();
    staging.st_nlink = 0;
    staging.st_dev = fixture->directory_stat.st_dev;
    staging.st_ino = (ino_t)0x12345;

    char source[PATH_MAX];
    char destination[PATH_MAX];
    int source_length = snprintf(source, sizeof(source), "%s/%s",
                                 fixture->root, source_name);
    int destination_length = snprintf(destination, sizeof(destination), "%s/%s",
                                      fixture->root, destination_name);
    assert(source_length > 0 && (size_t)source_length < sizeof(source));
    assert(destination_length > 0 &&
           (size_t)destination_length < sizeof(destination));

    CHECK(fcj3_operation_init(context, source, destination,
        &fixture->directory_stat, ".fc-tmp-unit", &staging));
}

static size_t read_journal(Fixture *fixture, const char *name,
                           char *buffer, size_t capacity)
{
    int fd = openat(fixture->directory_fd, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    assert(fd >= 0);
    size_t offset = 0U;
    while (offset < capacity)
    {
        ssize_t count = read(fd, buffer + offset, capacity - offset);
        if (count < 0 && errno == EINTR)
            continue;
        assert(count >= 0);
        if (count == 0)
            break;
        offset += (size_t)count;
    }
    assert(close(fd) == 0);
    return offset;
}

static void test_durable_append_and_file_lock(void)
{
    Fixture fixture;
    Fcj3Journal journal;
    Fcj3Journal second;
    Fcj3OperationContext context;
    Fcj3PreparedRecord prepared;
    fixture_init(&fixture);

    CHECK(fcj3_journal_open_at(&journal, fixture.directory_fd,
                               "test.fcj3", geteuid()));
    make_context(&fixture, &context, "source-a", "destination-a");
    CHECK(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STAGED,
                                             0, 100U, &prepared));
    assert(fcj3_journal_append_transition(&journal, &context, &prepared) == 0);
    assert(context.sequence == 1U);
    assert(context.state == FCJ3_STATE_STAGED);

    errno = 0;
    assert(fcj3_journal_open_at(&second, fixture.directory_fd,
                                "test.fcj3", geteuid()) == -1);
    assert(errno == EWOULDBLOCK || errno == EAGAIN);

    /* A separately opened descriptor in a child process must also be excluded. */
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0)
    {
        Fcj3Journal child_journal;
        if (fcj3_journal_open_at(&child_journal, fixture.directory_fd,
                                 "test.fcj3", geteuid()) == 0)
            _exit(10);
        _exit((errno == EWOULDBLOCK || errno == EAGAIN) ? 0 : 11);
    }
    int child_status = 0;
    assert(waitpid(child, &child_status, 0) == child);
    assert(WIFEXITED(child_status) && WEXITSTATUS(child_status) == 0);

    CHECK(fcj3_journal_close(&journal));

    char bytes[FCJ3_MAX_LINE_BYTES + 1U];
    size_t length = read_journal(&fixture, "test.fcj3", bytes, sizeof(bytes));
    assert(length == prepared.length);
    assert(memcmp(bytes, prepared.line, length) == 0);

    fcj3_operation_reset(&context);
    fixture_destroy(&fixture, "test.fcj3");
}

static void test_invalid_record_rejected_without_poison(void)
{
    Fixture fixture;
    Fcj3Journal journal;
    Fcj3OperationContext context;
    Fcj3PreparedRecord prepared;
    fixture_init(&fixture);
    CHECK(fcj3_journal_open_at(&journal, fixture.directory_fd,
                               "invalid.fcj3", geteuid()));
    make_context(&fixture, &context, "source", "destination");
    CHECK(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STAGED,
                                             0, 200U, &prepared));

    Fcj3PreparedRecord invalid = prepared;
    invalid.length = 0U;
    errno = 0;
    assert(fcj3_journal_append_transition(&journal, &context, &invalid) == -1);
    assert(errno == EINVAL);
    assert(context.poisoned == 0);
    assert(context.sequence == 0U);

    CHECK(fcj3_journal_append_transition(&journal, &context, &prepared));
    CHECK(fcj3_journal_close(&journal));
    fcj3_operation_reset(&context);
    fixture_destroy(&fixture, "invalid.fcj3");
}

static void test_partial_write_poisons_context(void)
{
    Fixture fixture;
    Fcj3Journal journal;
    Fcj3OperationContext context;
    Fcj3PreparedRecord prepared;
    fixture_init(&fixture);
    CHECK(fcj3_journal_open_at(&journal, fixture.directory_fd,
                               "partial.fcj3", geteuid()));
    make_context(&fixture, &context, "source", "destination");
    CHECK(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STAGED,
                                             0, 300U, &prepared));

    fcj3_journal_test_fail_write_after(5U, EIO);
    errno = 0;
    assert(fcj3_journal_append_transition(&journal, &context, &prepared) == -1);
    assert(errno == EIO);
    assert(context.poisoned == 1);
    assert(context.sequence == 0U);

    Fcj3PreparedRecord next;
    errno = 0;
    assert(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STARTED,
                                              0, 301U, &next) == -1);
    assert(errno == EIO);

    fcj3_journal_test_fail_write_after(SIZE_MAX, 0);
    CHECK(fcj3_journal_close(&journal));
    char bytes[32];
    size_t length = read_journal(&fixture, "partial.fcj3", bytes, sizeof(bytes));
    assert(length == 5U);
    assert(memcmp(bytes, prepared.line, length) == 0);

    fcj3_operation_reset(&context);
    fixture_destroy(&fixture, "partial.fcj3");
}

static void test_fsync_failure_poisons_context(void)
{
    Fixture fixture;
    Fcj3Journal journal;
    Fcj3OperationContext context;
    Fcj3PreparedRecord prepared;
    fixture_init(&fixture);
    CHECK(fcj3_journal_open_at(&journal, fixture.directory_fd,
                               "fsync.fcj3", geteuid()));
    make_context(&fixture, &context, "source", "destination");
    CHECK(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STAGED,
                                             0, 400U, &prepared));

    fcj3_journal_test_fail_fsync(EIO);
    errno = 0;
    assert(fcj3_journal_append_transition(&journal, &context, &prepared) == -1);
    assert(errno == EIO);
    assert(context.poisoned == 1);
    assert(context.sequence == 0U);
    fcj3_journal_test_fail_fsync(0);

    CHECK(fcj3_journal_close(&journal));
    char bytes[FCJ3_MAX_LINE_BYTES + 1U];
    size_t length = read_journal(&fixture, "fsync.fcj3", bytes, sizeof(bytes));
    assert(length == prepared.length);
    assert(memcmp(bytes, prepared.line, length) == 0);

    fcj3_operation_reset(&context);
    fixture_destroy(&fixture, "fsync.fcj3");
}

typedef struct
{
    Fcj3Journal *journal;
    Fcj3OperationContext *context;
    Fcj3PreparedRecord *prepared;
    int result;
    int error_code;
} ThreadAppend;

static void *append_thread(void *argument)
{
    ThreadAppend *work = argument;
    work->result = fcj3_journal_append_transition(work->journal,
                                                  work->context,
                                                  work->prepared);
    work->error_code = errno;
    return NULL;
}

static void test_thread_appends_are_complete_records(void)
{
    Fixture fixture;
    Fcj3Journal journal;
    Fcj3OperationContext contexts[2];
    Fcj3PreparedRecord prepared[2];
    pthread_t threads[2];
    ThreadAppend work[2];
    fixture_init(&fixture);
    CHECK(fcj3_journal_open_at(&journal, fixture.directory_fd,
                               "threads.fcj3", geteuid()));

    make_context(&fixture, &contexts[0], "source-0", "destination-0");
    make_context(&fixture, &contexts[1], "source-1", "destination-1");
    CHECK(fcj3_operation_prepare_transition(&contexts[0], FCJ3_STATE_STAGED,
                                             0, 500U, &prepared[0]));
    CHECK(fcj3_operation_prepare_transition(&contexts[1], FCJ3_STATE_STAGED,
                                             0, 501U, &prepared[1]));

    for (size_t i = 0U; i < 2U; ++i)
    {
        work[i].journal = &journal;
        work[i].context = &contexts[i];
        work[i].prepared = &prepared[i];
        work[i].result = -1;
        work[i].error_code = 0;
        assert(pthread_create(&threads[i], NULL, append_thread, &work[i]) == 0);
    }
    for (size_t i = 0U; i < 2U; ++i)
    {
        assert(pthread_join(threads[i], NULL) == 0);
        assert(work[i].result == 0);
        assert(contexts[i].sequence == 1U);
    }

    CHECK(fcj3_journal_close(&journal));
    char bytes[(FCJ3_MAX_LINE_BYTES * 2U) + 2U];
    size_t length = read_journal(&fixture, "threads.fcj3", bytes, sizeof(bytes));
    assert(length == prepared[0].length + prepared[1].length);

    Fcj3Replay replay;
    fcj3_replay_init(&replay);
    size_t offset = 0U;
    while (offset < length)
    {
        char *newline = memchr(bytes + offset, '\n', length - offset);
        assert(newline != NULL);
        size_t line_length = (size_t)(newline - (bytes + offset)) + 1U;
        CHECK(fcj3_replay_add_line(&replay, bytes + offset, line_length));
        offset += line_length;
    }
    assert(replay.count == 2U);
    fcj3_replay_destroy(&replay);

    for (size_t i = 0U; i < 2U; ++i)
        fcj3_operation_reset(&contexts[i]);
    fixture_destroy(&fixture, "threads.fcj3");
}

static void test_rejects_symlink_and_unsafe_mode(void)
{
    Fixture fixture;
    Fcj3Journal journal;
    fixture_init(&fixture);

    int target_fd = openat(fixture.directory_fd, "target",
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    assert(target_fd >= 0);
    assert(close(target_fd) == 0);
    assert(symlinkat("target", fixture.directory_fd, "link.fcj3") == 0);
    errno = 0;
    assert(fcj3_journal_open_at(&journal, fixture.directory_fd,
                                "link.fcj3", geteuid()) == -1);
    assert(errno == ELOOP);

    int unsafe_fd = openat(fixture.directory_fd, "unsafe.fcj3",
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0644);
    assert(unsafe_fd >= 0);
    assert(close(unsafe_fd) == 0);
    errno = 0;
    assert(fcj3_journal_open_at(&journal, fixture.directory_fd,
                                "unsafe.fcj3", geteuid()) == -1);
    assert(errno == EPERM);

    assert(unlinkat(fixture.directory_fd, "link.fcj3", 0) == 0);
    assert(unlinkat(fixture.directory_fd, "unsafe.fcj3", 0) == 0);
    assert(unlinkat(fixture.directory_fd, "target", 0) == 0);
    fixture_destroy(&fixture, NULL);
}

int main(void)
{
    test_durable_append_and_file_lock();
    test_invalid_record_rejected_without_poison();
    test_partial_write_poisons_context();
    test_fsync_failure_poisons_context();
    test_thread_appends_are_complete_records();
    test_rejects_symlink_and_unsafe_mode();

    puts("FCJ3 isolated durable journal adapter tests passed");
    return 0;
}
