#define _GNU_SOURCE

#include "journal/fcj3_writer.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "journal/fcj3_parser.h"

#define CHECK_RESULT(expression, expected) do { \
    int check_result_ = (expression); \
    assert(check_result_ == (expected)); \
} while (0)

typedef struct
{
    char root[PATH_MAX];
    int directory_fd;
    int staging_fd;
    int journal_fd;
    char journal_path[PATH_MAX];
    struct stat directory_stat;
    struct stat staging_stat;
} Fixture;

static void make_child_path(char output[PATH_MAX],
                             const char *root, const char *leaf)
{
    size_t root_length = strlen(root);
    size_t leaf_length = strlen(leaf);
    assert(root_length + 1U + leaf_length < PATH_MAX);
    memcpy(output, root, root_length);
    output[root_length] = '/';
    memcpy(output + root_length + 1U, leaf, leaf_length + 1U);
}

static void fixture_init(Fixture *fixture)
{
    char template_path[] = "/tmp/fcj3-writer-XXXXXX";
    char *root = mkdtemp(template_path);
    assert(root != NULL);
    assert(strlen(root) < sizeof(fixture->root));
    (void)snprintf(fixture->root, sizeof(fixture->root), "%s", root);

    fixture->directory_fd = open(fixture->root,
        O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    assert(fixture->directory_fd >= 0);
    assert(fstat(fixture->directory_fd, &fixture->directory_stat) == 0);

    fixture->staging_fd = openat(fixture->directory_fd, ".",
        O_TMPFILE | O_RDWR | O_CLOEXEC, 0600);
    if (fixture->staging_fd < 0)
    {
        int saved_errno = errno;
        (void)close(fixture->directory_fd);
        (void)rmdir(fixture->root);
        if (saved_errno == EOPNOTSUPP || saved_errno == ENOTSUP ||
            saved_errno == EINVAL || saved_errno == EISDIR ||
            saved_errno == EPERM || saved_errno == ENOSYS)
        {
            fprintf(stderr, "SKIP: O_TMPFILE unavailable for FCJ3 context test: %s\\n",
                    strerror(saved_errno));
            exit(77);
        }
        errno = saved_errno;
        assert(fixture->staging_fd >= 0);
    }
    assert(fstat(fixture->staging_fd, &fixture->staging_stat) == 0);
    assert(fixture->staging_stat.st_nlink == 0);

    int length = snprintf(fixture->journal_path, sizeof(fixture->journal_path),
                      "%s/test.journal", fixture->root);
    assert(length > 0 && (size_t)length < sizeof(fixture->journal_path));
    fixture->journal_fd = open(fixture->journal_path,
        O_WRONLY | O_CREAT | O_EXCL | O_APPEND | O_CLOEXEC, 0600);
    assert(fixture->journal_fd >= 0);
}

static void durably_append_test_record(int journal_fd,
                                       const Fcj3PreparedRecord *prepared)
{
    size_t offset = 0;
    while (offset < prepared->length)
    {
        ssize_t written = write(journal_fd, prepared->line + offset,
                                prepared->length - offset);
        if (written < 0 && errno == EINTR)
            continue;
        assert(written > 0);
        offset += (size_t)written;
    }
    assert(fsync(journal_fd) == 0);
}

static void fixture_destroy(Fixture *fixture)
{
    if (fixture->staging_fd >= 0)
        assert(close(fixture->staging_fd) == 0);
    if (fixture->journal_fd >= 0)
        assert(close(fixture->journal_fd) == 0);
    if (fixture->directory_fd >= 0)
        assert(close(fixture->directory_fd) == 0);
    assert(unlink(fixture->journal_path) == 0);
    assert(rmdir(fixture->root) == 0);
}

static void assert_lower_hex_id(const char *id)
{
    assert(strlen(id) == FCJ3_OPERATION_ID_HEX_LENGTH);
    for (size_t i = 0; i < FCJ3_OPERATION_ID_HEX_LENGTH; i++)
    {
        assert((id[i] >= '0' && id[i] <= '9') ||
               (id[i] >= 'a' && id[i] <= 'f'));
    }
}

static void test_secure_context_and_serialized_lifecycle(void)
{
    Fixture fixture = {0};
    Fcj3OperationContext context;
    Fcj3PreparedRecord prepared;
    Fcj3Replay replay;
    char source[PATH_MAX];
    char destination[PATH_MAX];

    fixture_init(&fixture);
    make_child_path(source, fixture.root, "source.dat");
    make_child_path(destination, fixture.root, "destination.dat");

    memset(&context, 0, sizeof(context));
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &fixture.directory_stat, ".fc-tmp-test", &fixture.staging_stat), 0);
    assert(context.initialized == 1);
    assert(context.sequence == 0);
    assert(context.state == FCJ3_STATE_NONE);
    assert(context.poisoned == 0);
    assert_lower_hex_id(context.operation_id);

    fcj3_replay_init(&replay);

    /* Prepare is side-effect-free on the context; commit follows durable append. */
    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STAGED,
                                                   0, 100U, &prepared), 0);
    assert(context.sequence == 0 && context.state == FCJ3_STATE_NONE);
    CHECK_RESULT(fcj3_replay_add_line(&replay, prepared.line, prepared.length), 0);
    durably_append_test_record(fixture.journal_fd, &prepared);
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &prepared), 0);
    assert(context.sequence == 1 && context.state == FCJ3_STATE_STAGED);

    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STARTED,
                                                   0, 101U, &prepared), 0);
    CHECK_RESULT(fcj3_replay_add_line(&replay, prepared.line, prepared.length), 0);
    durably_append_test_record(fixture.journal_fd, &prepared);
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &prepared), 0);

    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_PUBLISHED,
                                                   0, 102U, &prepared), 0);
    CHECK_RESULT(fcj3_replay_add_line(&replay, prepared.line, prepared.length), 0);
    durably_append_test_record(fixture.journal_fd, &prepared);
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &prepared), 0);

    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_COMPLETED,
                                                   0, 103U, &prepared), 0);
    CHECK_RESULT(fcj3_replay_add_line(&replay, prepared.line, prepared.length), 0);
    durably_append_test_record(fixture.journal_fd, &prepared);
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &prepared), 0);

    assert(context.sequence == 4);
    assert(context.state == FCJ3_STATE_COMPLETED);
    assert(replay.count == 1);
    assert(replay.transactions[0].state == FCJ3_STATE_COMPLETED);
    assert(strcmp(replay.transactions[0].operation_id, context.operation_id) == 0);
    assert(strcmp(replay.transactions[0].source, source) == 0);
    assert(strcmp(replay.transactions[0].destination, destination) == 0);
    assert(replay.transactions[0].destination_dir_dev == fixture.directory_stat.st_dev);
    assert(replay.transactions[0].destination_dir_ino == fixture.directory_stat.st_ino);
    assert(replay.transactions[0].staging_dev == fixture.staging_stat.st_dev);
    assert(replay.transactions[0].staging_ino == fixture.staging_stat.st_ino);

    errno = 0;
    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_RECOVERED,
                                                   0, 104U, &prepared), -1);
    assert(errno == EBADMSG);

    fcj3_replay_destroy(&replay);
    fcj3_operation_reset(&context);
    assert(context.initialized == 0);
    fixture_destroy(&fixture);
}

