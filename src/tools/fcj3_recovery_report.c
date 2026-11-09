#define _POSIX_C_SOURCE 200809L

#include "journal/fcj3_parser.h"
#include "journal/fcj3_recovery_classifier.h"
#include "journal/fcj3_recovery_evidence.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define FCJ3_REPORT_MAX_JOURNAL_BYTES (64U * 1024U * 1024U)

typedef struct
{
    Fcj3FilesystemEvidence filesystem;
    Fcj3RecoveryDecision decision;
    int source_resolver_errno;
    int destination_resolver_errno;
} ReportTransaction;

static int valid_component(const char *value, size_t length)
{
    if (!value || length == 0U || length > (size_t)NAME_MAX)
        return 0;
    if ((length == 1U && value[0] == '.') ||
        (length == 2U && value[0] == '.' && value[1] == '.'))
        return 0;
    for (size_t i = 0U; i < length; ++i)
    {
        unsigned char c = (unsigned char)value[i];
        if (c == '/' || c < 0x20U || c == 0x7fU)
            return 0;
    }
    return 1;
}

static int open_directory_path(const char *path)
{
    size_t length;
    size_t start = 1U;
    int current_fd;

    if (!path || path[0] != '/')
    {
        errno = EINVAL;
        return -1;
    }
    length = strnlen(path, PATH_MAX);
    if (length == 0U || length >= PATH_MAX ||
        (length > 1U && path[length - 1U] == '/'))
    {
        errno = EINVAL;
        return -1;
    }

    current_fd = open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (current_fd < 0)
        return -1;

    if (length == 1U)
        return current_fd;

    for (size_t i = 1U; i <= length; ++i)
    {
        if (i == length || path[i] == '/')
        {
            size_t component_length = i - start;
            char component[NAME_MAX + 1U];
            int next_fd;
            if (!valid_component(path + start, component_length))
            {
                (void)close(current_fd);
                errno = EINVAL;
                return -1;
            }
            memcpy(component, path + start, component_length);
            component[component_length] = '\0';
            do
            {
                next_fd = openat(current_fd, component,
                                 O_RDONLY | O_DIRECTORY | O_CLOEXEC |
                                     O_NOFOLLOW);
            } while (next_fd < 0 && errno == EINTR);
            if (next_fd < 0)
            {
                int saved_errno = errno;
                (void)close(current_fd);
                errno = saved_errno;
                return -1;
            }
            if (close(current_fd) != 0)
            {
                int saved_errno = errno;
                (void)close(next_fd);
                errno = saved_errno;
                return -1;
            }
            current_fd = next_fd;
            start = i + 1U;
        }
    }
    return current_fd;
}

/* Resolve an absolute file path's parent without following any symlink. */
static int resolve_parent(const char *path, int *parent_fd,
                          char basename[NAME_MAX + 1U])
{
    size_t length;
    size_t slash;
    size_t basename_length;
    char parent_path[PATH_MAX];

    if (!path || !parent_fd || !basename || path[0] != '/')
    {
        errno = EINVAL;
        return -1;
    }
    length = strnlen(path, PATH_MAX);
    if (length < 2U || length >= PATH_MAX || path[length - 1U] == '/')
    {
        errno = EINVAL;
        return -1;
    }
    slash = length;
    while (slash > 0U && path[slash - 1U] != '/')
        --slash;
    basename_length = length - slash;
    if (!valid_component(path + slash, basename_length))
    {
        errno = EINVAL;
        return -1;
    }
    memcpy(basename, path + slash, basename_length);
    basename[basename_length] = '\0';

    if (slash == 1U)
    {
        parent_path[0] = '/';
        parent_path[1] = '\0';
    }
    else
    {
        size_t parent_length = slash - 1U;
        if (parent_length >= sizeof(parent_path))
        {
            errno = ENAMETOOLONG;
            return -1;
        }
        memcpy(parent_path, path, parent_length);
        parent_path[parent_length] = '\0';
    }

    *parent_fd = open_directory_path(parent_path);
    return *parent_fd < 0 ? -1 : 0;
}

