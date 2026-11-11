#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "journal/fcj4_parser.h"
#include "journal/fcj4_writer.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s (errno=%d)\n", \
                __FILE__, __LINE__, #expr, errno); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

#define FIXTURE_CAPACITY (FCJ4_MAX_LINE_BYTES * 2U + 8U)

static const char operation_id[] =
    "11111111111111111111111111111111";
static const char digest[] =
    "0000000000000000000000000000000000000000000000000000000000000000";

typedef struct {
    char directory[128];
    char path[160];
} Fixture;

static void fixture_create(Fixture *fixture, const char *bytes, size_t length)
{
    char template_path[] = "/tmp/fcj4-restart-XXXXXX";
    int fd;

    CHECK(fixture != NULL);
    CHECK(length <= FIXTURE_CAPACITY);
    CHECK(mkdtemp(template_path) != NULL);
    CHECK(strlen(template_path) < sizeof(fixture->directory));
    memcpy(fixture->directory, template_path, strlen(template_path) + 1U);
    CHECK(snprintf(fixture->path, sizeof(fixture->path), "%s/journal",
                   fixture->directory) > 0);

    fd = open(fixture->path,
              O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
              S_IRUSR | S_IWUSR);
    CHECK(fd >= 0);
    {
        size_t offset = 0U;
        while (offset < length) {
            ssize_t written = write(fd, bytes + offset, length - offset);
            CHECK(written > 0);
            offset += (size_t)written;
        }
    }
    CHECK(close(fd) == 0);
}

static size_t fixture_read(const Fixture *fixture, char *buffer, size_t capacity)
{
    struct stat st;
    size_t offset = 0U;
    int fd = open(fixture->path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);

    CHECK(fd >= 0);
    CHECK(fstat(fd, &st) == 0);
    CHECK(S_ISREG(st.st_mode));
    CHECK(st.st_size >= 0 && (uint64_t)st.st_size <= (uint64_t)capacity);
    while (offset < (size_t)st.st_size) {
        ssize_t amount = read(fd, buffer + offset, (size_t)st.st_size - offset);
        CHECK(amount > 0);
        offset += (size_t)amount;
    }
    CHECK(close(fd) == 0);
    return offset;
}

static void fixture_destroy(const Fixture *fixture)
{
    CHECK(unlink(fixture->path) == 0);
    CHECK(rmdir(fixture->directory) == 0);
}

/*
 * Each replay attempt is made from a fresh read-only descriptor. The helper
 * snapshots exact bytes, checks all-or-nothing output, repeats replay, and
 * proves neither parse attempt changed the disposable journal.
 */
static void verify_fixture(const char *bytes, size_t length,
                           bool expect_success, size_t expected_records)
{
    Fixture fixture;
    char before[FIXTURE_CAPACITY];
    char after[FIXTURE_CAPACITY];
    char input[FIXTURE_CAPACITY];
    size_t before_length, after_length, input_length;
    Fcj4Replay first, second;
    int first_rc, second_rc;

    fixture_create(&fixture, bytes, length);
    before_length = fixture_read(&fixture, before, sizeof(before));
    CHECK(before_length == length);
    CHECK(memcmp(before, bytes, length) == 0);

    input_length = fixture_read(&fixture, input, sizeof(input));
    CHECK(input_length == before_length);
    fcj4_replay_init(&first);
    errno = 0;
    first_rc = fcj4_replay_parse(input, input_length, &first);
    if (expect_success) {
        CHECK(first_rc == 0);
        CHECK(first.record_count == expected_records);
        CHECK(first.count == 1U);
    } else {
        CHECK(first_rc == -1);
        CHECK(first.operations == NULL);
        CHECK(first.count == 0U);
        CHECK(first.capacity == 0U);
        CHECK(first.record_count == 0U);
    }
    fcj4_replay_destroy(&first);

    after_length = fixture_read(&fixture, after, sizeof(after));
    CHECK(after_length == before_length);
    CHECK(memcmp(after, before, before_length) == 0);

    input_length = fixture_read(&fixture, input, sizeof(input));
    fcj4_replay_init(&second);
    errno = 0;
    second_rc = fcj4_replay_parse(input, input_length, &second);
    CHECK((second_rc == 0) == expect_success);
    if (expect_success) {
        CHECK(second.record_count == expected_records);
        CHECK(second.count == 1U);
    } else {
        CHECK(second.operations == NULL);
        CHECK(second.count == 0U);
        CHECK(second.record_count == 0U);
    }
    fcj4_replay_destroy(&second);

    after_length = fixture_read(&fixture, after, sizeof(after));
    CHECK(after_length == before_length);
    CHECK(memcmp(after, before, before_length) == 0);
    fixture_destroy(&fixture);
}