static void test_failed_recovered_and_error_semantics(void)
{
    Fixture fixture = {0};
    Fcj3OperationContext context;
    Fcj3PreparedRecord prepared;
    char source[PATH_MAX];
    char destination[PATH_MAX];

    fixture_init(&fixture);
    make_child_path(source, fixture.root, "source");
    make_child_path(destination, fixture.root, "dest");
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &fixture.directory_stat, ".fc-tmp-test", &fixture.staging_stat), 0);

    errno = 0;
    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STARTED,
                                                   0, 1U, &prepared), -1);
    assert(errno == EBADMSG);
    errno = 0;
    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STAGED,
                                                   EIO, 1U, &prepared), -1);
    assert(errno == EBADMSG);

    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STAGED,
                                                   0, 1U, &prepared), 0);
    durably_append_test_record(fixture.journal_fd, &prepared);
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &prepared), 0);
    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_FAILED,
                                                   EIO, 2U, &prepared), 0);
    char failed_marker[32];
    (void)snprintf(failed_marker, sizeof(failed_marker), "|FAILED|%d|", EIO);
    assert(strstr(prepared.line, failed_marker) != NULL);
    durably_append_test_record(fixture.journal_fd, &prepared);
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &prepared), 0);
    assert(context.state == FCJ3_STATE_FAILED);

    fcj3_operation_reset(&context);
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &fixture.directory_stat, ".fc-tmp-test", &fixture.staging_stat), 0);
    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STAGED,
                                                   0, 1U, &prepared), 0);
    durably_append_test_record(fixture.journal_fd, &prepared);
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &prepared), 0);
    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_RECOVERED,
                                                   EIO, 2U, &prepared), 0);
    durably_append_test_record(fixture.journal_fd, &prepared);
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &prepared), 0);
    assert(context.state == FCJ3_STATE_RECOVERED);

    fcj3_operation_reset(&context);
    fixture_destroy(&fixture);
}