static int same_snapshot(const struct stat *before, const struct stat *after)
{
    return before->st_dev == after->st_dev &&
           before->st_ino == after->st_ino &&
           before->st_size == after->st_size &&
           before->st_mtim.tv_sec == after->st_mtim.tv_sec &&
           before->st_mtim.tv_nsec == after->st_mtim.tv_nsec &&
           before->st_ctim.tv_sec == after->st_ctim.tv_sec &&
           before->st_ctim.tv_nsec == after->st_ctim.tv_nsec;
}

static int replay_journal(int fd, Fcj3Replay *replay, size_t *bytes_read)
{
    char *line = malloc(FCJ3_MAX_LINE_BYTES + 1U);
    unsigned char chunk[16384];
    size_t line_length = 0U;
    size_t total = 0U;
    int result = -1;

    if (!line)
        return -1;

    for (;;)
    {
        ssize_t count;
        do
        {
            count = read(fd, chunk, sizeof(chunk));
        } while (count < 0 && errno == EINTR);

        if (count < 0)
            goto done;
        if (count == 0)
            break;
        if ((size_t)count > FCJ3_REPORT_MAX_JOURNAL_BYTES - total)
        {
            errno = E2BIG;
            goto done;
        }
        total += (size_t)count;

        for (ssize_t i = 0; i < count; ++i)
        {
            if (line_length >= FCJ3_MAX_LINE_BYTES)
            {
                errno = E2BIG;
                goto done;
            }
            line[line_length++] = (char)chunk[i];
            if (chunk[i] == '\n')
            {
                if (fcj3_replay_add_line(replay, line, line_length) != 0)
                    goto done;
                line_length = 0U;
            }
        }
    }

    if (line_length != 0U)
    {
        errno = EBADMSG;
        goto done;
    }
    *bytes_read = total;
    result = 0;

done:
    {
        int saved_errno = errno;
        free(line);
        errno = saved_errno;
    }
    return result;
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
        case FCJ3_STATE_NONE:
        default: return "INVALID";
    }
}

static const char *entry_name(Fcj3EntryEvidence evidence)
{
    switch (evidence)
    {
        case FCJ3_EVIDENCE_ABSENT: return "ABSENT";
        case FCJ3_EVIDENCE_MATCHING_REGULAR: return "MATCHING_REGULAR";
        case FCJ3_EVIDENCE_MISMATCHING_REGULAR: return "MISMATCHING_REGULAR";
        case FCJ3_EVIDENCE_SYMLINK: return "SYMLINK";
        case FCJ3_EVIDENCE_SPECIAL: return "SPECIAL";
        case FCJ3_EVIDENCE_UNKNOWN_ERROR:
        default: return "UNKNOWN_ERROR";
    }
}

static const char *source_name(Fcj3SourceEvidence evidence)
{
    switch (evidence)
    {
        case FCJ3_SOURCE_ABSENT: return "ABSENT";
        case FCJ3_SOURCE_PRESENT: return "PRESENT_ADVISORY_ONLY";
        case FCJ3_SOURCE_UNKNOWN_ERROR:
        default: return "UNKNOWN_ERROR";
    }
}

static const char *directory_name(Fcj3DirectoryEvidence evidence)
{
    switch (evidence)
    {
        case FCJ3_DIRECTORY_IDENTITY_MATCH: return "IDENTITY_MATCH";
        case FCJ3_DIRECTORY_IDENTITY_MISMATCH: return "IDENTITY_MISMATCH";
        case FCJ3_DIRECTORY_UNKNOWN_ERROR:
        default: return "UNKNOWN_ERROR";
    }
}

