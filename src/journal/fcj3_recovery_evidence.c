#define _POSIX_C_SOURCE 200809L

#include "journal/fcj3_recovery_evidence.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

static int valid_component(const char *component, size_t length)
{
    if (!component || length == 0U || length > (size_t)NAME_MAX)
        return 0;
    if ((length == 1U && component[0] == '.') ||
        (length == 2U && component[0] == '.' && component[1] == '.'))
        return 0;

    for (size_t i = 0U; i < length; ++i)
    {
        unsigned char value = (unsigned char)component[i];
        if (value == '/' || value < 0x20U || value == 0x7fU)
            return 0;
    }
    return 1;
}

/* Validate the recorded path lexically; resolution belongs to the caller. */
static int path_basename(const char *path, size_t capacity,
                         char output[NAME_MAX + 1U])
{
    size_t length;
    size_t component_start = 1U;

    if (!path || path[0] != '/')
        return -1;

    length = strnlen(path, capacity);
    if (length == 0U || length >= capacity || path[length - 1U] == '/')
        return -1;

    for (size_t i = 1U; i <= length; ++i)
    {
        if (i == length || path[i] == '/')
        {
            size_t component_length = i - component_start;
            if (!valid_component(path + component_start, component_length))
                return -1;
            component_start = i + 1U;
        }
        else
        {
            unsigned char value = (unsigned char)path[i];
            if (value < 0x20U || value == 0x7fU)
                return -1;
        }
    }

    size_t basename_start = length;
    while (basename_start > 0U && path[basename_start - 1U] != '/')
        --basename_start;
    size_t basename_length = length - basename_start;
    if (basename_length > (size_t)NAME_MAX)
        return -1;

    memcpy(output, path + basename_start, basename_length);
    output[basename_length] = '\0';
    return 0;
}

static int valid_stage_component(const char *component)
{
    size_t length;

    if (!component)
        return 0;
    length = strnlen(component, NAME_MAX + 1U);
    if (length >= NAME_MAX || length <= 8U ||
        memcmp(component, ".fc-tmp-", 8U) != 0)
        return 0;
    return valid_component(component, length);
}

static int stat_directory(int directory_fd, struct stat *metadata,
                          int *error_number)
{
    if (fstat(directory_fd, metadata) != 0)
    {
        *error_number = errno;
        return -1;
    }
    if (!S_ISDIR(metadata->st_mode))
    {
        *error_number = ENOTDIR;
        return -1;
    }
    *error_number = 0;
    return 0;
}

static Fcj3EntryEvidence inspect_entry(int directory_fd, const char *component,
                                       dev_t expected_device,
                                       ino_t expected_inode, int *error_number)
{
    struct stat metadata;
    int result;

    do
    {
        result = fstatat(directory_fd, component, &metadata,
                         AT_SYMLINK_NOFOLLOW);
    } while (result != 0 && errno == EINTR);

    if (result != 0)
    {
        *error_number = errno;
        return errno == ENOENT ? FCJ3_EVIDENCE_ABSENT
                               : FCJ3_EVIDENCE_UNKNOWN_ERROR;
    }

    *error_number = 0;
    if (S_ISLNK(metadata.st_mode))
        return FCJ3_EVIDENCE_SYMLINK;
    if (!S_ISREG(metadata.st_mode))
        return FCJ3_EVIDENCE_SPECIAL;
    if (metadata.st_dev == expected_device &&
        metadata.st_ino == expected_inode)
        return FCJ3_EVIDENCE_MATCHING_REGULAR;
    return FCJ3_EVIDENCE_MISMATCHING_REGULAR;
}

static Fcj3SourceEvidence inspect_source(int directory_fd,
                                         const char *component,
                                         mode_t *source_mode,
                                         int *error_number)
{
    struct stat metadata;
    int result;

    do
    {
        result = fstatat(directory_fd, component, &metadata,
                         AT_SYMLINK_NOFOLLOW);
    } while (result != 0 && errno == EINTR);

    if (result != 0)
    {
        *error_number = errno;
        return errno == ENOENT ? FCJ3_SOURCE_ABSENT
                               : FCJ3_SOURCE_UNKNOWN_ERROR;
    }

    *error_number = 0;
    *source_mode = metadata.st_mode;
    return FCJ3_SOURCE_PRESENT;
}

int fcj3_recovery_collect_at(const Fcj3Transaction *transaction,
                            int source_parent_fd,
                            int destination_parent_fd,
                            Fcj3FilesystemEvidence *evidence)
{
    char source_component[NAME_MAX + 1U];
    char destination_component[NAME_MAX + 1U];
    struct stat destination_directory;
    struct stat source_directory;
    int source_directory_error = 0;

    if (!transaction || !evidence || source_parent_fd < 0 ||
        destination_parent_fd < 0)
    {
        errno = EINVAL;
        return -1;
    }

    memset(evidence, 0, sizeof(*evidence));
    evidence->recovery.transaction = transaction;

    if (path_basename(transaction->source, sizeof(transaction->source),
                      source_component) != 0 ||
        path_basename(transaction->destination,
                      sizeof(transaction->destination),
                      destination_component) != 0 ||
        !valid_stage_component(transaction->staging_component))
    {
        errno = EINVAL;
        return -1;
    }

    if (stat_directory(source_parent_fd, &source_directory,
                       &source_directory_error) != 0)
    {
        evidence->recovery.source = FCJ3_SOURCE_UNKNOWN_ERROR;
        evidence->source_errno = source_directory_error;
    }
    else
    {
        evidence->recovery.source =
            inspect_source(source_parent_fd, source_component,
                           &evidence->source_mode, &evidence->source_errno);
    }

    if (stat_directory(destination_parent_fd, &destination_directory,
                       &evidence->directory_errno) != 0)
    {
        evidence->recovery.directory = FCJ3_DIRECTORY_UNKNOWN_ERROR;
        evidence->recovery.stage = FCJ3_EVIDENCE_UNKNOWN_ERROR;
        evidence->recovery.destination = FCJ3_EVIDENCE_UNKNOWN_ERROR;
        evidence->stage_errno = evidence->directory_errno;
        evidence->destination_errno = evidence->directory_errno;
        return 0;
    }

    if (destination_directory.st_dev != transaction->destination_dir_dev ||
        destination_directory.st_ino != transaction->destination_dir_ino)
    {
        evidence->recovery.directory = FCJ3_DIRECTORY_IDENTITY_MISMATCH;
        evidence->recovery.stage = FCJ3_EVIDENCE_UNKNOWN_ERROR;
        evidence->recovery.destination = FCJ3_EVIDENCE_UNKNOWN_ERROR;
        return 0;
    }

    evidence->recovery.directory = FCJ3_DIRECTORY_IDENTITY_MATCH;
    evidence->recovery.stage =
        inspect_entry(destination_parent_fd, transaction->staging_component,
                      transaction->staging_dev, transaction->staging_ino,
                      &evidence->stage_errno);
    evidence->recovery.destination =
        inspect_entry(destination_parent_fd, destination_component,
                      transaction->staging_dev, transaction->staging_ino,
                      &evidence->destination_errno);
    return 0;
}
