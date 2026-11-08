#define _POSIX_C_SOURCE 200809L

#include "journal/fcj3_parser.h"

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    Fcj3Transaction transaction;
    size_t source_length;
    size_t destination_length;
    size_t staging_length;
} ParsedRecord;

static int parse_u64_canonical(const char *text, uint64_t *value)
{
    uint64_t result = 0;

    if (!text || !text[0] || !value)
        return -1;
    if (text[0] == '0' && text[1] != '\0')
        return -1;

    for (const unsigned char *cursor = (const unsigned char *)text;
         *cursor != '\0'; cursor++)
    {
        unsigned int digit;
        if (*cursor < '0' || *cursor > '9')
            return -1;
        digit = (unsigned int)(*cursor - '0');
        if (result > (UINT64_MAX - digit) / 10U)
            return -1;
        result = result * 10U + digit;
    }

    *value = result;
    return 0;
}

static int is_lower_hex_string(const char *text, size_t expected_length)
{
    if (!text || strlen(text) != expected_length)
        return 0;

    for (size_t i = 0; i < expected_length; i++)
    {
        unsigned char c = (unsigned char)text[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            return 0;
    }
    return 1;
}

static int decode_hex_field(const char *hex, size_t hex_length,
                            size_t decoded_length, char *output,
                            size_t output_capacity)
{
    if (!hex || !output || decoded_length == 0 ||
        decoded_length >= output_capacity ||
        decoded_length > SIZE_MAX / 2U ||
        hex_length != decoded_length * 2U)
        return -1;

    for (size_t i = 0; i < decoded_length; i++)
    {
        unsigned char high = (unsigned char)hex[i * 2U];
        unsigned char low = (unsigned char)hex[i * 2U + 1U];
        unsigned int high_value;
        unsigned int low_value;

        if (high >= '0' && high <= '9')
            high_value = (unsigned int)(high - '0');
        else if (high >= 'a' && high <= 'f')
            high_value = (unsigned int)(high - 'a') + 10U;
        else
            return -1;

        if (low >= '0' && low <= '9')
            low_value = (unsigned int)(low - '0');
        else if (low >= 'a' && low <= 'f')
            low_value = (unsigned int)(low - 'a') + 10U;
        else
            return -1;

        unsigned char decoded = (unsigned char)((high_value << 4U) | low_value);
        if (decoded == 0)
            return -1;
        output[i] = (char)decoded;
    }

    output[decoded_length] = '\0';
    return 0;
}

static int parse_state(const char *text, Fcj3State *state)
{
    if (strcmp(text, "STAGED") == 0)
        *state = FCJ3_STATE_STAGED;
    else if (strcmp(text, "STARTED") == 0)
        *state = FCJ3_STATE_STARTED;
    else if (strcmp(text, "PUBLISHED") == 0)
        *state = FCJ3_STATE_PUBLISHED;
    else if (strcmp(text, "COMPLETED") == 0)
        *state = FCJ3_STATE_COMPLETED;
    else if (strcmp(text, "FAILED") == 0)
        *state = FCJ3_STATE_FAILED;
    else if (strcmp(text, "RECOVERED") == 0)
        *state = FCJ3_STATE_RECOVERED;
    else
        return -1;
    return 0;
}

int fcj3_error_code_is_valid(Fcj3State state, int error_code)
{
    if (error_code < 0)
        return 0;
    if (state == FCJ3_STATE_FAILED)
        return error_code != 0;
    if (state == FCJ3_STATE_RECOVERED)
        return 1;
    return error_code == 0;
}

static int fits_dev_t(uint64_t value)
{
    dev_t converted = (dev_t)value;
    return (uintmax_t)converted == (uintmax_t)value;
}

static int fits_ino_t(uint64_t value)
{
    ino_t converted = (ino_t)value;
    return (uintmax_t)converted == (uintmax_t)value;
}

static int valid_staging_component(const char *component, size_t length)
{
    /* The recovery design reserves the .fc-tmp- namespace for journal-owned
     * staging entries. Require a nonempty suffix; the name alone is never
     * treated as proof of ownership. */
    if (!component || length <= 8U || length >= NAME_MAX ||
        strncmp(component, ".fc-tmp-", 8U) != 0 ||
        strcmp(component, ".") == 0 || strcmp(component, "..") == 0)
        return 0;

    for (size_t i = 0; i < length; i++)
    {
        unsigned char c = (unsigned char)component[i];
        if (c == '/' || c < 0x20U || c == 0x7fU)
            return 0;
    }
    return 1;
}

static int split_fields(char *buffer, size_t content_length,
                        char *fields[16])
{
    size_t count = 1;
    fields[0] = buffer;

    for (size_t i = 0; i < content_length; i++)
    {
        if (buffer[i] == '|')
        {
            if (count >= 16U)
                return -1;
            buffer[i] = '\0';
            fields[count++] = &buffer[i + 1U];
        }
    }
    return count == 16U ? 0 : -1;
}

static int parse_record(const char *line, size_t length, ParsedRecord *parsed)
{
    char buffer[FCJ3_MAX_LINE_BYTES + 1U];
    char *fields[16] = {0};
    uint64_t numbers[10] = {0};
    size_t content_length;

    if (!line || !parsed || length == 0)
        goto malformed;
    if (length > FCJ3_MAX_LINE_BYTES)
    {
        errno = E2BIG;
        return -1;
    }
    if (line[length - 1U] != '\n' ||
        memchr(line, '\0', length) != NULL)
        goto malformed;

    content_length = length - 1U;
    if (content_length == 0 || line[content_length - 1U] == '\r')
        goto malformed;

    memcpy(buffer, line, content_length);
    buffer[content_length] = '\0';

    if (split_fields(buffer, content_length, fields) != 0 ||
        strcmp(fields[0], "FCJ3") != 0 ||
        !is_lower_hex_string(fields[2], FCJ3_OPERATION_ID_HEX_LENGTH))
        goto malformed;

    /* Numeric field indexes: timestamp, sequence, error, source length,
     * destination length, directory dev/ino, staging dev/ino, stage length. */
    static const size_t numeric_fields[10] = {1, 3, 5, 6, 7, 8, 9, 10, 11, 12};
    for (size_t i = 0; i < 10U; i++)
    {
        if (parse_u64_canonical(fields[numeric_fields[i]], &numbers[i]) != 0)
            goto malformed;
    }

    Fcj3Transaction *tx = &parsed->transaction;
    memset(tx, 0, sizeof(*tx));

    if (numbers[3] == 0 || numbers[3] >= PATH_MAX ||
        numbers[4] == 0 || numbers[4] >= PATH_MAX ||
        numbers[9] == 0 || numbers[9] >= NAME_MAX ||
        numbers[2] > (uint64_t)INT_MAX ||
        numbers[1] == 0 || numbers[1] > 4U ||
        !fits_dev_t(numbers[5]) || !fits_ino_t(numbers[6]) ||
        !fits_dev_t(numbers[7]) || !fits_ino_t(numbers[8]) ||
        numbers[5] != numbers[7])
        goto malformed;

    if (parse_state(fields[4], &tx->state) != 0 ||
        !fcj3_error_code_is_valid(tx->state, (int)numbers[2]))
        goto malformed;

    memcpy(tx->operation_id, fields[2], FCJ3_OPERATION_ID_HEX_LENGTH + 1U);
    tx->timestamp_ns = numbers[0];
    tx->sequence = numbers[1];
    tx->error_code = (int)numbers[2];
    tx->destination_dir_dev = (dev_t)numbers[5];
    tx->destination_dir_ino = (ino_t)numbers[6];
    tx->staging_dev = (dev_t)numbers[7];
    tx->staging_ino = (ino_t)numbers[8];

    parsed->source_length = (size_t)numbers[3];
    parsed->destination_length = (size_t)numbers[4];
    parsed->staging_length = (size_t)numbers[9];

    if (decode_hex_field(fields[13], strlen(fields[13]),
                         parsed->source_length, tx->source,
                         sizeof(tx->source)) != 0 ||
        decode_hex_field(fields[14], strlen(fields[14]),
                         parsed->destination_length, tx->destination,
                         sizeof(tx->destination)) != 0 ||
        decode_hex_field(fields[15], strlen(fields[15]),
                         parsed->staging_length, tx->staging_component,
                         sizeof(tx->staging_component)) != 0 ||
        !valid_staging_component(tx->staging_component,
                                 parsed->staging_length))
        goto malformed;

    return 0;

malformed:
    errno = EBADMSG;
    return -1;
}

static int metadata_matches(const Fcj3Transaction *left,
                            const Fcj3Transaction *right)
{
    return strcmp(left->operation_id, right->operation_id) == 0 &&
           strcmp(left->source, right->source) == 0 &&
           strcmp(left->destination, right->destination) == 0 &&
           strcmp(left->staging_component, right->staging_component) == 0 &&
           left->destination_dir_dev == right->destination_dir_dev &&
           left->destination_dir_ino == right->destination_dir_ino &&
           left->staging_dev == right->staging_dev &&
           left->staging_ino == right->staging_ino;
}

int fcj3_transition_is_valid(Fcj3State previous, Fcj3State next)
{
    switch (previous)
    {
        case 0:
            return next == FCJ3_STATE_STAGED;
        case FCJ3_STATE_STAGED:
            return next == FCJ3_STATE_STARTED ||
                   next == FCJ3_STATE_FAILED ||
                   next == FCJ3_STATE_RECOVERED;
        case FCJ3_STATE_STARTED:
            return next == FCJ3_STATE_PUBLISHED ||
                   next == FCJ3_STATE_FAILED ||
                   next == FCJ3_STATE_RECOVERED;
        case FCJ3_STATE_PUBLISHED:
            return next == FCJ3_STATE_COMPLETED ||
                   next == FCJ3_STATE_RECOVERED;
        case FCJ3_STATE_COMPLETED:
        case FCJ3_STATE_FAILED:
        case FCJ3_STATE_RECOVERED:
        default:
            return 0;
    }
}

void fcj3_replay_init(Fcj3Replay *replay)
{
    if (replay)
        memset(replay, 0, sizeof(*replay));
}

void fcj3_replay_destroy(Fcj3Replay *replay)
{
    if (!replay)
        return;
    free(replay->transactions);
    memset(replay, 0, sizeof(*replay));
}

int fcj3_replay_add_line(Fcj3Replay *replay, const char *line, size_t length)
{
    ParsedRecord parsed;
    if (!replay)
    {
        errno = EINVAL;
        return -1;
    }
    if (parse_record(line, length, &parsed) != 0)
        return -1;

    for (size_t i = 0; i < replay->count; i++)
    {
        Fcj3Transaction *existing = &replay->transactions[i];
        if (strcmp(existing->operation_id, parsed.transaction.operation_id) != 0)
            continue;

        if (!metadata_matches(existing, &parsed.transaction) ||
            existing->sequence >= 4U ||
            parsed.transaction.sequence != existing->sequence + 1U ||
            !fcj3_transition_is_valid(existing->state, parsed.transaction.state))
        {
            errno = EBADMSG;
            return -1;
        }

        *existing = parsed.transaction;
        return 0;
    }

    if (parsed.transaction.state != FCJ3_STATE_STAGED ||
        parsed.transaction.sequence != 1U)
    {
        errno = EBADMSG;
        return -1;
    }

    if (replay->count >= FCJ3_MAX_TRANSACTIONS)
    {
        errno = E2BIG;
        return -1;
    }

    if (replay->count == replay->capacity)
    {
        size_t next_capacity = replay->capacity == 0 ? 16U : replay->capacity * 2U;
        if (next_capacity > FCJ3_MAX_TRANSACTIONS)
            next_capacity = FCJ3_MAX_TRANSACTIONS;
        if (next_capacity <= replay->capacity ||
            next_capacity > SIZE_MAX / sizeof(*replay->transactions))
        {
            errno = EOVERFLOW;
            return -1;
        }

        Fcj3Transaction *grown = realloc(
            replay->transactions, next_capacity * sizeof(*replay->transactions));
        if (!grown)
            return -1;
        replay->transactions = grown;
        replay->capacity = next_capacity;
    }

    replay->transactions[replay->count++] = parsed.transaction;
    return 0;
}