static const char *class_name(Fcj3RecoveryClass classification)
{
    switch (classification)
    {
        case FCJ3_CLASS_NO_ACTION_TERMINAL: return "NO_ACTION_TERMINAL";
        case FCJ3_CLASS_PRESERVE_INCOMPLETE: return "PRESERVE_INCOMPLETE";
        case FCJ3_CLASS_PRESERVE_PUBLISHED: return "PRESERVE_PUBLISHED";
        case FCJ3_CLASS_MANUAL_REVIEW: return "MANUAL_REVIEW";
        case FCJ3_CLASS_INVALID_INPUT:
        default: return "INVALID_INPUT";
    }
}

static const char *reason_name(Fcj3RecoveryReason reason)
{
    switch (reason)
    {
        case FCJ3_REASON_NONE: return "NONE";
        case FCJ3_REASON_INVALID_ARGUMENT: return "INVALID_ARGUMENT";
        case FCJ3_REASON_INVALID_TRANSACTION: return "INVALID_TRANSACTION";
        case FCJ3_REASON_INVALID_EVIDENCE: return "INVALID_EVIDENCE";
        case FCJ3_REASON_DIRECTORY_IDENTITY_MISMATCH: return "DIRECTORY_IDENTITY_MISMATCH";
        case FCJ3_REASON_DIRECTORY_UNKNOWN: return "DIRECTORY_UNKNOWN";
        case FCJ3_REASON_SOURCE_UNKNOWN: return "SOURCE_UNKNOWN";
        case FCJ3_REASON_ENTRY_UNKNOWN: return "ENTRY_UNKNOWN";
        case FCJ3_REASON_ENTRY_TYPE_OR_IDENTITY_MISMATCH: return "ENTRY_TYPE_OR_IDENTITY_MISMATCH";
        case FCJ3_REASON_STAGE_AND_DESTINATION_BOTH_PRESENT: return "STAGE_AND_DESTINATION_BOTH_PRESENT";
        case FCJ3_REASON_SOURCE_MISSING: return "SOURCE_MISSING";
        case FCJ3_REASON_STATE_EVIDENCE_CONFLICT: return "STATE_EVIDENCE_CONFLICT";
        case FCJ3_REASON_EXPECTED_DESTINATION_MISSING: return "EXPECTED_DESTINATION_MISSING";
        case FCJ3_REASON_UNEXPECTED_VISIBLE_ENTRY: return "UNEXPECTED_VISIBLE_ENTRY";
        default: return "UNKNOWN_REASON";
    }
}

static void json_string(const char *value)
{
    const unsigned char *cursor = (const unsigned char *)value;
    putchar('"');
    while (*cursor)
    {
        unsigned char c = *cursor++;
        switch (c)
        {
            case '"': fputs("\\\"", stdout); break;
            case '\\': fputs("\\\\", stdout); break;
            case '\b': fputs("\\b", stdout); break;
            case '\f': fputs("\\f", stdout); break;
            case '\n': fputs("\\n", stdout); break;
            case '\r': fputs("\\r", stdout); break;
            case '\t': fputs("\\t", stdout); break;
            default:
                if (c < 0x20U || c >= 0x7fU)
                    (void)printf("\\u%04x", (unsigned int)c);
                else
                    putchar((int)c);
                break;
        }
    }
    putchar('"');
}

