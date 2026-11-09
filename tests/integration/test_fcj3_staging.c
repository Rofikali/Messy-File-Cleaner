#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "journal/fcj3_journal.h"
#include "journal/fcj3_parser.h"
#include "journal/fcj3_stage.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/stat.h>
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
    Fcj3Journal journal;
} Fixture;

static int write_all(int fd, const void *buffer, size_t length)
{
    const unsigned char *bytes = buffer;
    size_t offset = 0U;
    while (offset < length)
    {
        ssize_t count = write(fd, bytes + offset, length - offset);
        if (count < 0 && errno == EINTR)
            continue;
        if (count <= 0)
        {
            if (count == 0)
                errno = EIO;
            return -1;
        }
        offset += (size_t)count;
    }
    return 0;
}

static void fixture_init(Fixture *fixture)
{
    memset(fixture, 0, sizeof(*fixture));
    fixture->directory_fd = -1;
    memcpy(fixture->root, "/tmp/fcj3-stage-test-XXXXXX",
           sizeof("/tmp/fcj3-stage-test-XXXXXX"));
    assert(mkdtemp(fixture->root) != NULL);
    fixture->directory_fd = open(fixture->root, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    assert(fixture->directory_fd >= 0);
    CHECK(fstat(fixture->directory_fd, &fixture->directory_stat));
    assert(S_ISDIR(fixture->directory_stat.st_mode));
    CHECK(fcj3_journal_open_at(&fixture->journal, fixture->directory_fd,
                               "test.fcj3", geteuid()));
}

static void fixture_destroy(Fixture *fixture)
{
    CHECK(fcj3_journal_close(&fixture->journal));
    assert(unlinkat(fixture->directory_fd, "test.fcj3", 0) == 0);
    assert(close(fixture->directory_fd) == 0);
    assert(rmdir(fixture->root) == 0);
}

static int random_component(char output[NAME_MAX])
{
    static const char hex[] = "0123456789abcdef";
    unsigned char bytes[16];
    size_t offset = 0U;
    while (offset < sizeof(bytes))
    {
        ssize_t count = getrandom(bytes + offset, sizeof(bytes) - offset, 0);
        if (count < 0 && errno == EINTR)
            continue;
        if (count <= 0)
        {
            if (count == 0)
                errno = EIO;
            return -1;
        }
        offset += (size_t)count;
    }

    memcpy(output, ".fc-tmp-", 8U);
    for (size_t i = 0U; i < sizeof(bytes); ++i)
    {
        output[8U + i * 2U] = hex[bytes[i] >> 4U];
        output[9U + i * 2U] = hex[bytes[i] & 0x0fU];
    }
    output[40U] = '\0';
    return 0;
}

static int create_source(Fixture *fixture, const char *name,
                         const char *payload, size_t payload_length)
{
    int fd = openat(fixture->directory_fd, name,
                    O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0)
        return -1;
    int result = write_all(fd, payload, payload_length);
    if (result == 0)
        result = fsync(fd);
    int saved_errno = errno;
    if (close(fd) != 0 && result == 0)
    {
        result = -1;
        saved_errno = errno;
    }
    errno = saved_errno;
    return result;
}

static void append_state(Fixture *fixture, Fcj3OperationContext *context,
                         Fcj3State state, int error_code, uint64_t timestamp)
{
    Fcj3PreparedRecord prepared;
    CHECK(fcj3_operation_prepare_transition(context, state, error_code,
                                             timestamp, &prepared));
    CHECK(fcj3_journal_append_transition(&fixture->journal, context, &prepared));
}

static size_t read_file_at(int directory_fd, const char *name,
                           char *buffer, size_t capacity)
{
    int fd = openat(directory_fd, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
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

static void assert_replay_state(Fixture *fixture, Fcj3State expected_state,
                                uint64_t expected_sequence)
{
    int fd = openat(fixture->directory_fd, "test.fcj3", O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    assert(fd >= 0);
    Fcj3Replay replay;
    fcj3_replay_init(&replay);
    char buffer[FCJ3_MAX_LINE_BYTES + 1U];
    size_t used = 0U;
    for (;;)
    {
        ssize_t count = read(fd, buffer + used, sizeof(buffer) - used);
        if (count < 0 && errno == EINTR)
            continue;
        assert(count >= 0);
        if (count == 0)
            break;
        used += (size_t)count;
        assert(used < sizeof(buffer));
    }
    assert(close(fd) == 0);

    size_t offset = 0U;
    while (offset < used)
    {
        char *newline = memchr(buffer + offset, '\n', used - offset);
        assert(newline != NULL);
        size_t line_length = (size_t)(newline - (buffer + offset)) + 1U;
        CHECK(fcj3_replay_add_line(&replay, buffer + offset, line_length));
        offset += line_length;
    }
    assert(replay.count == 1U);
    assert(replay.transactions[0].state == expected_state);
    assert(replay.transactions[0].sequence == expected_sequence);
    fcj3_replay_destroy(&replay);
}

static void test_publish_preserves_source_and_exact_inode(void)
{
    static const char payload[] = "FCJ3 anonymous staging payload";
    Fixture fixture;
    Fcj3Stage stage;
    Fcj3OperationContext context;
    struct stat staging_metadata;
    char component[NAME_MAX];
    char source_path[PATH_MAX];
    char destination_path[PATH_MAX];
    fixture_init(&fixture);
    CHECK(create_source(&fixture, "source.bin", payload, sizeof(payload) - 1U));
    CHECK(random_component(component));
    CHECK(fcj3_stage_open_at(&stage, fixture.directory_fd, component, geteuid()));

    int source_fd = openat(fixture.directory_fd, "source.bin",
                           O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    assert(source_fd >= 0);
    struct stat source_snapshot;
    uint64_t bytes_copied = 0U;
    CHECK(fcj3_stage_copy_from(&stage, source_fd, &source_snapshot, &bytes_copied));
    assert(bytes_copied == sizeof(payload) - 1U);
    assert(source_snapshot.st_size == (off_t)(sizeof(payload) - 1U));
    assert(close(source_fd) == 0);
    CHECK(fcj3_stage_sync(&stage, &staging_metadata));
    int source_length = snprintf(source_path, sizeof(source_path), "%s/source.bin",
                                 fixture.root);
    int destination_length = snprintf(destination_path, sizeof(destination_path),
                                      "%s/destination.bin", fixture.root);
    assert(source_length > 0 && (size_t)source_length < sizeof(source_path));
    assert(destination_length > 0 && (size_t)destination_length < sizeof(destination_path));
    CHECK(fcj3_operation_init(&context, source_path, destination_path,
        &fixture.directory_stat, component, &staging_metadata));

    append_state(&fixture, &context, FCJ3_STATE_STAGED, 0, 1000U);
    CHECK(fcj3_stage_link(&stage));
    append_state(&fixture, &context, FCJ3_STATE_STARTED, 0, 1001U);
    CHECK(fcj3_stage_publish(&stage, "destination.bin"));
    append_state(&fixture, &context, FCJ3_STATE_PUBLISHED, 0, 1002U);

    struct stat published;
    CHECK(fstatat(fixture.directory_fd, "destination.bin", &published,
                  AT_SYMLINK_NOFOLLOW));
    assert(published.st_dev == staging_metadata.st_dev);
    assert(published.st_ino == staging_metadata.st_ino);
    assert(faccessat(fixture.directory_fd, component, F_OK, AT_SYMLINK_NOFOLLOW) == -1);
    assert(errno == ENOENT);

    char actual[sizeof(payload)];
    size_t actual_length = read_file_at(fixture.directory_fd, "destination.bin",
                                        actual, sizeof(actual));
    assert(actual_length == sizeof(payload) - 1U);
    assert(memcmp(actual, payload, actual_length) == 0);
    actual_length = read_file_at(fixture.directory_fd, "source.bin",
                                 actual, sizeof(actual));
    assert(actual_length == sizeof(payload) - 1U);
    assert(memcmp(actual, payload, actual_length) == 0);

    assert_replay_state(&fixture, FCJ3_STATE_PUBLISHED, 3U);
    CHECK(fcj3_stage_close(&stage));
    fcj3_operation_reset(&context);
    assert(unlinkat(fixture.directory_fd, "source.bin", 0) == 0);
    assert(unlinkat(fixture.directory_fd, "destination.bin", 0) == 0);
    fixture_destroy(&fixture);
}

static void test_collision_never_overwrites_and_fails_operation(void)
{
    static const char original[] = "pre-existing destination";
    static const char staged[] = "new staged payload";
    Fixture fixture;
    Fcj3Stage stage;
    Fcj3OperationContext context;
    struct stat staging_metadata;
    char component[NAME_MAX];
    char source_path[PATH_MAX];
    char destination_path[PATH_MAX];
    fixture_init(&fixture);
    CHECK(create_source(&fixture, "source.bin", staged, sizeof(staged) - 1U));
    CHECK(create_source(&fixture, "destination.bin", original, sizeof(original) - 1U));
    CHECK(random_component(component));
    CHECK(fcj3_stage_open_at(&stage, fixture.directory_fd, component, geteuid()));
    int source_fd = openat(fixture.directory_fd, "source.bin",
                           O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    assert(source_fd >= 0);
    struct stat source_snapshot;
    uint64_t bytes_copied = 0U;
    CHECK(fcj3_stage_copy_from(&stage, source_fd, &source_snapshot, &bytes_copied));
    assert(bytes_copied == sizeof(staged) - 1U);
    assert(close(source_fd) == 0);
    CHECK(fcj3_stage_sync(&stage, &staging_metadata));

    int source_length = snprintf(source_path, sizeof(source_path), "%s/source.bin",
                                 fixture.root);
    int destination_length = snprintf(destination_path, sizeof(destination_path),
                                      "%s/destination.bin", fixture.root);
    assert(source_length > 0 && (size_t)source_length < sizeof(source_path));
    assert(destination_length > 0 && (size_t)destination_length < sizeof(destination_path));
    CHECK(fcj3_operation_init(&context, source_path, destination_path,
        &fixture.directory_stat, component, &staging_metadata));

    append_state(&fixture, &context, FCJ3_STATE_STAGED, 0, 2000U);
    CHECK(fcj3_stage_link(&stage));
    append_state(&fixture, &context, FCJ3_STATE_STARTED, 0, 2001U);

    errno = 0;
    assert(fcj3_stage_publish(&stage, "destination.bin") == -1);
    assert(errno == EEXIST);
    append_state(&fixture, &context, FCJ3_STATE_FAILED, EEXIST, 2002U);

    char actual[sizeof(original)];
    size_t actual_length = read_file_at(fixture.directory_fd, "destination.bin",
                                        actual, sizeof(actual));
    assert(actual_length == sizeof(original) - 1U);
    assert(memcmp(actual, original, actual_length) == 0);

    struct stat stage_entry;
    CHECK(fstatat(fixture.directory_fd, component, &stage_entry, AT_SYMLINK_NOFOLLOW));
    assert(stage_entry.st_dev == staging_metadata.st_dev);
    assert(stage_entry.st_ino == staging_metadata.st_ino);

    assert_replay_state(&fixture, FCJ3_STATE_FAILED, 3U);
    CHECK(fcj3_stage_close(&stage));
    fcj3_operation_reset(&context);
    assert(unlinkat(fixture.directory_fd, component, 0) == 0);
    assert(unlinkat(fixture.directory_fd, "source.bin", 0) == 0);
    assert(unlinkat(fixture.directory_fd, "destination.bin", 0) == 0);
    fixture_destroy(&fixture);
}


static void test_partial_copy_failure_poisoned_and_source_preserved(void)
{
    static const char payload[] = "source data must remain unchanged after partial copy";
    Fixture fixture;
    Fcj3Stage stage;
    char component[NAME_MAX];
    fixture_init(&fixture);
    CHECK(create_source(&fixture, "source.bin", payload, sizeof(payload) - 1U));
    int source_fd = openat(fixture.directory_fd, "source.bin",
                           O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    assert(source_fd >= 0);
    CHECK(random_component(component));
    CHECK(fcj3_stage_open_at(&stage, fixture.directory_fd, component, geteuid()));

    struct stat snapshot;
    uint64_t bytes_copied = 0U;
    fcj3_stage_test_fail_write_after(7U, EIO);
    errno = 0;
    assert(fcj3_stage_copy_from(&stage, source_fd, &snapshot, &bytes_copied) == -1);
    assert(errno == EIO);
    assert(fcj3_stage_fd(&stage) == -1);
    struct stat staging_stat;
    errno = 0;
    assert(fcj3_stage_sync(&stage, &staging_stat) == -1);
    assert(errno == EINVAL);

    char actual[sizeof(payload)];
    size_t actual_length = read_file_at(fixture.directory_fd, "source.bin",
                                        actual, sizeof(actual));
    assert(actual_length == sizeof(payload) - 1U);
    assert(memcmp(actual, payload, actual_length) == 0);
    struct stat named;
    assert(fstatat(fixture.directory_fd, component, &named, AT_SYMLINK_NOFOLLOW) == -1);
    assert(errno == ENOENT);

    assert(close(source_fd) == 0);
    CHECK(fcj3_stage_close(&stage));
    assert(unlinkat(fixture.directory_fd, "source.bin", 0) == 0);
    fixture_destroy(&fixture);
}


static void test_source_change_during_copy_is_rejected(void)
{
    static const char payload[] = "source snapshot must remain stable";
    Fixture fixture;
    Fcj3Stage stage;
    char component[NAME_MAX];
    fixture_init(&fixture);
    CHECK(create_source(&fixture, "source.bin", payload, sizeof(payload) - 1U));
    int source_fd = openat(fixture.directory_fd, "source.bin",
                           O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    int mutation_fd = openat(fixture.directory_fd, "source.bin",
                             O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    assert(source_fd >= 0 && mutation_fd >= 0);
    CHECK(random_component(component));
    CHECK(fcj3_stage_open_at(&stage, fixture.directory_fd, component, geteuid()));

    struct stat snapshot;
    uint64_t bytes_copied = 0U;
    fcj3_stage_test_grow_source_after_copy(mutation_fd);
    errno = 0;
    assert(fcj3_stage_copy_from(&stage, source_fd, &snapshot, &bytes_copied) == -1);
    assert(errno == ESTALE);
    assert(fcj3_stage_fd(&stage) == -1);

    struct stat stage_stat;
    errno = 0;
    assert(fcj3_stage_sync(&stage, &stage_stat) == -1);
    assert(errno == EINVAL);
    struct stat named;
    assert(fstatat(fixture.directory_fd, component, &named, AT_SYMLINK_NOFOLLOW) == -1);
    assert(errno == ENOENT);

    assert(close(source_fd) == 0);
    assert(close(mutation_fd) == 0);
    CHECK(fcj3_stage_close(&stage));
    assert(unlinkat(fixture.directory_fd, "source.bin", 0) == 0);
    fixture_destroy(&fixture);
}


static void test_stage_fsync_failure_poisoned(void)
{
    static const char payload[] = "file fsync failure must block publication";
    Fixture fixture;
    Fcj3Stage stage;
    char component[NAME_MAX];
    fixture_init(&fixture);
    CHECK(create_source(&fixture, "source.bin", payload, sizeof(payload) - 1U));
    int source_fd = openat(fixture.directory_fd, "source.bin",
                           O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    assert(source_fd >= 0);
    CHECK(random_component(component));
    CHECK(fcj3_stage_open_at(&stage, fixture.directory_fd, component, geteuid()));
    struct stat source_snapshot;
    struct stat staging_snapshot;
    uint64_t bytes_copied = 0U;
    CHECK(fcj3_stage_copy_from(&stage, source_fd, &source_snapshot, &bytes_copied));

    fcj3_stage_test_fail_fsync(EIO);
    errno = 0;
    assert(fcj3_stage_sync(&stage, &staging_snapshot) == -1);
    assert(errno == EIO);
    assert(fcj3_stage_fd(&stage) == -1);
    assert(fcj3_stage_link(&stage) == -1);
    struct stat named;
    assert(fstatat(fixture.directory_fd, component, &named, AT_SYMLINK_NOFOLLOW) == -1);
    assert(errno == ENOENT);

    char actual[sizeof(payload)];
    size_t actual_length = read_file_at(fixture.directory_fd, "source.bin",
                                        actual, sizeof(actual));
    assert(actual_length == sizeof(payload) - 1U);
    assert(memcmp(actual, payload, actual_length) == 0);
    assert(close(source_fd) == 0);
    CHECK(fcj3_stage_close(&stage));
    assert(unlinkat(fixture.directory_fd, "source.bin", 0) == 0);
    fixture_destroy(&fixture);
}

static void test_link_directory_fsync_failure_preserves_stage_name(void)
{
    static const char payload[] = "linked staging entry remains for recovery";
    Fixture fixture;
    Fcj3Stage stage;
    char component[NAME_MAX];
    fixture_init(&fixture);
    CHECK(create_source(&fixture, "source.bin", payload, sizeof(payload) - 1U));
    int source_fd = openat(fixture.directory_fd, "source.bin",
                           O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    assert(source_fd >= 0);
    CHECK(random_component(component));
    CHECK(fcj3_stage_open_at(&stage, fixture.directory_fd, component, geteuid()));
    struct stat source_snapshot;
    struct stat staging_snapshot;
    uint64_t bytes_copied = 0U;
    CHECK(fcj3_stage_copy_from(&stage, source_fd, &source_snapshot, &bytes_copied));
    CHECK(fcj3_stage_sync(&stage, &staging_snapshot));

    fcj3_stage_test_fail_fsync(EIO);
    errno = 0;
    assert(fcj3_stage_link(&stage) == -1);
    assert(errno == EIO);
    assert(fcj3_stage_publish(&stage, "destination.bin") == -1);
    struct stat visible;
    CHECK(fstatat(fixture.directory_fd, component, &visible, AT_SYMLINK_NOFOLLOW));
    assert(visible.st_dev == staging_snapshot.st_dev);
    assert(visible.st_ino == staging_snapshot.st_ino);

    assert(close(source_fd) == 0);
    CHECK(fcj3_stage_close(&stage));
    CHECK(fstatat(fixture.directory_fd, component, &visible, AT_SYMLINK_NOFOLLOW));
    assert(visible.st_dev == staging_snapshot.st_dev);
    assert(visible.st_ino == staging_snapshot.st_ino);
    assert(unlinkat(fixture.directory_fd, component, 0) == 0);
    assert(unlinkat(fixture.directory_fd, "source.bin", 0) == 0);
    fixture_destroy(&fixture);
}

static void test_publish_directory_fsync_failure_preserves_destination(void)
{
    static const char payload[] = "published destination survives fsync uncertainty";
    Fixture fixture;
    Fcj3Stage stage;
    char component[NAME_MAX];
    fixture_init(&fixture);
    CHECK(create_source(&fixture, "source.bin", payload, sizeof(payload) - 1U));
    int source_fd = openat(fixture.directory_fd, "source.bin",
                           O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    assert(source_fd >= 0);
    CHECK(random_component(component));
    CHECK(fcj3_stage_open_at(&stage, fixture.directory_fd, component, geteuid()));
    struct stat source_snapshot;
    struct stat staging_snapshot;
    uint64_t bytes_copied = 0U;
    CHECK(fcj3_stage_copy_from(&stage, source_fd, &source_snapshot, &bytes_copied));
    CHECK(fcj3_stage_sync(&stage, &staging_snapshot));
    CHECK(fcj3_stage_link(&stage));

    fcj3_stage_test_fail_fsync(EIO);
    errno = 0;
    assert(fcj3_stage_publish(&stage, "destination.bin") == -1);
    assert(errno == EIO);
    struct stat visible;
    CHECK(fstatat(fixture.directory_fd, "destination.bin", &visible, AT_SYMLINK_NOFOLLOW));
    assert(visible.st_dev == staging_snapshot.st_dev);
    assert(visible.st_ino == staging_snapshot.st_ino);
    assert(fstatat(fixture.directory_fd, component, &visible, AT_SYMLINK_NOFOLLOW) == -1);
    assert(errno == ENOENT);

    assert(close(source_fd) == 0);
    CHECK(fcj3_stage_close(&stage));
    CHECK(fstatat(fixture.directory_fd, "destination.bin", &visible, AT_SYMLINK_NOFOLLOW));
    assert(visible.st_dev == staging_snapshot.st_dev);
    assert(visible.st_ino == staging_snapshot.st_ino);
    assert(unlinkat(fixture.directory_fd, "destination.bin", 0) == 0);
    assert(unlinkat(fixture.directory_fd, "source.bin", 0) == 0);
    fixture_destroy(&fixture);
}

int main(void)
{
    test_publish_preserves_source_and_exact_inode();
    test_collision_never_overwrites_and_fails_operation();
    test_partial_copy_failure_poisoned_and_source_preserved();
    test_source_change_during_copy_is_rejected();
    test_stage_fsync_failure_poisoned();
    test_link_directory_fsync_failure_preserves_stage_name();
    test_publish_directory_fsync_failure_preserves_destination();
    puts("FCJ3 isolated staging lifecycle tests passed");
    return 0;
}