static size_t serialize_begin(char *out, size_t capacity)
{
    Fcj4Record record;
    size_t written = 0U;

    memset(&record, 0, sizeof(record));
    record.kind = FCJ4_RECORD_BEGIN;
    record.value.begin = (Fcj4BeginRecord){
        operation_id, "/tmp/source",
        1U, 2U, 1U, 3U, 1000U, 1000U, 384U, 5U,
        1, 2U, 3, 4U,
        "/tmp/dest", 1U, 4U, "dest", 1U, 5U
    };
    CHECK(fcj4_serialize(&record, out, capacity, &written) == 0);
    CHECK(written > 1U && out[written - 1U] == '\n');
    return written;
}

static size_t serialize_sealed(char *out, size_t capacity)
{
    Fcj4Record record;
    size_t written = 0U;

    memset(&record, 0, sizeof(record));
    record.kind = FCJ4_RECORD_SEALED;
    record.value.sealed = (Fcj4SealedRecord){
        operation_id, ".fc-tmp-x", 1U, 6U, 1000U, 384U, 5U, digest
    };
    CHECK(fcj4_serialize(&record, out, capacity, &written) == 0);
    CHECK(written > 1U && out[written - 1U] == '\n');
    return written;
}

static size_t make_two_record_journal(char *out, size_t capacity,
                                      size_t *begin_length)
{
    size_t first = serialize_begin(out, capacity);
    size_t second;

    CHECK(first < capacity);
    second = serialize_sealed(out + first, capacity - first);
    CHECK(first + second <= capacity);
    if (begin_length != NULL) *begin_length = first;
    return first + second;
}

static void test_every_strict_prefix(void)
{
    char journal[FIXTURE_CAPACITY];
    size_t begin_length = 0U;
    size_t total = make_two_record_journal(journal, sizeof(journal),
                                           &begin_length);

    CHECK(begin_length > 1U && total > begin_length);
    for (size_t prefix = 0U; prefix < begin_length; prefix++)
        verify_fixture(journal, prefix, false, 0U);
    for (size_t prefix = begin_length; prefix < total; prefix++)
        verify_fixture(journal, prefix, false, 0U);
    verify_fixture(journal, total, true, 2U);
}

