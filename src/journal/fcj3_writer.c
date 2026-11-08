#define _GNU_SOURCE

#include "journal/fcj3_writer.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/random.h>

#ifdef FC_TESTING
static atomic_int test_random_error;

void fcj3_test_set_random_error(int error_code)
{
    atomic_store_explicit(&test_random_error, error_code, memory_order_release);
}
#endif

static ssize_t fcj3_getrandom(void *buffer, size_t length)
{
#ifdef FC_TESTING
    int injected = atomic_exchange_explicit(&test_random_error, 0,
                                            memory_order_acq_rel);
    if (injected != 0)
    {
        errno = injected;
        return -1;
    }
#endif
    return getrandom(buffer, length, 0);
}

static int secure_random_id(char output[FCJ3_OPERATION_ID_HEX_LENGTH + 1U])
{
    static const char digits[] = "0123456789abcdef";
    unsigned char random_bytes[16];
    size_t offset = 0;

    while (offset < sizeof(random_bytes))
    {
        ssize_t received = fcj3_getrandom(random_bytes + offset,
                                          sizeof(random_bytes) - offset);
        if (received < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (received == 0)
        {
            errno = EIO;
            return -1;
        }
        offset += (size_t)received;
    }

    for (size_t i = 0; i < sizeof(random_bytes); i++)
    {
        output[i * 2U] = digits[random_bytes[i] >> 4U];
        output[i * 2U + 1U] = digits[random_bytes[i] & 0x0fU];
    }
    output[FCJ3_OPERATION_ID_HEX_LENGTH] = '\0';
    return 0;
}

static int valid_staging_component(const char *component, size_t length)
{
    if (!component || length <= 8U || length >= NAME_MAX ||
        strncmp(component, ".fc-tmp-", 8U) != 0)
        return 0;

    for (size_t i = 0; i < length; i++)
    {
        unsigned char c = (unsigned char)component[i];
        if (c == '/' || c < 0x20U || c == 0x7fU)
            return 0;
    }
    return 1;
}

static const char *state_name(Fcj3State state)
{
    switch (state)
    {
        case FCJ3_STATE_STAGED: return "STAGED";
        case FCJ3_STATE_STARTED: return "STARTED";
        case FCJ3_STATE_PUBLISHED: return "PUBLISHED";
        case FCJ3_STATE_COMPLETED: return "COMPLETED";
        case FCJ3_STATE_FAILED: return "FAILED";
        case FCJ3_STATE_RECOVERED: return "RECOVERED";
        default: return NULL;
    }
}

static int dev_to_u64(dev_t value, uint64_t *output)
{
    uintmax_t wide = (uintmax_t)value;
    uint64_t narrowed = (uint64_t)wide;
    if (!output || (uintmax_t)narrowed != wide)
        return -1;
    *output = narrowed;
    return 0;
}

static int ino_to_u64(ino_t value, uint64_t *output)
{
    uintmax_t wide = (uintmax_t)value;
    uint64_t narrowed = (uint64_t)wide;
    if (!output || (uintmax_t)narrowed != wide)
        return -1;
    *output = narrowed;
    return 0;
}

static int hex_encode(const char *input, size_t length,
                      char *output, size_t capacity)
{
    static const char digits[] = "0123456789abcdef";
    if (!input || !output || length > (capacity - 1U) / 2U)
    {
        errno = EOVERFLOW;
        return -1;
    }

    for (size_t i = 0; i < length; i++)
    {
        unsigned char byte = (unsigned char)input[i];
        output[i * 2U] = digits[byte >> 4U];
        output[i * 2U + 1U] = digits[byte & 0x0fU];
    }
    output[length * 2U] = '\0';
    return 0;
}

int fcj3_operation_init(Fcj3OperationContext *context,
                        const char *source, const char *destination,
                        const struct stat *destination_directory,
                        const char *staging_component,
                        const struct stat *staging_inode)
{
    size_t source_length;
    size_t destination_length;
    size_t staging_length;
    uint64_t identity_check;

    if (!context)
    {
        errno = EINVAL;
        return -1;
    }
    memset(context, 0, sizeof(*context));

    if (!source || !destination || !destination_directory ||
        !staging_component || !staging_inode)
    {
        errno = EINVAL;
        return -1;
    }

    source_length = strnlen(source, PATH_MAX);
    destination_length = strnlen(destination, PATH_MAX);
    staging_length = strnlen(staging_component, NAME_MAX);
    if (source_length == 0 || source_length >= PATH_MAX ||
        destination_length == 0 || destination_length >= PATH_MAX ||
        !S_ISDIR(destination_directory->st_mode) ||
        !S_ISREG(staging_inode->st_mode) ||
        staging_inode->st_nlink != 0 ||
        staging_inode->st_dev != destination_directory->st_dev ||
        !valid_staging_component(staging_component, staging_length))
    {
        errno = EINVAL;
        return -1;
    }

    if (dev_to_u64(destination_directory->st_dev, &identity_check) != 0 ||
        ino_to_u64(destination_directory->st_ino, &identity_check) != 0 ||
        dev_to_u64(staging_inode->st_dev, &identity_check) != 0 ||
        ino_to_u64(staging_inode->st_ino, &identity_check) != 0)
    {
        errno = EOVERFLOW;
        return -1;
    }
    (void)identity_check;

    if (secure_random_id(context->operation_id) != 0)
    {
        int saved_errno = errno;
        memset(context, 0, sizeof(*context));
        errno = saved_errno;
        return -1;
    }

    memcpy(context->source, source, source_length + 1U);
    memcpy(context->destination, destination, destination_length + 1U);
    memcpy(context->staging_component, staging_component, staging_length + 1U);
    context->destination_dir_dev = destination_directory->st_dev;
    context->destination_dir_ino = destination_directory->st_ino;
    context->staging_dev = staging_inode->st_dev;
    context->staging_ino = staging_inode->st_ino;
    context->initialized = 1;
    return 0;
}

void fcj3_operation_reset(Fcj3OperationContext *context)
{
    if (context)
        memset(context, 0, sizeof(*context));
}

static int encode_record(const Fcj3OperationContext *context,
                         Fcj3State state, int error_code,
                         uint64_t sequence, uint64_t timestamp_ns,
                         Fcj3PreparedRecord *prepared)
{
    char source_hex[PATH_MAX * 2U + 1U];
    char destination_hex[PATH_MAX * 2U + 1U];
    char staging_hex[NAME_MAX * 2U + 1U];
    size_t source_length = strlen(context->source);
    size_t destination_length = strlen(context->destination);
    size_t staging_length = strlen(context->staging_component);
    uint64_t destination_dir_dev;
    uint64_t destination_dir_ino;
    uint64_t staging_dev;
    uint64_t staging_ino;

    if (dev_to_u64(context->destination_dir_dev, &destination_dir_dev) != 0 ||
        ino_to_u64(context->destination_dir_ino, &destination_dir_ino) != 0 ||
        dev_to_u64(context->staging_dev, &staging_dev) != 0 ||
        ino_to_u64(context->staging_ino, &staging_ino) != 0)
    {
        errno = EOVERFLOW;
        return -1;
    }

    if (hex_encode(context->source, source_length,
                   source_hex, sizeof(source_hex)) != 0 ||
        hex_encode(context->destination, destination_length,
                   destination_hex, sizeof(destination_hex)) != 0 ||
        hex_encode(context->staging_component, staging_length,
                   staging_hex, sizeof(staging_hex)) != 0)
        return -1;

    int length = snprintf(prepared->line, sizeof(prepared->line),
        "FCJ3|%" PRIu64 "|%s|%" PRIu64 "|%s|%d|%zu|%zu|%" PRIu64
        "|%" PRIu64 "|%" PRIu64 "|%" PRIu64 "|%zu|%s|%s|%s\n",
        timestamp_ns, context->operation_id, sequence, state_name(state),
        error_code, source_length, destination_length,
        destination_dir_dev, destination_dir_ino, staging_dev, staging_ino,
        staging_length, source_hex, destination_hex, staging_hex);

    if (length < 0 || (size_t)length > FCJ3_MAX_LINE_BYTES)
    {
        errno = EOVERFLOW;
        return -1;
    }

    prepared->length = (size_t)length;
    return 0;
}

int fcj3_operation_prepare_transition(const Fcj3OperationContext *context,
                                     Fcj3State next_state, int error_code,
                                     uint64_t timestamp_ns,
                                     Fcj3PreparedRecord *prepared)
{
    const char *name;
    uint64_t next_sequence;

    if (!context || !prepared || !context->initialized)
    {
        errno = EINVAL;
        return -1;
    }
    if (context->poisoned)
    {
        errno = EIO;
        return -1;
    }

    name = state_name(next_state);
    if (!name || !fcj3_transition_is_valid(context->state, next_state) ||
        !fcj3_error_code_is_valid(next_state, error_code) ||
        context->sequence >= 4U)
    {
        errno = EBADMSG;
        return -1;
    }

    next_sequence = context->sequence + 1U;
    memset(prepared, 0, sizeof(*prepared));
    if (encode_record(context, next_state, error_code, next_sequence,
                      timestamp_ns, prepared) != 0)
        return -1;

    memcpy(prepared->operation_id, context->operation_id,
           sizeof(prepared->operation_id));
    prepared->previous_sequence = context->sequence;
    prepared->previous_state = context->state;
    prepared->sequence = next_sequence;
    prepared->state = next_state;
    prepared->error_code = error_code;
    prepared->timestamp_ns = timestamp_ns;
    prepared->valid = 1;
    return 0;
}

int fcj3_operation_commit_transition(Fcj3OperationContext *context,
                                     const Fcj3PreparedRecord *prepared)
{
    Fcj3PreparedRecord expected;

    if (!context || !prepared || !context->initialized || !prepared->valid)
    {
        errno = EINVAL;
        return -1;
    }
    if (context->poisoned)
    {
        errno = EIO;
        return -1;
    }
    if (strcmp(context->operation_id, prepared->operation_id) != 0 ||
        context->sequence != prepared->previous_sequence ||
        context->state != prepared->previous_state ||
        prepared->sequence != context->sequence + 1U ||
        !fcj3_transition_is_valid(context->state, prepared->state) ||
        !fcj3_error_code_is_valid(prepared->state, prepared->error_code) ||
        prepared->length == 0 ||
        prepared->length > FCJ3_MAX_LINE_BYTES)
    {
        errno = ESTALE;
        return -1;
    }

    memset(&expected, 0, sizeof(expected));
    if (encode_record(context, prepared->state, prepared->error_code,
                      prepared->sequence, prepared->timestamp_ns, &expected) != 0 ||
        expected.length != prepared->length ||
        memcmp(expected.line, prepared->line, prepared->length) != 0)
    {
        errno = ESTALE;
        return -1;
    }

    context->sequence = prepared->sequence;
    context->state = prepared->state;
    return 0;
}

void fcj3_operation_mark_uncertain(Fcj3OperationContext *context)
{
    if (context && context->initialized)
        context->poisoned = 1;
}
