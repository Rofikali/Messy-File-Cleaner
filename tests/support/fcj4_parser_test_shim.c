#include "fcj4_parser_test_shim.h"

#include "journal/fcj4_parser.h"

#include <errno.h>

/*
 * Test-only ownership boundary. Fcj4Replay never escapes C.
 * In particular, record the parser's errno BEFORE destroying replay:
 * a cleanup routine is not required to preserve errno.
 */
int fcj4_test_parse_bytes(const unsigned char *bytes, size_t length,
                          int *out_errno, size_t *out_operations,
                          size_t *out_records)
{
    if (out_errno != NULL)
        *out_errno = 0;
    if (out_operations != NULL)
        *out_operations = 0U;
    if (out_records != NULL)
        *out_records = 0U;

    if (out_errno == NULL || out_operations == NULL || out_records == NULL ||
        (bytes == NULL && length != 0U)) {
        if (out_errno != NULL)
            *out_errno = EINVAL;
        errno = EINVAL;
        return -2;
    }

    Fcj4Replay replay;
    fcj4_replay_init(&replay);
    int result = fcj4_replay_parse(bytes, length, &replay);
    int saved_errno = result == 0 ? 0 : errno;

    if (result == 0) {
        *out_operations = replay.count;
        *out_records = replay.record_count;
    }
    fcj4_replay_destroy(&replay);
    *out_errno = saved_errno;
    if (result != 0) {
        errno = saved_errno;
        return -1;
    }
    return 0;
}