static void test_physical_framing_corruption(void)
{
    char begin[FCJ4_MAX_LINE_BYTES];
    char changed[FIXTURE_CAPACITY];
    size_t length = serialize_begin(begin, sizeof(begin));
    size_t middle = length / 2U;

    /* Missing terminal LF. */
    verify_fixture(begin, length - 1U, false, 0U);

    /* CRLF is not normalized to LF. */
    memcpy(changed, begin, length - 1U);
    changed[length - 1U] = '\r';
    changed[length] = '\n';
    verify_fixture(changed, length + 1U, false, 0U);

    /* Embedded NUL is rejected at distinct positions in the physical line. */
    for (size_t i = 0U; i < 3U; i++) {
        size_t at = i == 0U ? 0U : (i == 1U ? middle : length - 2U);
        memcpy(changed, begin, length);
        changed[at] = '\0';
        verify_fixture(changed, length, false, 0U);
    }

    /* A second LF creates an empty physical line. */
    memcpy(changed, begin, length);
    changed[length] = '\n';
    verify_fixture(changed, length + 1U, false, 0U);

    /* Control and non-ASCII bytes in an otherwise complete record. */
    memcpy(changed, begin, length);
    changed[middle] = '\001';
    verify_fixture(changed, length, false, 0U);
    memcpy(changed, begin, length);
    changed[middle] = (char)0xC3;
    verify_fixture(changed, length, false, 0U);

    /* Missing and extra delimiters. */
    memcpy(changed, begin, length);
    changed[4] = '!';
    verify_fixture(changed, length, false, 0U);
    memcpy(changed, begin, length);
    changed[13] = '|';
    verify_fixture(changed, length, false, 0U);

    /* Empty required operation identifier. */
    memcpy(changed, begin, length);
    changed[13] = '|';
    changed[14] = '1';
    verify_fixture(changed, length, false, 0U);

    /* Non-canonical integer: decimal 11 becomes 01. */
    memcpy(changed, begin, length);
    {
        char *field = strstr(changed, "|11|");
        CHECK(field != NULL);
        field[1] = '0';
    }
    verify_fixture(changed, length, false, 0U);

    /* Uppercase hex and malformed identifier. */
    memcpy(changed, begin, length);
    {
        char *path_hex = strstr(changed, "|2f746d702f736f75726365|");
        CHECK(path_hex != NULL);
        path_hex[1] = 'F';
    }
    verify_fixture(changed, length, false, 0U);
    memcpy(changed, begin, length);
    changed[14] = 'g';
    verify_fixture(changed, length, false, 0U);

    /* Exact physical-line limit is parsed (and rejected as malformed);
       one byte beyond the limit is rejected by the configured size bound. */
    memset(changed, 'A', FCJ4_MAX_LINE_BYTES + 1U);
    changed[FCJ4_MAX_LINE_BYTES - 1U] = '\n';
    verify_fixture(changed, FCJ4_MAX_LINE_BYTES, false, 0U);
    changed[FCJ4_MAX_LINE_BYTES - 1U] = 'A';
    changed[FCJ4_MAX_LINE_BYTES] = '\n';
    verify_fixture(changed, FCJ4_MAX_LINE_BYTES + 1U, false, 0U);
    {
        Fcj4Replay replay;
        fcj4_replay_init(&replay);
        errno = 0;
        CHECK(fcj4_replay_parse(changed, FCJ4_MAX_LINE_BYTES + 1U,
                                &replay) == -1);
        CHECK(errno == E2BIG);
        CHECK(replay.operations == NULL);
        fcj4_replay_destroy(&replay);
    }
}

static void test_sealed_digest_corruption(void)
{
    char journal[FIXTURE_CAPACITY];
    char changed[FIXTURE_CAPACITY];
    size_t begin_length = 0U;
    size_t total = make_two_record_journal(journal, sizeof(journal),
                                           &begin_length);
    char *digest_field;

    memcpy(changed, journal, total);
    digest_field = strstr(changed + begin_length,
        "0000000000000000000000000000000000000000000000000000000000000000");
    CHECK(digest_field != NULL);
    digest_field[0] = 'G';
    verify_fixture(changed, total, false, 0U);

    memcpy(changed, journal, total);
    digest_field = strstr(changed + begin_length,
        "0000000000000000000000000000000000000000000000000000000000000000");
    CHECK(digest_field != NULL);
    digest_field[63] = 'A';
    verify_fixture(changed, total, false, 0U);
}

static void test_child_exit_with_partial_record(void)
{
    char begin[FCJ4_MAX_LINE_BYTES];
    size_t length = serialize_begin(begin, sizeof(begin));
    Fixture fixture;
    pid_t child;
    int status = 0;
    char bytes[FIXTURE_CAPACITY];
    size_t read_length;

    fixture_create(&fixture, NULL, 0U);
    child = fork();
    CHECK(child >= 0);
    if (child == 0) {
        int fd = open(fixture.path, O_WRONLY | O_CLOEXEC | O_NOFOLLOW);
        size_t prefix = length / 2U;
        size_t offset = 0U;
        if (fd < 0) _exit(20);
        while (offset < prefix) {
            ssize_t amount = write(fd, begin + offset, prefix - offset);
            if (amount <= 0) _exit(21);
            offset += (size_t)amount;
        }
        if (close(fd) != 0) _exit(22);
        /* Process exit is not a hardware power-loss simulation. */
        _exit(0);
    }

    CHECK(waitpid(child, &status, 0) == child);
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    read_length = fixture_read(&fixture, bytes, sizeof(bytes));
    CHECK(read_length == length / 2U);
    verify_fixture(bytes, read_length, false, 0U);
    fixture_destroy(&fixture);
}

