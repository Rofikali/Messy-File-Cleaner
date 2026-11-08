#define _POSIX_C_SOURCE 200809L

#include "journal/fcj3_parser.h"

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK_RESULT(expression, expected) do { \
    int check_result_ = (expression); \
    assert(check_result_ == (expected)); \
} while (0)

static const char *const valid_id = "0123456789abcdef0123456789abcdef";

static size_t make_line(char *output, size_t capacity, const char *id,
                        uint64_t sequence, const char *state, int error_code)
{
    int length = snprintf(output, capacity,
        "FCJ3|%" PRIu64 "|%s|%" PRIu64 "|%s|%d|5|6|1|2|1|3|9|"
        "2f73726331|2f6465737431|2e66632d746d702d31\n",
        sequence, id, sequence, state, error_code);
    assert(length > 0 && (size_t)length < capacity);
    return (size_t)length;
}

static void expect_bad(const char *line, size_t length, int expected_errno)
{
    Fcj3Replay replay;
    fcj3_replay_init(&replay);
    errno = 0;
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), -1);
    assert(errno == expected_errno);
    assert(replay.count == 0);
    fcj3_replay_destroy(&replay);
}

static void test_valid_state_machine_and_identity(void)
{
    char line[512];
    size_t length;
    Fcj3Replay replay;
    fcj3_replay_init(&replay);

    length = make_line(line, sizeof(line), valid_id, 1, "STAGED", 0);
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), 0);
    assert(replay.count == 1);
    assert(replay.transactions[0].state == FCJ3_STATE_STAGED);
    assert(strcmp(replay.transactions[0].source, "/src1") == 0);
    assert(strcmp(replay.transactions[0].destination, "/dest1") == 0);
    assert(strcmp(replay.transactions[0].staging_component, ".fc-tmp-1") == 0);

    length = make_line(line, sizeof(line), valid_id, 2, "STARTED", 0);
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), 0);
    length = make_line(line, sizeof(line), valid_id, 3, "PUBLISHED", 0);
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), 0);
    length = make_line(line, sizeof(line), valid_id, 4, "COMPLETED", 0);
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), 0);
    assert(replay.count == 1);
    assert(replay.transactions[0].sequence == 4);
    assert(replay.transactions[0].state == FCJ3_STATE_COMPLETED);

    /* Same path pair, different operation IDs: two independent transactions. */
    const char *second_id = "1123456789abcdef0123456789abcdef";
    length = make_line(line, sizeof(line), second_id, 1, "STAGED", 0);
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), 0);
    assert(replay.count == 2);

    fcj3_replay_destroy(&replay);
}

static void test_alternate_terminal_transitions(void)
{
    char line[512];
    size_t length;
    Fcj3Replay replay;
    fcj3_replay_init(&replay);

    length = make_line(line, sizeof(line), valid_id, 1, "STAGED", 0);
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), 0);
    length = make_line(line, sizeof(line), valid_id, 2, "FAILED", EIO);
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), 0);
    assert(replay.transactions[0].state == FCJ3_STATE_FAILED);
    fcj3_replay_destroy(&replay);

    fcj3_replay_init(&replay);
    length = make_line(line, sizeof(line), valid_id, 1, "STAGED", 0);
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), 0);
    length = make_line(line, sizeof(line), valid_id, 2, "RECOVERED", 0);
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), 0);
    assert(replay.transactions[0].state == FCJ3_STATE_RECOVERED);
    fcj3_replay_destroy(&replay);
}

