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

    /* The journal itself must not be a symlink. */
    char journal_link[PATH_MAX];
    make_path(journal_link, root, "journal-link");
    assert(symlink(journal_path, journal_link) == 0);
    errno = 0;
    assert(journal_open(&journal, journal_link) == -1);
    assert(errno == ELOOP);

    unlink(journal_link);
    unlink(malformed_path);
    unlink(source);
    unlink(destination);
    unlink(source_only);
    unlink(ambiguous_destination);
    unlink(journal_path);
    rmdir(root);

    puts("journal recovery security tests passed");
    return 0;
}
