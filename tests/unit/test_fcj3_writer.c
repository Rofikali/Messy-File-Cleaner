#define _POSIX_C_SOURCE 200809L

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
    char stage_path[PATH_MAX];
    int directory_fd;
    int staging_fd;
    struct stat directory_stat;
    struct stat staging_stat;
} Fixture;

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

    int length = snprintf(fixture->stage_path, sizeof(fixture->stage_path),
                          "%s/.fc-tmp-test", fixture->root);
    assert(length > 0 && (size_t)length < sizeof(fixture->stage_path));
    fixture->staging_fd = open(fixture->stage_path,
        O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    assert(fixture->staging_fd >= 0);
    assert(fstat(fixture->staging_fd, &fixture->staging_stat) == 0);
}

static void fixture_destroy(Fixture *fixture)
{
    if (fixture->staging_fd >= 0)
        assert(close(fixture->staging_fd) == 0);
    if (fixture->directory_fd >= 0)
        assert(close(fixture->directory_fd) == 0);
    assert(unlink(fixture->stage_path) == 0);
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
    (void)snprintf(source, sizeof(source), "%s/source.dat", fixture.root);
    (void)snprintf(destination, sizeof(destination), "%s/destination.dat",
                   fixture.root);

    memset(&context, 0, sizeof(context));
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &fixture.directory_stat, ".fc-tmp-test", &fixture.staging_stat), 0);
    assert(context.initialized == 1);
    assert(context.sequence == 0);
    assert(context.state == 0);
    assert(context.poisoned == 0);
    assert_lower_hex_id(context.operation_id);

    fcj3_replay_init(&replay);

    /* Prepare is side-effect-free on the context; commit follows durable append. */
    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STAGED,
                                                   0, 100U, &prepared), 0);
    assert(context.sequence == 0 && context.state == 0);
    CHECK_RESULT(fcj3_replay_add_line(&replay, prepared.line, prepared.length), 0);
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &prepared), 0);
    assert(context.sequence == 1 && context.state == FCJ3_STATE_STAGED);

    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STARTED,
                                                   0, 101U, &prepared), 0);
    CHECK_RESULT(fcj3_replay_add_line(&replay, prepared.line, prepared.length), 0);
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &prepared), 0);

    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_PUBLISHED,
                                                   0, 102U, &prepared), 0);
    CHECK_RESULT(fcj3_replay_add_line(&replay, prepared.line, prepared.length), 0);
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &prepared), 0);

    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_COMPLETED,
                                                   0, 103U, &prepared), 0);
    CHECK_RESULT(fcj3_replay_add_line(&replay, prepared.line, prepared.length), 0);
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
    (void)snprintf(source, sizeof(source), "%s/source", fixture.root);
    (void)snprintf(destination, sizeof(destination), "%s/dest", fixture.root);
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
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &prepared), 0);
    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_FAILED,
                                                   EIO, 2U, &prepared), 0);
    assert(strstr(prepared.line, "|FAILED|5|") != NULL);
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &prepared), 0);
    assert(context.state == FCJ3_STATE_FAILED);

    fcj3_operation_reset(&context);
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &fixture.directory_stat, ".fc-tmp-test", &fixture.staging_stat), 0);
    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STAGED,
                                                   0, 1U, &prepared), 0);
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &prepared), 0);
    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_RECOVERED,
                                                   EIO, 2U, &prepared), 0);
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
    (void)snprintf(source, sizeof(source), "%s/source", fixture.root);
    (void)snprintf(destination, sizeof(destination), "%s/dest", fixture.root);
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &fixture.directory_stat, ".fc-tmp-test", &fixture.staging_stat), 0);

    CHECK_RESULT(fcj3_operation_prepare_transition(&context, FCJ3_STATE_STAGED,
                                                   0, 1U, &first), 0);
    stale = first;
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &first), 0);
    errno = 0;
    CHECK_RESULT(fcj3_operation_commit_transition(&context, &stale), -1);
    assert(errno == ESTALE);

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
    char other_path[PATH_MAX];

    fixture_init(&fixture);
    (void)snprintf(source, sizeof(source), "%s/source", fixture.root);
    (void)snprintf(destination, sizeof(destination), "%s/dest", fixture.root);
    (void)snprintf(other_path, sizeof(other_path), "%s/other", fixture.root);

    CHECK_RESULT(fstat(fixture.staging_fd, &regular_stat), 0);
    CHECK_RESULT(fstat(fixture.directory_fd, &directory_stat), 0);

    fcj3_test_set_random_error(EIO);
    errno = 0;
    CHECK_RESULT(fcj3_operation_init(&context, source, destination,
        &directory_stat, ".fc-tmp-test", &regular_stat), -1);
    assert(errno == EIO);
    assert(context.initialized == 0);
    assert(context.operation_id[0] == '\0');

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

    (void)other_path;
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