static void test_transition_and_identity_rejection(void)
{
    char line[512];
    size_t length;
    Fcj3Replay replay;
    fcj3_replay_init(&replay);

    length = make_line(line, sizeof(line), valid_id, 1, "STAGED", 0);
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), 0);

    /* Duplicate sequence, gap, and illegal transition must not mutate state. */
    errno = 0;
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), -1);
    assert(errno == EBADMSG);
    assert(replay.transactions[0].sequence == 1);

    length = make_line(line, sizeof(line), valid_id, 3, "PUBLISHED", 0);
    errno = 0;
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), -1);
    assert(errno == EBADMSG);
    assert(replay.transactions[0].sequence == 1);

    length = make_line(line, sizeof(line), valid_id, 2, "PUBLISHED", 0);
    errno = 0;
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), -1);
    assert(errno == EBADMSG);

    /* A staging inode on another device cannot belong to this O_TMPFILE directory. */
    length = make_line(line, sizeof(line), valid_id, 1, "STAGED", 0);
    char *device_identity = strstr(line, "|1|2|1|3|9|");
    assert(device_identity != NULL);
    device_identity[5] = '9';
    expect_bad(line, length, EBADMSG);

    /* A changed staging inode under the same operation ID is corruption. */
    length = make_line(line, sizeof(line), valid_id, 2, "STARTED", 0);
    char *stage_identity = strstr(line, "|1|3|9|");
    assert(stage_identity != NULL);
    stage_identity[3] = '4'; /* staging inode becomes 4, not 3 */
    errno = 0;
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), -1);
    assert(errno == EBADMSG);
    assert(replay.transactions[0].sequence == 1);

    fcj3_replay_destroy(&replay);

    /* No records may follow a terminal state. */
    fcj3_replay_init(&replay);
    length = make_line(line, sizeof(line), valid_id, 1, "STAGED", 0);
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), 0);
    length = make_line(line, sizeof(line), valid_id, 2, "FAILED", EIO);
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), 0);
    length = make_line(line, sizeof(line), valid_id, 3, "RECOVERED", 0);
    errno = 0;
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), -1);
    assert(errno == EBADMSG);
    assert(replay.transactions[0].sequence == 2);
    fcj3_replay_destroy(&replay);
}

static void test_canonical_encoding_rejection(void)
{
    char line[512];
    size_t length = make_line(line, sizeof(line), valid_id, 1, "STAGED", 0);

    char changed[512];
    memcpy(changed, line, length + 1U);
    changed[5] = '0'; /* timestamp 1 becomes non-canonical 01 */
    {
        char *separator = strchr(changed + 5, '|');
        assert(separator != NULL);
        memmove(changed + 7, separator, strlen(separator) + 1U);
        changed[5] = '0';
        changed[6] = '1';
        expect_bad(changed, length + 1U, EBADMSG);
    }

    memcpy(changed, line, length + 1U);
    changed[5] = '+';
    expect_bad(changed, length, EBADMSG);

    memcpy(changed, line, 5U);
    memcpy(changed + 5, "18446744073709551616", 20U);
    memcpy(changed + 25, line + 6, length - 6U + 1U);
    expect_bad(changed, length + 19U, EBADMSG);

    memcpy(changed, line, length + 1U);
    changed[8] = 'A'; /* uppercase operation ID */
    expect_bad(changed, length, EBADMSG);

    memcpy(changed, line, length + 1U);
    char *source_hex = strstr(changed, "|2f73726331|");
    assert(source_hex != NULL);
    source_hex[2] = 'F'; /* uppercase hex is not canonical */
    expect_bad(changed, length, EBADMSG);

    memcpy(changed, line, length + 1U);
    source_hex = strstr(changed, "|2f73726331|");
    assert(source_hex != NULL);
    source_hex[1] = '0';
    source_hex[2] = '0'; /* decoded NUL byte */
    expect_bad(changed, length, EBADMSG);

    memcpy(changed, line, length + 1U);
    char *stage_hex = strstr(changed, "|2e66632d746d702d31\n");
    assert(stage_hex != NULL);
    stage_hex[1] = '2';
    stage_hex[2] = 'f'; /* slash in a single-component staging name */
    expect_bad(changed, length, EBADMSG);

    memcpy(changed, line, length + 1U);
    changed[length - 1U] = '\0'; /* missing final LF */
    expect_bad(changed, length, EBADMSG);

    memcpy(changed, line, length + 1U);
    changed[length - 1U] = '\r';
    changed[length] = '\n';
    expect_bad(changed, length + 1U, EBADMSG);

    memcpy(changed, line, length + 1U);
    changed[length - 1U] = '|'; /* seventeenth field */
    changed[length] = '\n';
    expect_bad(changed, length + 1U, EBADMSG);

    /* FCJ2 is deliberately not interpreted by the FCJ3 parser. */
    memcpy(changed, line, length + 1U);
    memcpy(changed, "FCJ2", 4U);
    expect_bad(changed, length, EBADMSG);
}

