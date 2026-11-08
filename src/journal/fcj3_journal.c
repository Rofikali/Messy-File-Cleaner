#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "journal/fcj3_journal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef FC_TESTING
static _Atomic size_t test_write_fail_after = SIZE_MAX;
static _Atomic size_t test_write_accepted;
static atomic_int test_write_error;
static atomic_int test_fsync_error;

void fcj3_journal_test_fail_write_after(size_t after_bytes, int error_code)
{
    atomic_store_explicit(&test_write_accepted, 0U, memory_order_release);
    atomic_store_explicit(&test_write_error, error_code, memory_order_release);
    atomic_store_explicit(&test_write_fail_after, after_bytes,
                          memory_order_release);
}

void fcj3_journal_test_fail_fsync(int error_code)
{
    atomic_store_explicit(&test_fsync_error, error_code, memory_order_release);
}
#endif

static int valid_component(const char *component)
{
    if (component == NULL || component[0] == '\0' ||
        strcmp(component, ".") == 0 || strcmp(component, "..") == 0)
        return 0;

    size_t length = strnlen(component, NAME_MAX);
    if (length == 0U || length >= NAME_MAX)
        return 0;

    for (size_t i = 0U; i < length; ++i)
    {
        unsigned char byte = (unsigned char)component[i];
        if (byte == '/' || byte < 0x20U || byte == 0x7fU)
            return 0;
    }
    return 1;
}

static int validate_directory(int directory_fd, uid_t expected_owner)
{
    struct stat metadata;
    if (fstat(directory_fd, &metadata) != 0)
        return -1;
    if (!S_ISDIR(metadata.st_mode) || metadata.st_uid != expected_owner ||
        (metadata.st_mode & 0022) != 0)
    {
        errno = EPERM;
        return -1;
    }
    return 0;
}

static int validate_journal_file(int fd, uid_t expected_owner)
{
    struct stat metadata;
    if (fstat(fd, &metadata) != 0)
        return -1;
    if (!S_ISREG(metadata.st_mode) || metadata.st_uid != expected_owner ||
        metadata.st_nlink != 1 || (metadata.st_mode & 0777) != 0600)
    {
        errno = EPERM;
        return -1;
    }
    return 0;
}

static int acquire_exclusive_lock(int fd)
{
    for (;;)
    {
        if (flock(fd, LOCK_EX | LOCK_NB) == 0)
            return 0;
        if (errno != EINTR)
            return -1;
    }
}

int fcj3_journal_open_at(Fcj3Journal *journal, int directory_fd,
                        const char *component, uid_t expected_owner)
{
    int fd = -1;
    int created = 0;
    int mutex_initialized = 0;

    if (journal == NULL || directory_fd < 0 || !valid_component(component))
    {
        errno = EINVAL;
        return -1;
    }

    memset(journal, 0, sizeof(*journal));
    journal->fd = -1;

    if (validate_directory(directory_fd, expected_owner) != 0)
        return -1;

    fd = openat(directory_fd, component,
                O_WRONLY | O_APPEND | O_CLOEXEC | O_NOFOLLOW |
                    O_NONBLOCK | O_CREAT | O_EXCL,
                0600);
    if (fd >= 0)
    {
        created = 1;
    }
    else if (errno == EEXIST)
    {
        fd = openat(directory_fd, component,
                    O_WRONLY | O_APPEND | O_CLOEXEC | O_NOFOLLOW |
                        O_NONBLOCK);
    }

    if (fd < 0)
        return -1;

    if (validate_journal_file(fd, expected_owner) != 0)
        goto fail;

    if (acquire_exclusive_lock(fd) != 0)
        goto fail;

    /* Revalidate after obtaining the lifetime lock. */
    if (validate_journal_file(fd, expected_owner) != 0)
        goto fail;

    int mutex_error = pthread_mutex_init(&journal->mutex, NULL);
    if (mutex_error != 0)
    {
        errno = mutex_error;
        goto fail;
    }
    mutex_initialized = 1;

    if (created && fsync(directory_fd) != 0)
        goto fail;

    journal->fd = fd;
    journal->initialized = 1;
    return 0;

fail:
    {
        int saved_errno = errno;
        if (mutex_initialized)
            (void)pthread_mutex_destroy(&journal->mutex);
        (void)close(fd);
        errno = saved_errno;
        return -1;
    }
}

