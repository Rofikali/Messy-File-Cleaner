#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "journal/journal.h"

static void make_path(char output[PATH_MAX], const char *root, const char *name)
{
    int n = snprintf(output, PATH_MAX, "%s/%s", root, name);
    assert(n > 0 && n < PATH_MAX);
}

static void create_file(const char *path, const char *value)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    assert(fd >= 0);
    size_t length = strlen(value);
    size_t offset = 0;
    while (offset < length) {
        ssize_t n = write(fd, value + offset, length - offset);
        assert(n > 0);
        offset += (size_t)n;
    }
    assert(fsync(fd) == 0);
    assert(close(fd) == 0);
}

static int file_equals(const char *path, const char *expected)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
        return 0;
    size_t expected_length = strlen(expected);
    char buffer[PATH_MAX];
    size_t actual_length = fread(buffer, 1, sizeof(buffer), fp);
    int extra = fgetc(fp);
    int close_status = fclose(fp);
    return close_status == 0 && actual_length == expected_length &&
           extra == EOF && memcmp(buffer, expected, expected_length) == 0;
}

static size_t count_lines(const char *path)
{
    FILE *fp = fopen(path, "r");
    assert(fp != NULL);
    size_t count = 0;
    int ch;
    while ((ch = fgetc(fp)) != EOF)
        if (ch == '\n')
            count++;
    assert(fclose(fp) == 0);
    return count;
}