static void test_invalid_state_error_and_component_rules(void)
{
    char line[512];
    size_t length;
    Fcj3Replay replay;

    fcj3_replay_init(&replay);
    length = make_line(line, sizeof(line), valid_id, 1, "STAGED", EIO);
    expect_bad(line, length, EBADMSG);

    length = make_line(line, sizeof(line), valid_id, 2, "FAILED", 0);
    expect_bad(line, length, EBADMSG);

    length = make_line(line, sizeof(line), valid_id, 1, "STARTED", 0);
    expect_bad(line, length, EBADMSG);

    /* The reserved staging namespace requires the exact prefix and a suffix. */
    int bad_prefix_length = snprintf(line, sizeof(line),
        "FCJ3|1|%s|1|STAGED|0|5|6|1|2|1|3|9|"
        "2f73726331|2f6465737431|6e6f742d746d702d31\\n", valid_id);
    assert(bad_prefix_length > 0 && (size_t)bad_prefix_length < sizeof(line));
    expect_bad(line, (size_t)bad_prefix_length, EBADMSG);

    int empty_suffix_length = snprintf(line, sizeof(line),
        "FCJ3|1|%s|1|STAGED|0|5|6|1|2|1|3|8|"
        "2f73726331|2f6465737431|2e66632d746d702d\\n", valid_id);
    assert(empty_suffix_length > 0 && (size_t)empty_suffix_length < sizeof(line));
    expect_bad(line, (size_t)empty_suffix_length, EBADMSG);

    length = make_line(line, sizeof(line), valid_id, 1, "STAGED", 0);
    char *stage_hex = strstr(line, "|2e66632d746d702d31\n");
    assert(stage_hex != NULL);
    /* Replace the component with a single dot (length and hex length updated). */
    memcpy(stage_hex, "|2e\n", 4U);
    /* This intentionally also makes the line structurally short; it is rejected. */
    expect_bad(line, strlen(line), EBADMSG);
    fcj3_replay_destroy(&replay);
}

static void test_bounds_and_transaction_capacity(void)
{
    char *oversized = malloc(FCJ3_MAX_LINE_BYTES + 2U);
    assert(oversized != NULL);
    memset(oversized, 'A', FCJ3_MAX_LINE_BYTES + 1U);
    oversized[FCJ3_MAX_LINE_BYTES] = '\n';
    expect_bad(oversized, FCJ3_MAX_LINE_BYTES + 1U, E2BIG);
    free(oversized);

    Fcj3Replay replay;
    fcj3_replay_init(&replay);
    char line[512];
    for (size_t i = 0; i < FCJ3_MAX_TRANSACTIONS; i++)
    {
        char id[FCJ3_OPERATION_ID_HEX_LENGTH + 1U];
        int id_length = snprintf(id, sizeof(id), "%032" PRIx64, (uint64_t)i);
        assert(id_length == (int)FCJ3_OPERATION_ID_HEX_LENGTH);
        size_t length = make_line(line, sizeof(line), id, 1, "STAGED", 0);
        CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), 0);
    }
    assert(replay.count == FCJ3_MAX_TRANSACTIONS);

    char extra_id[FCJ3_OPERATION_ID_HEX_LENGTH + 1U];
    (void)snprintf(extra_id, sizeof(extra_id), "%032x", 4096U);
    size_t length = make_line(line, sizeof(line), extra_id, 1, "STAGED", 0);
    errno = 0;
    CHECK_RESULT(fcj3_replay_add_line(&replay, line, length), -1);
    assert(errno == E2BIG);
    assert(replay.count == FCJ3_MAX_TRANSACTIONS);
    fcj3_replay_destroy(&replay);
}

int main(void)
{
    test_valid_state_machine_and_identity();
    test_alternate_terminal_transitions();
    test_transition_and_identity_rejection();
    test_canonical_encoding_rejection();
    test_invalid_state_error_and_component_rules();
    test_bounds_and_transaction_capacity();

    puts("FCJ3 read-only parser tests passed");
    return 0;
}