static void test_semantic_corruption(void)
{
    char begin[FCJ4_MAX_LINE_BYTES];
    char sealed[FCJ4_MAX_LINE_BYTES];
    char move[256];
    char journal[FIXTURE_CAPACITY];
    char changed[FIXTURE_CAPACITY];
    size_t begin_length = serialize_begin(begin, sizeof(begin));
    size_t sealed_length = serialize_sealed(sealed, sizeof(sealed));
    size_t used;

    /* Duplicate BEGIN for the same operation identifier. */
    memcpy(changed, begin, begin_length);
    memcpy(changed + begin_length, begin, begin_length);
    verify_fixture(changed, begin_length * 2U, false, 0U);

    /* A syntactically valid MOVE cannot precede its BEGIN. */
    {
        Fcj4Record record;
        size_t move_length = 0U;
        memset(&record, 0, sizeof(record));
        record.kind = FCJ4_RECORD_MOVE;
        record.value.move = (Fcj4MoveRecord){
            operation_id, 1U, FCJ4_MOVE_STAGED, 0U
        };
        CHECK(fcj4_serialize(&record, move, sizeof(move), &move_length) == 0);
        verify_fixture(move, move_length, false, 0U);
    }

    /* SEALED must match the immutable BEGIN/destination identity. */
    used = begin_length + sealed_length;
    memcpy(changed, begin, begin_length);
    memcpy(changed + begin_length, sealed, sealed_length);
    {
        char *stage_dev = strstr(changed + begin_length, "|1|6|1000|");
        CHECK(stage_dev != NULL);
        stage_dev[1] = '2';
    }
    verify_fixture(changed, used, false, 0U);

    /* A valid two-record history remains accepted. */
    memcpy(journal, begin, begin_length);
    memcpy(journal + begin_length, sealed, sealed_length);
    verify_fixture(journal, used, true, 2U);
}

static void test_valid_prefix_plus_invalid_suffix(void)
{
    char journal[FIXTURE_CAPACITY];
    char combined[FIXTURE_CAPACITY];
    char move[256];
    size_t begin_length = 0U;
    size_t valid_length = make_two_record_journal(journal, sizeof(journal),
                                                  &begin_length);

    /* Positive control: the complete prefix alone is valid. */
    verify_fixture(journal, valid_length, true, 2U);

    /* One-byte truncated suffix. */
    memcpy(combined, journal, valid_length);
    combined[valid_length] = 'F';
    verify_fixture(combined, valid_length + 1U, false, 0U);

    /* Complete but malformed physical line. */
    memcpy(combined, journal, valid_length);
    memcpy(combined + valid_length, "FCJ4|UNKNOWN|1|x\n", 17U);
    verify_fixture(combined, valid_length + 17U, false, 0U);

    /* Complete, framed, semantically invalid duplicate BEGIN. */
    memcpy(combined, journal, valid_length);
    memcpy(combined + valid_length, journal, begin_length);
    verify_fixture(combined, valid_length + begin_length, false, 0U);

    /* Invalid record after a valid prefix must not expose the prefix. */
    {
        Fcj4Record record;
        size_t move_length = 0U;
        memset(&record, 0, sizeof(record));
        record.kind = FCJ4_RECORD_MOVE;
        record.value.move = (Fcj4MoveRecord){
            "22222222222222222222222222222222",
            1U, FCJ4_MOVE_STAGED, 0U
        };
        CHECK(fcj4_serialize(&record, move, sizeof(move), &move_length) == 0);
        memcpy(combined, journal, valid_length);
        memcpy(combined + valid_length, move, move_length);
        verify_fixture(combined, valid_length + move_length, false, 0U);
    }

    /* An extra blank line after a valid journal is not silently ignored. */
    memcpy(combined, journal, valid_length);
    combined[valid_length] = '\n';
    verify_fixture(combined, valid_length + 1U, false, 0U);
}

int main(void)
{
    test_every_strict_prefix();
    test_physical_framing_corruption();
    test_semantic_corruption();
    test_sealed_digest_corruption();
    test_valid_prefix_plus_invalid_suffix();
    test_child_exit_with_partial_record();
    puts("FCJ4 malformed/truncated restart matrix: all tests passed");
    return EXIT_SUCCESS;
}
