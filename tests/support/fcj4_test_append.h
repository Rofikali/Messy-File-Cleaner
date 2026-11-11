#ifndef FCJ4_TEST_APPEND_H
#define FCJ4_TEST_APPEND_H

/*
 * Private Linux-only FCJ4 append adapter for disposable test fixtures.
 * This header is under tests/support and must never enter a production target.
 */
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct Fcj4TestAppend Fcj4TestAppend;

typedef struct {
    ssize_t (*pwrite_fn)(void *user, int fd, const void *buffer,
                         size_t length, off_t offset);
    int (*fsync_fn)(void *user, int fd);
    int (*close_fn)(void *user, int fd);
    int (*flock_fn)(void *user, int fd, int operation);
    int (*fstat_fn)(void *user, int fd, struct stat *out);
    int (*getfl_fn)(void *user, int fd);
    int (*getfd_fn)(void *user, int fd);
    void *user;
    void *(*calloc_fn)(void *allocator_user, size_t count, size_t size);
    void (*free_fn)(void *allocator_user, void *pointer);
    void *allocator_user;
} Fcj4TestAppendOps;

typedef enum {
    FCJ4_TEST_APPEND_REJECTED = 0,
    FCJ4_TEST_APPEND_DURABLE,
    FCJ4_TEST_APPEND_RECONCILIATION_REQUIRED,
    FCJ4_TEST_APPEND_UNCERTAIN_WRITE,
    FCJ4_TEST_APPEND_UNCERTAIN_FSYNC,
    FCJ4_TEST_APPEND_CLEANUP_ERROR
} Fcj4TestAppendResultCode;

typedef struct {
    Fcj4TestAppendResultCode code;
    int primary_errno;
    int cleanup_errno;
    uint64_t durable_length;
} Fcj4TestAppendResult;

/*
 * On success, ownership of fd transfers to the context. On failure, fd
 * remains caller-owned and open. If initialization acquired flock before a
 * later validation failure, it attempts exactly one unlock and preserves the
 * original initialization errno. The descriptor must be a private regular
 * file, O_CLOEXEC, not O_APPEND, with the supplied device/inode/size.
 *
 * ops == NULL selects real Linux syscalls and the real calloc/free pair.
 * A supplied table must provide every syscall callback. Its allocator callbacks
 * must be both present or both absent; absent callbacks select calloc/free.
 * Keep syscall user and allocator_user independent. All callbacks and their
 * user data must remain valid and immutable for the context lifetime. The
 * allocator callbacks run only during init/destroy; free must not fail.
 * Do not share mutable fault state globally. The context is single-owner;
 * concurrent/reentrant append calls are rejected using an atomic try-guard.
 */
int fcj4_test_append_init(Fcj4TestAppend **out, int fd,
                          dev_t expected_dev, ino_t expected_ino,
                          uint64_t expected_length,
                          const Fcj4TestAppendOps *ops);

/*
 * Append one exact serializer-prepared FCJ4 line at expected_offset. The
 * adapter checks framing/ASCII only; the serializer is the canonicality
 * authority. A successful fsync() returns DURABLE and blocks the next append
 * until ack_commit().
 * Uncertain write/fsync outcomes poison the context. No retry of a complete
 * record, truncation, repair, or valid-prefix acceptance is performed.
 */
int fcj4_test_append_record(Fcj4TestAppend *ctx, const char *bytes,
                            size_t length, uint64_t expected_offset,
                            Fcj4TestAppendResult *result);

/* Acknowledge only after the caller commits these exact bytes to writer state. */
int fcj4_test_append_ack_commit(Fcj4TestAppend *ctx, const char *bytes,
                                size_t length);

/* Poison a durable-pending context if the caller's writer commit fails. */
void fcj4_test_append_poison(Fcj4TestAppend *ctx);

/*
 * Unlock once and close once. Never retry close after an error. A durable
 * fsync result is not undone by later cleanup errors. Destroying a durable-
 * pending context without acknowledgement returns EIO and requires replay.
 * Context is freed even when cleanup reports an error.
 */
int fcj4_test_append_destroy(Fcj4TestAppend *ctx,
                             Fcj4TestAppendResult *result);

#endif