int main(void)
{
    char root_template[] = "/tmp/fc-journal-XXXXXX";
    char *root = mkdtemp(root_template);
    assert(root != NULL);

    char journal_path[PATH_MAX];
    make_path(journal_path, root, "move.log");

    /* v2 hex encoding safely round-trips delimiter and newline characters. */
    char source[PATH_MAX], destination[PATH_MAX];
    make_path(source, root, "source|with\nnewline.txt");
    make_path(destination, root, "destination|with\nnewline.txt");
    create_file(source, "source bytes");
    create_file(destination, "published bytes");

    MoveJournal journal;
    assert(journal_open(&journal, journal_path) == 0);
    MoveJournal second_writer;
    errno = 0;
    assert(journal_open(&second_writer, journal_path) == -1);
    assert(errno == EWOULDBLOCK || errno == EAGAIN);
    assert(journal_record_started(&journal, source, destination) == 0);
    assert(journal_record_published(&journal, source, destination) == 0);
    assert(journal_close(&journal) == 0);

    /* Crash after publish but before source retirement: preserve both copies. */
    assert(journal_recover(journal_path) == 0);
    assert(access(source, F_OK) == 0);
    assert(access(destination, F_OK) == 0);
    assert(count_lines(journal_path) == 3);

    /* Recovery is idempotent: the RECOVERED state is not appended again. */
    assert(journal_recover(journal_path) == 0);
    assert(count_lines(journal_path) == 3);

    /* An unfinished STARTED operation with only its source is a safe no-op. */
    char source_only[PATH_MAX], missing_destination[PATH_MAX];
    make_path(source_only, root, "source-only.txt");
    make_path(missing_destination, root, "not-created.txt");
    create_file(source_only, "keep me");
    assert(journal_open(&journal, journal_path) == 0);
    assert(journal_record_started(&journal, source_only, missing_destination) == 0);
    assert(journal_close(&journal) == 0);
    assert(journal_recover(journal_path) == 0);
    assert(access(source_only, F_OK) == 0);
    assert(access(missing_destination, F_OK) != 0 && errno == ENOENT);

    /* STARTED + missing source + existing destination is ambiguous: fail closed. */
    char absent_source[PATH_MAX], ambiguous_destination[PATH_MAX];
    make_path(absent_source, root, "absent-source.txt");
    make_path(ambiguous_destination, root, "ambiguous-destination.txt");
    create_file(ambiguous_destination, "must not be claimed");
    assert(journal_open(&journal, journal_path) == 0);
    assert(journal_record_started(&journal, absent_source, ambiguous_destination) == 0);
    assert(journal_close(&journal) == 0);
    size_t before_ambiguous = count_lines(journal_path);
    errno = 0;
    assert(journal_recover(journal_path) == -1);
    assert(errno == EUCLEAN);
    assert(count_lines(journal_path) == before_ambiguous);
    assert(access(ambiguous_destination, F_OK) == 0);

    /* A truncated/malformed record is not silently ignored or auto-repaired. */
    char malformed_path[PATH_MAX];
    make_path(malformed_path, root, "malformed.log");
    create_file(malformed_path, "broken-record-without-newline");
    errno = 0;
    assert(journal_recover(malformed_path) == -1);
    assert(access(malformed_path, F_OK) == 0);

    /* Descriptor-relative traversal creates nested parents and rejects
     * symlink ancestors without touching the symlink target. */
    char nested_path[PATH_MAX], nested_parent[PATH_MAX], real_parent[PATH_MAX];
    make_path(nested_parent, root, "nested/child");
    make_path(nested_path, root, "nested/child/journal.log");
    assert(journal_open(&journal, nested_path) == 0);
    assert(journal_close(&journal) == 0);
    make_path(real_parent, root, "real-parent");
    assert(mkdir(real_parent, 0700) == 0);
    char link_parent[PATH_MAX], linked_journal[PATH_MAX];
    make_path(link_parent, root, "linked-parent");
    make_path(linked_journal, root, "linked-parent/journal.log");
    assert(symlink(real_parent, link_parent) == 0);
    errno = 0;
    assert(journal_open(&journal, linked_journal) == -1);
    assert(errno == ELOOP || errno == ENOTDIR);
    char untouched[PATH_MAX];
    make_path(untouched, real_parent, "journal.log");
    assert(access(untouched, F_OK) != 0 && errno == ENOENT);

    /* Recovery of an absent journal must not create parent directories. */
    char absent_parent_journal[PATH_MAX], absent_parent[PATH_MAX];
    make_path(absent_parent_journal, root, "absent-parent/child/journal.log");
    make_path(absent_parent, root, "absent-parent");
    assert(journal_recover(absent_parent_journal) == 0);
    assert(access(absent_parent, F_OK) != 0 && errno == ENOENT);

    /* The journal itself must not be a symlink. */
    char journal_link[PATH_MAX];
    make_path(journal_link, root, "journal-link");
    assert(symlink(journal_path, journal_link) == 0);
    errno = 0;
    assert(journal_open(&journal, journal_link) == -1);
    assert(errno == ELOOP);

    /* Journal append failures never authorize a move or overwrite. */
    char fail_before_path[PATH_MAX], partial_path[PATH_MAX], sync_fail_path[PATH_MAX];
    make_path(fail_before_path, root, "fail-before.log");
    make_path(partial_path, root, "partial-tail.log");
    make_path(sync_fail_path, root, "sync-fail.log");

    char protected_source[PATH_MAX], protected_destination[PATH_MAX];
    make_path(protected_source, root, "protected-source.txt");
    make_path(protected_destination, root, "protected-destination.txt");
    create_file(protected_source, "source must survive");
    create_file(protected_destination, "destination must not be overwritten");

    assert(journal_open(&journal, fail_before_path) == 0);
    fc_test_journal_set_write_failure(FC_TEST_JOURNAL_WRITE_FAIL_BEFORE, EIO, 0);
    errno = 0;
    assert(journal_record_started(&journal, protected_source, protected_destination) == -1);
    assert(errno == EIO);
    assert(journal_close(&journal) == 0);
    assert(journal_recover(fail_before_path) == 0);
    assert(access(protected_source, F_OK) == 0);
    assert(access(protected_destination, F_OK) == 0);
    assert(file_equals(protected_source, "source must survive"));
    assert(file_equals(protected_destination, "destination must not be overwritten"));

    assert(journal_open(&journal, partial_path) == 0);
    fc_test_journal_set_write_failure(FC_TEST_JOURNAL_WRITE_FAIL_PARTIAL, ENOSPC, 12);
    errno = 0;
    assert(journal_record_started(&journal, protected_source, protected_destination) == -1);
    assert(errno == ENOSPC);
    assert(journal_close(&journal) == 0);
    errno = 0;
    assert(journal_recover(partial_path) == -1);
    assert(errno == EBADMSG || errno == EOVERFLOW);
    assert(access(protected_source, F_OK) == 0);
    assert(access(protected_destination, F_OK) == 0);
    assert(file_equals(protected_source, "source must survive"));
    assert(file_equals(protected_destination, "destination must not be overwritten"));

    assert(journal_open(&journal, sync_fail_path) == 0);
    fc_test_journal_set_fsync_error(EIO);
    errno = 0;
    assert(journal_record_started(&journal, protected_source, protected_destination) == -1);
    assert(errno == EIO);
    assert(journal_close(&journal) == 0);
    /* The complete record may be visible despite uncertain durability.
       Recovery is conservative and leaves both user paths untouched. */
    assert(journal_recover(sync_fail_path) == 0);
    assert(access(protected_source, F_OK) == 0);
    assert(access(protected_destination, F_OK) == 0);
    assert(file_equals(protected_source, "source must survive"));
    assert(file_equals(protected_destination, "destination must not be overwritten"));



    /* State-specific failures: the last durable state governs recovery. */
    char published_fail_log[PATH_MAX], failed_fail_log[PATH_MAX];
    char completed_fail_log[PATH_MAX], recovered_fail_log[PATH_MAX];
    char transition_source[PATH_MAX], transition_dest[PATH_MAX];
    make_path(published_fail_log, root, "published-fail.log");
    make_path(failed_fail_log, root, "failed-fail.log");
    make_path(completed_fail_log, root, "completed-fail.log");
    make_path(recovered_fail_log, root, "recovered-fail.log");
    make_path(transition_source, root, "transition-source.txt");
    make_path(transition_dest, root, "transition-destination.txt");

    create_file(transition_source, "transition source");
    create_file(transition_dest, "existing published destination");
    assert(journal_open(&journal, published_fail_log) == 0);
    assert(journal_record_started(&journal, transition_source, transition_dest) == 0);
    fc_test_journal_set_state_write_failure("PUBLISHED",
        FC_TEST_JOURNAL_WRITE_FAIL_BEFORE, EIO, 0);
    errno = 0;
    assert(journal_record_published(&journal, transition_source, transition_dest) == -1);
    assert(errno == EIO);
    assert(journal_close(&journal) == 0);
    assert(journal_recover(published_fail_log) == 0);
    assert(file_equals(transition_source, "transition source"));
    assert(file_equals(transition_dest, "existing published destination"));

    assert(journal_open(&journal, failed_fail_log) == 0);
    assert(journal_record_started(&journal, transition_source, transition_dest) == 0);
    fc_test_journal_set_state_write_failure("FAILED",
        FC_TEST_JOURNAL_WRITE_FAIL_BEFORE, EIO, 0);
    errno = 0;
    assert(journal_record_failed(&journal, transition_source, transition_dest, EIO) == -1);
    assert(errno == EIO);
    assert(journal_close(&journal) == 0);
    assert(journal_recover(failed_fail_log) == 0);
    assert(file_equals(transition_source, "transition source"));
    assert(file_equals(transition_dest, "existing published destination"));

    /* A completed filesystem operation with a failed COMPLETED append leaves
       the durable PUBLISHED record; recovery records a conservative outcome. */
    assert(journal_open(&journal, completed_fail_log) == 0);
    assert(journal_record_started(&journal, transition_source, transition_dest) == 0);
    assert(journal_record_published(&journal, transition_source, transition_dest) == 0);
    fc_test_journal_set_state_write_failure("COMPLETED",
        FC_TEST_JOURNAL_WRITE_FAIL_BEFORE, EIO, 0);
    errno = 0;
    assert(journal_record_completed(&journal, transition_source, transition_dest) == -1);
    assert(errno == EIO);
    assert(journal_close(&journal) == 0);
    assert(unlink(transition_source) == 0);
    assert(journal_recover(completed_fail_log) == 0);
    assert(file_equals(transition_dest, "existing published destination"));

    /* Failure to append RECOVERED is retryable and never mutates user paths. */
    assert(journal_open(&journal, recovered_fail_log) == 0);
    assert(journal_record_started(&journal, transition_source, transition_dest) == 0);
    assert(journal_close(&journal) == 0);
    fc_test_journal_set_state_write_failure("RECOVERED",
        FC_TEST_JOURNAL_WRITE_FAIL_BEFORE, EIO, 0);
    errno = 0;
    assert(journal_recover(recovered_fail_log) == -1);
    assert(errno == EIO);
    assert(file_equals(transition_dest, "existing published destination"));
    assert(journal_recover(recovered_fail_log) == 0);
    assert(file_equals(transition_dest, "existing published destination"));

    unlink(protected_source);
    unlink(protected_destination);
    unlink(fail_before_path);
    unlink(partial_path);
    unlink(sync_fail_path);
    unlink(published_fail_log);
    unlink(failed_fail_log);
    unlink(completed_fail_log);
    unlink(recovered_fail_log);
    unlink(transition_source);
    unlink(transition_dest);

    unlink(journal_link);
    unlink(link_parent);
    unlink(nested_path);
    rmdir(nested_parent);
    char nested_root[PATH_MAX];
    make_path(nested_root, root, "nested");
    rmdir(nested_root);
    unlink(malformed_path);
    unlink(source);
    unlink(destination);
    unlink(source_only);
    unlink(ambiguous_destination);
    unlink(journal_path);
    rmdir(real_parent);
    rmdir(root);

    puts("journal recovery security tests passed");
    return 0;
}
