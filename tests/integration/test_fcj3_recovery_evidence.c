#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "journal/fcj3_recovery_evidence.h"

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CHECK(expression) do { \
    if (!(expression)) { \
        int saved_errno = errno; \
        fprintf(stderr, "CHECK failed at %s:%d: %s (errno=%d)\\n", \
                __FILE__, __LINE__, #expression, saved_errno); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static void create_file_at(int directory_fd, const char *name)
{
    int fd = openat(directory_fd, name,
                    O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
                    0600);
    CHECK(fd >= 0);
    CHECK(write(fd, "x", 1U) == 1);
    CHECK(close(fd) == 0);
}

static Fcj3Transaction make_transaction(const char *root,
                                        const struct stat *directory,
                                        const struct stat *stage)
{
    Fcj3Transaction transaction;
    memset(&transaction, 0, sizeof(transaction));
    (void)snprintf(transaction.operation_id, sizeof(transaction.operation_id),
                   "%s", "0123456789abcdef0123456789abcdef");
    (void)snprintf(transaction.source, sizeof(transaction.source),
                   "%s/source.bin", root);
    (void)snprintf(transaction.destination, sizeof(transaction.destination),
                   "%s/destination.bin", root);
    (void)snprintf(transaction.staging_component,
                   sizeof(transaction.staging_component), "%s",
                   ".fc-tmp-evidence");
    transaction.destination_dir_dev = directory->st_dev;
    transaction.destination_dir_ino = directory->st_ino;
    transaction.staging_dev = stage->st_dev;
    transaction.staging_ino = stage->st_ino;
    transaction.state = FCJ3_STATE_STARTED;
    return transaction;
}

static Fcj3FilesystemEvidence collect(const Fcj3Transaction *transaction,
                                      int source_fd, int destination_fd)
{
    Fcj3FilesystemEvidence evidence;
    CHECK(fcj3_recovery_collect_at(transaction, source_fd, destination_fd,
                                    &evidence) == 0);
    return evidence;
}

static void test_regular_identity_and_missing_entries(int root_fd,
                                                        const char *root,
                                                        const struct stat *dir,
                                                        const struct stat *stage)
{
    Fcj3Transaction transaction = make_transaction(root, dir, stage);
    Fcj3FilesystemEvidence evidence =
        collect(&transaction, root_fd, root_fd);

    CHECK(evidence.recovery.directory == FCJ3_DIRECTORY_IDENTITY_MATCH);
    CHECK(evidence.recovery.source == FCJ3_SOURCE_PRESENT);
    CHECK(evidence.recovery.stage == FCJ3_EVIDENCE_MATCHING_REGULAR);
    CHECK(evidence.recovery.destination == FCJ3_EVIDENCE_ABSENT);
    CHECK(evidence.source_errno == 0);
    CHECK(evidence.stage_errno == ENOENT);
    CHECK(evidence.destination_errno == ENOENT);
}

static void test_mismatch_symlink_and_special(int root_fd, const char *root,
                                               const struct stat *dir,
                                               const struct stat *stage)
{
    Fcj3Transaction transaction = make_transaction(root, dir, stage);
    create_file_at(root_fd, "destination.bin");
    Fcj3FilesystemEvidence evidence =
        collect(&transaction, root_fd, root_fd);
    CHECK(evidence.recovery.destination == FCJ3_EVIDENCE_MISMATCHING_REGULAR);
    CHECK(unlinkat(root_fd, "destination.bin", 0) == 0);

    CHECK(symlinkat("source.bin", root_fd, "destination.bin") == 0);
    evidence = collect(&transaction, root_fd, root_fd);
    CHECK(evidence.recovery.destination == FCJ3_EVIDENCE_SYMLINK);
    CHECK(unlinkat(root_fd, "destination.bin", 0) == 0);

    CHECK(mkfifoat(root_fd, "destination.bin", 0600) == 0);
    evidence = collect(&transaction, root_fd, root_fd);
    CHECK(evidence.recovery.destination == FCJ3_EVIDENCE_SPECIAL);
    CHECK(unlinkat(root_fd, "destination.bin", 0) == 0);

    CHECK(unlinkat(root_fd, ".fc-tmp-evidence", 0) == 0);
    CHECK(symlinkat("source.bin", root_fd, ".fc-tmp-evidence") == 0);
    evidence = collect(&transaction, root_fd, root_fd);
    CHECK(evidence.recovery.stage == FCJ3_EVIDENCE_SYMLINK);
    CHECK(unlinkat(root_fd, ".fc-tmp-evidence", 0) == 0);
}

static void test_directory_identity_gate(int root_fd, const char *root,
                                         const struct stat *dir,
                                         const struct stat *stage)
{
    char other_template[] = "/tmp/fcj3-evidence-other-XXXXXX";
    char *other = mkdtemp(other_template);
    CHECK(other != NULL);
    int other_fd = open(other, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    CHECK(other_fd >= 0);

    Fcj3Transaction transaction = make_transaction(root, dir, stage);
    Fcj3FilesystemEvidence evidence =
        collect(&transaction, root_fd, other_fd);
    CHECK(evidence.recovery.directory == FCJ3_DIRECTORY_IDENTITY_MISMATCH);
    CHECK(evidence.recovery.stage == FCJ3_EVIDENCE_UNKNOWN_ERROR);
    CHECK(evidence.recovery.destination == FCJ3_EVIDENCE_UNKNOWN_ERROR);
    CHECK(close(other_fd) == 0);
    CHECK(rmdir(other) == 0);
}

static void test_unsafe_paths_are_rejected(int root_fd, const char *root,
                                           const struct stat *dir,
                                           const struct stat *stage)
{
    Fcj3Transaction transaction = make_transaction(root, dir, stage);
    Fcj3FilesystemEvidence evidence;
    (void)snprintf(transaction.destination, sizeof(transaction.destination),
                   "%s/../outside", root);
    errno = 0;
    CHECK(fcj3_recovery_collect_at(&transaction, root_fd, root_fd,
                                    &evidence) == -1);
    CHECK(errno == EINVAL);
}

int main(void)
{
    char root_template[] = "/tmp/fcj3-evidence-XXXXXX";
    char *root = mkdtemp(root_template);
    CHECK(root != NULL);
    int root_fd = open(root, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    CHECK(root_fd >= 0);

    create_file_at(root_fd, "source.bin");
    create_file_at(root_fd, ".fc-tmp-evidence");

    struct stat directory_metadata;
    struct stat stage_metadata;
    CHECK(fstat(root_fd, &directory_metadata) == 0);
    CHECK(fstatat(root_fd, ".fc-tmp-evidence", &stage_metadata,
                   AT_SYMLINK_NOFOLLOW) == 0);

    test_regular_identity_and_missing_entries(root_fd, root,
                                               &directory_metadata,
                                               &stage_metadata);
    test_mismatch_symlink_and_special(root_fd, root, &directory_metadata,
                                      &stage_metadata);
    test_directory_identity_gate(root_fd, root, &directory_metadata,
                                 &stage_metadata);
    test_unsafe_paths_are_rejected(root_fd, root, &directory_metadata,
                                   &stage_metadata);

    CHECK(unlinkat(root_fd, ".fc-tmp-evidence", 0) == 0);
    CHECK(unlinkat(root_fd, "source.bin", 0) == 0);
    CHECK(close(root_fd) == 0);
    CHECK(rmdir(root) == 0);
    puts("FCJ3 read-only evidence collector tests passed");
    return EXIT_SUCCESS;
}