static int collect_transaction(const Fcj3Transaction *transaction,
                               ReportTransaction *report)
{
    int source_fd = -1;
    int destination_fd = -1;
    char source_basename[NAME_MAX + 1U];
    char destination_basename[NAME_MAX + 1U];
    int source_result;
    int destination_result;

    memset(report, 0, sizeof(*report));
    report->filesystem.recovery.transaction = transaction;

    source_result = resolve_parent(transaction->source, &source_fd,
                                   source_basename);
    if (source_result != 0)
        report->source_resolver_errno = errno;

    destination_result = resolve_parent(transaction->destination,
                                        &destination_fd,
                                        destination_basename);
    if (destination_result != 0)
        report->destination_resolver_errno = errno;

    if (source_result == 0 && destination_result == 0)
    {
        if (fcj3_recovery_collect_at(transaction, source_fd,
                                     destination_fd,
                                     &report->filesystem) != 0)
        {
            int saved_errno = errno;
            report->filesystem.recovery.directory =
                FCJ3_DIRECTORY_UNKNOWN_ERROR;
            report->filesystem.recovery.stage =
                FCJ3_EVIDENCE_UNKNOWN_ERROR;
            report->filesystem.recovery.destination =
                FCJ3_EVIDENCE_UNKNOWN_ERROR;
            report->filesystem.recovery.source =
                FCJ3_SOURCE_UNKNOWN_ERROR;
            report->filesystem.directory_errno = saved_errno;
            report->filesystem.source_errno = saved_errno;
        }
    }
    else
    {
        report->filesystem.recovery.directory =
            destination_result == 0
                ? FCJ3_DIRECTORY_UNKNOWN_ERROR
                : FCJ3_DIRECTORY_UNKNOWN_ERROR;
        report->filesystem.recovery.stage = FCJ3_EVIDENCE_UNKNOWN_ERROR;
        report->filesystem.recovery.destination = FCJ3_EVIDENCE_UNKNOWN_ERROR;
        report->filesystem.recovery.source = FCJ3_SOURCE_UNKNOWN_ERROR;
        if (report->source_resolver_errno != 0)
            report->filesystem.source_errno = report->source_resolver_errno;
        if (report->destination_resolver_errno != 0)
            report->filesystem.directory_errno =
                report->destination_resolver_errno;
        report->filesystem.stage_errno = report->destination_resolver_errno;
        report->filesystem.destination_errno =
            report->destination_resolver_errno;
    }

    if (source_fd >= 0 && close(source_fd) != 0 &&
        report->source_resolver_errno == 0)
        report->source_resolver_errno = errno;
    if (destination_fd >= 0 && close(destination_fd) != 0 &&
        report->destination_resolver_errno == 0)
        report->destination_resolver_errno = errno;

    return fcj3_recovery_classify(&report->filesystem.recovery,
                                  &report->decision);
}

static void print_transaction(const Fcj3Transaction *tx,
                              const ReportTransaction *report)
{
    const Fcj3FilesystemEvidence *fs = &report->filesystem;
    printf("{\"operation_id\":");
    json_string(tx->operation_id);
    printf(",\"sequence\":%" PRIu64 ",\"state\":", tx->sequence);
    json_string(state_name(tx->state));
    printf(",\"error_code\":%d,\"source\":", tx->error_code);
    json_string(tx->source);
    printf(",\"destination\":");
    json_string(tx->destination);
    printf(",\"staging_component\":");
    json_string(tx->staging_component);
    printf(",\"directory_evidence\":");
    json_string(directory_name(fs->recovery.directory));
    printf(",\"stage_evidence\":");
    json_string(entry_name(fs->recovery.stage));
    printf(",\"destination_evidence\":");
    json_string(entry_name(fs->recovery.destination));
    printf(",\"source_evidence\":");
    json_string(source_name(fs->recovery.source));
    printf(",\"source_mode\":%u", (unsigned int)fs->source_mode);
    printf(",\"errno\":{\"source\":%d,\"directory\":%d,\"stage\":%d,\"destination\":%d,\"source_resolver\":%d,\"destination_resolver\":%d}",
           fs->source_errno, fs->directory_errno, fs->stage_errno,
           fs->destination_errno, report->source_resolver_errno,
           report->destination_resolver_errno);
    printf(",\"classification\":");
    json_string(class_name(report->decision.classification));
    printf(",\"reason\":");
    json_string(reason_name(report->decision.reason));
    putchar('}');
}