static void test_stale_prepared_record_and_uncertainty(void)
{
    Fixture fixture = {0};
    Fcj3OperationContext context;
    Fcj3PreparedRecord first;
    Fcj3PreparedRecord stale;
    char source[PATH_MAX];
    char destination[PATH_MAX];

    fixture_init(&fixture);
    make_child_path(source, fixture.root, "source");
    make_child_path(destination, fixture.root, "dest");

    /* If a durable append is followed by a tampered commit, poison the context. */
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &fixture.directory_stat, ".fc-tmp-test", &fixture.staging_stat), 0);
    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STAGED,
                                                   0, 1U, &first), 0);
    stale = first;
    stale.line[5] = '9';
    durably_append_test_record(fixture.journal_fd, &first);
    errno = 0;
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &stale), -1);
    assert(errno == ESTALE);
    assert(context.sequence == 0 && context.poisoned == 1);
    errno = 0;
    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STARTED,
                                                   0, 2U, &first), -1);
    assert(errno == EIO);
    fcj3_operation_reset(&context);

    /* A duplicate/stale commit after a successful transition also poisons. */
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &fixture.directory_stat, ".fc-tmp-test", &fixture.staging_stat), 0);
    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STAGED,
                                                   0, 1U, &first), 0);
    stale = first;
    durably_append_test_record(fixture.journal_fd, &first);
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &first), 0);
    errno = 0;
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &stale), -1);
    assert(errno == ESTALE);
    assert(context.poisoned == 1);
    fcj3_operation_reset(&context);

    /* Explicitly poison after a simulated partial-write/fsync uncertainty. */
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &fixture.directory_stat, ".fc-tmp-test", &fixture.staging_stat), 0);
    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STAGED,
                                                   0, 1U, &first), 0);
    fcj3_operation_mark_uncertain(&context);
    errno = 0;
    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STARTED,
                                                   0, 2U, &first), -1);
    assert(errno == EIO);
    errno = 0;
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &stale), -1);
    assert(errno == EIO);

    fcj3_operation_reset(&context);
    fixture_destroy(&fixture);
}

static void test_random_failure_and_invalid_inputs(void)
{
    Fixture fixture = {0};
    Fcj3OperationContext context;
    struct stat regular_stat;
    struct stat directory_stat;
    char source[PATH_MAX];
    char destination[PATH_MAX];

    fixture_init(&fixture);
    make_child_path(source, fixture.root, "source");
    make_child_path(destination, fixture.root, "dest");

    CHECK_RESULT(fstat(fixture.staging_fd, &regular_stat), 0);
    CHECK_RESULT(fstat(fixture.directory_fd, &directory_stat), 0);
    assert(regular_stat.st_nlink == 0);

    /* A named file cannot masquerade as the pre-link O_TMPFILE inode. */
    int named_fd = openat(fixture.directory_fd, "named-stage",
        O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    assert(named_fd >= 0);
    struct stat named_stat;
    CHECK_RESULT(fstat(named_fd, &named_stat), 0);
    assert(named_stat.st_nlink > 0);
    errno = 0;
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &directory_stat, ".fc-tmp-test", &named_stat), -1);
    assert(errno == EINVAL);
    assert(close(named_fd) == 0);
    assert(unlinkat(fixture.directory_fd, "named-stage", 0) == 0);

    struct stat different_device = regular_stat;
    different_device.st_dev = (dev_t)(different_device.st_dev + 1);
    errno = 0;
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &directory_stat, ".fc-tmp-test", &different_device), -1);
    assert(errno == EINVAL);

    struct stat foreign_owner = regular_stat;
    foreign_owner.st_uid = (uid_t)(geteuid() + 1U);
    errno = 0;
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &directory_stat, ".fc-tmp-test", &foreign_owner), -1);
    assert(errno == EINVAL);
    fcj3_test_set_random_error(EIO);
    errno = 0;
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &directory_stat, ".fc-tmp-test", &regular_stat), -1);
    assert(errno == EIO);
    assert(context.initialized == 0);
    assert(context.operation_id[0] == '\0');

    /* EINTR is retried; it must not trigger a weak fallback or abort. */
    fcj3_test_set_random_error(EINTR);
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &directory_stat, ".fc-tmp-test", &regular_stat), 0);
    assert(context.initialized == 1);
    fcj3_operation_reset(&context);

    /* Short reads are accumulated until all 128 random bits are obtained. */
    fcj3_test_set_random_chunk_limit(3U);
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &directory_stat, ".fc-tmp-test", &regular_stat), 0);
    assert_lower_hex_id(context.operation_id);
    fcj3_operation_reset(&context);
    fcj3_test_set_random_chunk_limit(0U);

    errno = 0;
    CHECK_RESULT(fcj3_operation_init(&context, "", destination,
        &directory_stat, ".fc-tmp-test", &regular_stat), -1);
    assert(errno == EINVAL);

    errno = 0;
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &regular_stat, ".fc-tmp-test", &regular_stat), -1);
    assert(errno == EINVAL);

    errno = 0;
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &directory_stat, "not-a-temp", &regular_stat), -1);
    assert(errno == EINVAL);

    errno = 0;
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &directory_stat, ".fc-tmp-", &regular_stat), -1);
    assert(errno == EINVAL);

    /* A directory cannot be accepted as the staging inode. */
    errno = 0;
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &directory_stat, ".fc-tmp-test", &directory_stat), -1);
    assert(errno == EINVAL);

    fixture_destroy(&fixture);
}

int main(void)
{
    test_secure_context_and_serialized_lifecycle();
    test_failed_recovered_and_error_semantics();
    test_stale_prepared_record_and_uncertainty();
    test_random_failure_and_invalid_inputs();

    puts("FCJ3 operation-context and serialization tests passed");
    return 0;
}