static int validate_prepared_record(const Fcj3PreparedRecord *prepared)
{
    if (prepared == NULL || !prepared->valid || prepared->length == 0U ||
        prepared->length > FCJ3_MAX_LINE_BYTES ||
        prepared->line[prepared->length] != '\0' ||
        prepared->line[prepared->length - 1U] != '\n' ||
        memchr(prepared->line, '\0', prepared->length) != NULL ||
        memchr(prepared->line, '\n', prepared->length - 1U) != NULL)
    {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

static ssize_t journal_write(int fd, const void *buffer, size_t length)
{
#ifdef FC_TESTING
    size_t limit = atomic_load_explicit(&test_write_fail_after,
                                        memory_order_acquire);
    size_t accepted = atomic_load_explicit(&test_write_accepted,
                                           memory_order_acquire);
    if (limit != SIZE_MAX)
    {
        if (accepted >= limit)
        {
            int injected = atomic_load_explicit(&test_write_error,
                                                memory_order_acquire);
            errno = injected != 0 ? injected : EIO;
            return -1;
        }
        if (length > limit - accepted)
            length = limit - accepted;
    }
#endif

    ssize_t written = write(fd, buffer, length);
#ifdef FC_TESTING
    if (written > 0)
        (void)atomic_fetch_add_explicit(&test_write_accepted,
                                        (size_t)written,
                                        memory_order_acq_rel);
#endif
    return written;
}

static int write_all(int fd, const char *bytes, size_t length)
{
    size_t offset = 0U;
    while (offset < length)
    {
        ssize_t written = journal_write(fd, bytes + offset, length - offset);
        if (written < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (written == 0)
        {
            errno = EIO;
            return -1;
        }
        offset += (size_t)written;
    }
    return 0;
}

static int journal_sync(int fd)
{
#ifdef FC_TESTING
    int injected = atomic_exchange_explicit(&test_fsync_error, 0,
                                            memory_order_acq_rel);
    if (injected != 0)
    {
        errno = injected;
        return -1;
    }
#endif
    return fsync(fd);
}

int fcj3_journal_append_transition(Fcj3Journal *journal,
                                   Fcj3OperationContext *context,
                                   const Fcj3PreparedRecord *prepared)
{
    if (journal == NULL || context == NULL || !journal->initialized ||
        journal->fd < 0)
    {
        errno = EINVAL;
        return -1;
    }
    if (validate_prepared_record(prepared) != 0)
        return -1;

#ifdef FC_TESTING
    atomic_store_explicit(&test_write_accepted, 0U, memory_order_release);
#endif

    int lock_error = pthread_mutex_lock(&journal->mutex);
    if (lock_error != 0)
    {
        errno = lock_error;
        return -1;
    }

    if (write_all(journal->fd, prepared->line, prepared->length) != 0)
    {
        int saved_errno = errno;
        fcj3_operation_mark_uncertain(context);
        (void)pthread_mutex_unlock(&journal->mutex);
        errno = saved_errno;
        return -1;
    }

    if (journal_sync(journal->fd) != 0)
    {
        int saved_errno = errno;
        fcj3_operation_mark_uncertain(context);
        (void)pthread_mutex_unlock(&journal->mutex);
        errno = saved_errno;
        return -1;
    }

    if (fcj3_operation_commit_transition(context, prepared) != 0)
    {
        int saved_errno = errno;
        (void)pthread_mutex_unlock(&journal->mutex);
        errno = saved_errno;
        return -1;
    }

    int unlock_error = pthread_mutex_unlock(&journal->mutex);
    if (unlock_error != 0)
    {
        fcj3_operation_mark_uncertain(context);
        errno = unlock_error;
        return -1;
    }
    return 0;
}

int fcj3_journal_close(Fcj3Journal *journal)
{
    if (journal == NULL || !journal->initialized || journal->fd < 0)
    {
        errno = EINVAL;
        return -1;
    }

    int mutex_error = pthread_mutex_destroy(&journal->mutex);
    if (mutex_error != 0)
    {
        errno = mutex_error;
        return -1;
    }

    int fd = journal->fd;
    journal->fd = -1;
    journal->initialized = 0;
    int close_result = close(fd);
    int close_errno = errno;
    memset(&journal->mutex, 0, sizeof(journal->mutex));

    if (close_result != 0)
    {
        errno = close_errno;
        return -1;
    }
    return 0;
}
