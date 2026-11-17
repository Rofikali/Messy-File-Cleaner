#include "fcj4_parser_test_shim.h"
#include "journal/fcj4_parser.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Independently compiled, fixture-only, no files or production runtime. */
#define CHECK(x) do { \
    if (!(x)) { \
        fprintf(stderr, "RUST-01A shim check failed at %s:%d: %s (errno=%d)\\n", \
                __FILE__, __LINE__, #x, errno); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static const unsigned char begin_only[] =
    "FCJ4|BEGIN|1|11111111111111111111111111111111"
    "|11|2f746d702f736f75726365|1|2|1|3|REGULAR|1000|1000"
    "|384|5|1|2|3|4|9|2f746d702f64657374|1|4|4|64657374|1|5\n";

static void check_parse(const unsigned char *bytes, size_t len,
                        int expected_rc, int expected_errno,
                        size_t expected_ops, size_t expected_records)
{
    int error_code = -333;
    size_t ops = 777U, records = 888U;
    errno = 0;
    int rc = fcj4_test_parse_bytes(bytes, len, &error_code, &ops, &records);
    CHECK(rc == expected_rc);
    CHECK(error_code == expected_errno);
    CHECK(ops == expected_ops);
    CHECK(records == expected_records);
    if (expected_rc != 0)
        CHECK(errno == expected_errno);
}

static void test_valid_and_repeated_calls(void)
{
    unsigned char bytes[sizeof(begin_only)];
    memcpy(bytes, begin_only, sizeof(bytes));
    for (unsigned int i = 0U; i < 32U; ++i) {
        check_parse(bytes, sizeof(bytes) - 1U, 0, 0, 1U, 1U);
        CHECK(memcmp(bytes, begin_only, sizeof(bytes)) == 0);
        check_parse((const unsigned char *)"FCJ4|NOPE|1|x\n",
                    sizeof("FCJ4|NOPE|1|x\n") - 1U,
                    -1, EBADMSG, 0U, 0U);
    }
}

static void test_malformed_and_boundaries(void)
{
    const unsigned char embedded_nul[] = {'F','C','J','4','|','B','E','G','I','N',0,'\n'};
    const unsigned char invalid_utf8[] = {0xff, '\n'};
    const unsigned char one_byte = 'a';

    check_parse(NULL, 0U, -1, EBADMSG, 0U, 0U);
    check_parse(begin_only, sizeof(begin_only) - 2U,
                -1, EBADMSG, 0U, 0U); /* missing trailing LF */
    check_parse(embedded_nul, sizeof(embedded_nul),
                -1, EBADMSG, 0U, 0U);
    check_parse(invalid_utf8, sizeof(invalid_utf8),
                -1, EBADMSG, 0U, 0U);
    check_parse(&one_byte, (size_t)FCJ4_MAX_JOURNAL_BYTES + 1U,
                -1, E2BIG, 0U, 0U); /* parser bounds before any read */
    check_parse(NULL, 1U, -2, EINVAL, 0U, 0U);
}

static void test_invalid_output_pointers_are_safe(void)
{
    int error_code = 99;
    size_t ops = 99U, records = 99U;
    CHECK(fcj4_test_parse_bytes(begin_only, sizeof(begin_only) - 1U,
                                NULL, &ops, &records) == -2);
    CHECK(ops == 0U && records == 0U);
    CHECK(fcj4_test_parse_bytes(begin_only, sizeof(begin_only) - 1U,
                                &error_code, NULL, &records) == -2);
    CHECK(error_code == EINVAL && records == 0U);
    error_code = 99; ops = 99U;
    CHECK(fcj4_test_parse_bytes(begin_only, sizeof(begin_only) - 1U,
                                &error_code, &ops, NULL) == -2);
    CHECK(error_code == EINVAL && ops == 0U);
}

int main(void)
{
    test_valid_and_repeated_calls();
    test_malformed_and_boundaries();
    test_invalid_output_pointers_are_safe();
    puts("RUST-01A: C-owned FCJ4 parser shim positive/negative tests passed");
    return EXIT_SUCCESS;
}