static int report_journal(const char *journal_path)
{
    int parent_fd = -1;
    int journal_fd = -1;
    char basename[NAME_MAX + 1U];
    struct stat before;
    struct stat after;
    Fcj3Replay replay;
    ReportTransaction *reports = NULL;
    size_t bytes_read = 0U;
    int exit_status = EXIT_FAILURE;

    fcj3_replay_init(&replay);

    if (resolve_parent(journal_path, &parent_fd, basename) != 0)
    {
        fprintf(stderr, "cannot resolve journal parent: %s\n", strerror(errno));
        goto done;
    }

    do
    {
        journal_fd = openat(parent_fd, basename,
                            O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    } while (journal_fd < 0 && errno == EINTR);
    if (journal_fd < 0)
    {
        fprintf(stderr, "cannot open journal without following symlinks: %s\n",
                strerror(errno));
        goto done;
    }

    if (fstat(journal_fd, &before) != 0)
    {
        fprintf(stderr, "cannot stat journal: %s\n", strerror(errno));
        goto done;
    }
    if (!S_ISREG(before.st_mode))
    {
        fprintf(stderr, "journal is not a regular file\n");
        goto done;
    }
    if (before.st_size < 0 ||
        (uintmax_t)before.st_size > FCJ3_REPORT_MAX_JOURNAL_BYTES)
    {
        fprintf(stderr, "journal exceeds the 64 MiB report limit\n");
        goto done;
    }

    if (replay_journal(journal_fd, &replay, &bytes_read) != 0)
    {
        fprintf(stderr, "journal replay rejected: %s\n", strerror(errno));
        goto done;
    }

    if (fstat(journal_fd, &after) != 0)
    {
        fprintf(stderr, "cannot re-stat journal: %s\n", strerror(errno));
        goto done;
    }
    if (!same_snapshot(&before, &after) ||
        (uintmax_t)after.st_size != (uintmax_t)bytes_read)
    {
        fprintf(stderr, "journal changed during read; refusing report\n");
        goto done;
    }

    if (replay.count > SIZE_MAX / sizeof(*reports))
    {
        fprintf(stderr, "transaction report allocation overflow\n");
        goto done;
    }
    reports = calloc(replay.count ? replay.count : 1U, sizeof(*reports));
    if (!reports)
    {
        fprintf(stderr, "cannot allocate transaction report: %s\n",
                strerror(errno));
        goto done;
    }

    for (size_t i = 0U; i < replay.count; ++i)
    {
        if (collect_transaction(&replay.transactions[i], &reports[i]) != 0)
        {
            fprintf(stderr, "classifier rejected replay transaction %zu\n", i);
            goto done;
        }
    }

    printf("{\"schema_version\":1,\"read_only\":true,\"changes_made\":false,");
    printf("\"journal\":{\"path\":");
    json_string(journal_path);
    printf(",\"device\":%" PRIuMAX ",\"inode\":%" PRIuMAX
           ",\"size_bytes\":%zu,\"stability_check\":\"passed\","
           "\"snapshot_atomic\":false},",
           (uintmax_t)before.st_dev, (uintmax_t)before.st_ino, bytes_read);
    printf("\"transaction_count\":%zu,\"transactions\":[", replay.count);
    for (size_t i = 0U; i < replay.count; ++i)
    {
        if (i != 0U)
            putchar(',');
        print_transaction(&replay.transactions[i], &reports[i]);
    }
    puts("]}");
    if (ferror(stdout))
    {
        fprintf(stderr, "failed to write complete JSON report\n");
        goto done;
    }
    exit_status = EXIT_SUCCESS;

done:
    {
        int saved_errno = errno;
        free(reports);
        fcj3_replay_destroy(&replay);
        if (journal_fd >= 0)
            (void)close(journal_fd);
        if (parent_fd >= 0)
            (void)close(parent_fd);
        errno = saved_errno;
    }
    return exit_status;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s /absolute/path/to/fcj3-journal\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argv[1][0] != '/')
    {
        fprintf(stderr, "journal path must be absolute\n");
        return EXIT_FAILURE;
    }
    return report_journal(argv[1]);
}
