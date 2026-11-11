#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "fcj4_test_append.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s (errno=%d)\n", \
                __FILE__, __LINE__, #expr, errno); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

#define CHECK_PTHREAD(call) do { \
    int pthread_error_ = (call); \
    if (pthread_error_ != 0) { \
        fprintf(stderr, "pthread error at %s:%d: %s => %d\n", \
                __FILE__, __LINE__, #call, pthread_error_); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static const char record_bytes[] = "FCJ4|BEGIN|concurrency-test\n";

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    bool block_first_write;
    bool callback_entered;
    bool release_callback;
    int pwrite_calls;
    int fsync_calls;
    int lock_calls;
    int unlock_calls;
    int close_calls;
} TestGate;

typedef struct {
    Fcj4TestAppend *context;
    Fcj4TestAppendResult result;
    int rc;
    int error;
} AppendThread;

static void gate_init(TestGate *gate, bool block_first_write)
{
    memset(gate, 0, sizeof(*gate));
    CHECK_PTHREAD(pthread_mutex_init(&gate->mutex, NULL));
    CHECK_PTHREAD(pthread_cond_init(&gate->condition, NULL));
    gate->block_first_write = block_first_write;
}

static void gate_wait_for_callback(TestGate *gate)
{
    CHECK_PTHREAD(pthread_mutex_lock(&gate->mutex));
    while (!gate->callback_entered)
        CHECK_PTHREAD(pthread_cond_wait(&gate->condition, &gate->mutex));
    CHECK_PTHREAD(pthread_mutex_unlock(&gate->mutex));
}

static void gate_release_callback(TestGate *gate)
{
    CHECK_PTHREAD(pthread_mutex_lock(&gate->mutex));
    gate->release_callback = true;
    CHECK_PTHREAD(pthread_cond_broadcast(&gate->condition));
    CHECK_PTHREAD(pthread_mutex_unlock(&gate->mutex));
}

static int gate_counter(TestGate *gate, int which)
{
    int value;
    CHECK_PTHREAD(pthread_mutex_lock(&gate->mutex));
    switch (which) {
    case 0: value = gate->pwrite_calls; break;
    case 1: value = gate->fsync_calls; break;
    case 2: value = gate->unlock_calls; break;
    case 3: value = gate->close_calls; break;
    default: value = gate->lock_calls; break;
    }
    CHECK_PTHREAD(pthread_mutex_unlock(&gate->mutex));
    return value;
}

static void gate_destroy(TestGate *gate)
{
    CHECK_PTHREAD(pthread_cond_destroy(&gate->condition));
    CHECK_PTHREAD(pthread_mutex_destroy(&gate->mutex));
}

static ssize_t gated_pwrite(void *user, int fd, const void *buffer,
                            size_t length, off_t offset)
{
    TestGate *gate = (TestGate *)user;
    bool pause;

    CHECK_PTHREAD(pthread_mutex_lock(&gate->mutex));
    gate->pwrite_calls++;
    pause = gate->block_first_write;
    if (pause) {
        gate->block_first_write = false;
        gate->callback_entered = true;
        CHECK_PTHREAD(pthread_cond_broadcast(&gate->condition));
        while (!gate->release_callback)
            CHECK_PTHREAD(pthread_cond_wait(&gate->condition, &gate->mutex));
    }
    CHECK_PTHREAD(pthread_mutex_unlock(&gate->mutex));
    return pwrite(fd, buffer, length, offset);
}

static int gated_fsync(void *user, int fd)
{
    TestGate *gate = (TestGate *)user;
    CHECK_PTHREAD(pthread_mutex_lock(&gate->mutex));
    gate->fsync_calls++;
    CHECK_PTHREAD(pthread_mutex_unlock(&gate->mutex));
    return fsync(fd);
}

static int gated_close(void *user, int fd)
{
    TestGate *gate = (TestGate *)user;
    CHECK_PTHREAD(pthread_mutex_lock(&gate->mutex));
    gate->close_calls++;
    CHECK_PTHREAD(pthread_mutex_unlock(&gate->mutex));
    return close(fd);
}

static int gated_flock(void *user, int fd, int operation)
{
    TestGate *gate = (TestGate *)user;
    CHECK_PTHREAD(pthread_mutex_lock(&gate->mutex));
    if (operation == LOCK_UN)
        gate->unlock_calls++;
    else
        gate->lock_calls++;
    CHECK_PTHREAD(pthread_mutex_unlock(&gate->mutex));
    return flock(fd, operation);
}

static int real_fstat(void *user, int fd, struct stat *out)
{
    (void)user;
    return fstat(fd, out);
}

static int real_getfl(void *user, int fd)
{
    (void)user;
    return fcntl(fd, F_GETFL);
}

static int real_getfd(void *user, int fd)
{
    (void)user;
    return fcntl(fd, F_GETFD);
}

static Fcj4TestAppendOps make_ops(TestGate *gate)
{
    Fcj4TestAppendOps ops;
    memset(&ops, 0, sizeof(ops));
    ops.pwrite_fn = gated_pwrite;
    ops.fsync_fn = gated_fsync;
    ops.close_fn = gated_close;
    ops.flock_fn = gated_flock;
    ops.fstat_fn = real_fstat;
    ops.getfl_fn = real_getfl;
    ops.getfd_fn = real_getfd;
    ops.user = gate;
    return ops;
}

static int make_fixture(char path[128])
{
    char directory[] = "/tmp/fcj4-concurrency-XXXXXX";
    int fd;
    CHECK(mkdtemp(directory) != NULL);
    CHECK(snprintf(path, 128U, "%s/journal", directory) > 0);
    fd = open(path, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    CHECK(fd >= 0);
    return fd;
}

static void remove_fixture(const char *path)
{
    char directory[128];
    char *slash;
    CHECK(strlen(path) < sizeof(directory));
    memcpy(directory, path, strlen(path) + 1U);
    slash = strrchr(directory, '/');
    CHECK(slash != NULL);
    *slash = '\0';
    CHECK(unlink(path) == 0);
    CHECK(rmdir(directory) == 0);
}

static Fcj4TestAppend *init_context(int fd, TestGate *gate)
{
    struct stat st;
    Fcj4TestAppend *context = NULL;
    Fcj4TestAppendOps ops = make_ops(gate);
    CHECK(fstat(fd, &st) == 0);
    CHECK(fcj4_test_append_init(&context, fd, st.st_dev, st.st_ino,
                                0U, &ops) == 0);
    CHECK(context != NULL);
    return context;
}

static void *append_worker(void *argument)
{
    AppendThread *thread = (AppendThread *)argument;
    errno = 0;
    thread->rc = fcj4_test_append_record(
        thread->context, record_bytes, sizeof(record_bytes) - 1U,
        0U, &thread->result);
    thread->error = errno;
    return NULL;
}

static void start_blocked_append(TestGate *gate, Fcj4TestAppend *context,
                                 AppendThread *thread, pthread_t *worker)
{
    memset(thread, 0, sizeof(*thread));
    thread->context = context;
    CHECK_PTHREAD(pthread_create(worker, NULL, append_worker, thread));
    gate_wait_for_callback(gate);
}

static void finish_blocked_append(TestGate *gate, pthread_t worker)
{
    gate_release_callback(gate);
    CHECK_PTHREAD(pthread_join(worker, NULL));
}

static void acknowledge_and_destroy(Fcj4TestAppend *context,
                                    const char *path, TestGate *gate)
{
    Fcj4TestAppendResult result;
    CHECK(fcj4_test_append_ack_commit(
        context, record_bytes, sizeof(record_bytes) - 1U) == 0);
    CHECK(fcj4_test_append_destroy(context, &result) == 0);
    CHECK(gate_counter(gate, 2) == 1);
    CHECK(gate_counter(gate, 3) == 1);
    remove_fixture(path);
}

static void test_append_rejected_while_another_thread_appends(void)
{
    char path[128];
    int fd = make_fixture(path);
    TestGate gate;
    Fcj4TestAppend *context;
    AppendThread worker_result;
    pthread_t worker;
    Fcj4TestAppendResult competing_result;
    int competing_rc, competing_errno;

    gate_init(&gate, true);
    context = init_context(fd, &gate);
    start_blocked_append(&gate, context, &worker_result, &worker);

    errno = 0;
    competing_rc = fcj4_test_append_record(
        context, record_bytes, sizeof(record_bytes) - 1U,
        0U, &competing_result);
    competing_errno = errno;

    finish_blocked_append(&gate, worker);
    CHECK(competing_rc == -1 && competing_errno == EBUSY);
    CHECK(worker_result.rc == 0 && worker_result.error == 0);
    CHECK(worker_result.result.code == FCJ4_TEST_APPEND_DURABLE);
    CHECK(gate_counter(&gate, 0) == 1);
    {
        struct stat st;
        CHECK(stat(path, &st) == 0);
        CHECK(st.st_size == (off_t)(sizeof(record_bytes) - 1U));
    }
    CHECK(gate_counter(&gate, 1) == 1);
    acknowledge_and_destroy(context, path, &gate);
    gate_destroy(&gate);
}

static void test_destroy_rejected_while_append_is_active(void)
{
    char path[128];
    int fd = make_fixture(path);
    TestGate gate;
    Fcj4TestAppend *context;
    AppendThread worker_result;
    pthread_t worker;
    Fcj4TestAppendResult destroy_result;
    int destroy_rc, destroy_errno;

    gate_init(&gate, true);
    context = init_context(fd, &gate);
    start_blocked_append(&gate, context, &worker_result, &worker);

    errno = 0;
    destroy_rc = fcj4_test_append_destroy(context, &destroy_result);
    destroy_errno = errno;
    finish_blocked_append(&gate, worker);
    CHECK(destroy_rc == -1 && destroy_errno == EBUSY);
    CHECK(gate_counter(&gate, 2) == 0);
    CHECK(gate_counter(&gate, 3) == 0);
    CHECK(worker_result.rc == 0);
    acknowledge_and_destroy(context, path, &gate);
    gate_destroy(&gate);
}

static void test_acknowledgement_rejected_while_append_is_active(void)
{
    char path[128];
    int fd = make_fixture(path);
    TestGate gate;
    Fcj4TestAppend *context;
    AppendThread worker_result;
    pthread_t worker;
    int ack_rc, ack_errno;

    gate_init(&gate, true);
    context = init_context(fd, &gate);
    start_blocked_append(&gate, context, &worker_result, &worker);

    errno = 0;
    ack_rc = fcj4_test_append_ack_commit(
        context, record_bytes, sizeof(record_bytes) - 1U);
    ack_errno = errno;

    finish_blocked_append(&gate, worker);
    CHECK(ack_rc == -1 && ack_errno == EBUSY);
    CHECK(worker_result.rc == 0);
    acknowledge_and_destroy(context, path, &gate);
    gate_destroy(&gate);
}

static void test_poison_contention_does_not_change_active_append(void)
{
    char path[128];
    int fd = make_fixture(path);
    TestGate gate;
    Fcj4TestAppend *context;
    AppendThread worker_result;
    pthread_t worker;
    Fcj4TestAppendResult second_result;

    gate_init(&gate, true);
    context = init_context(fd, &gate);
    start_blocked_append(&gate, context, &worker_result, &worker);

    fcj4_test_append_poison(context);
    finish_blocked_append(&gate, worker);

    CHECK(worker_result.rc == 0);
    CHECK(worker_result.result.code == FCJ4_TEST_APPEND_DURABLE);
    CHECK(fcj4_test_append_ack_commit(
        context, record_bytes, sizeof(record_bytes) - 1U) == 0);

    errno = 0;
    CHECK(fcj4_test_append_record(
        context, record_bytes, sizeof(record_bytes) - 1U,
        sizeof(record_bytes) - 1U, &second_result) == 0);
    CHECK(second_result.code == FCJ4_TEST_APPEND_DURABLE);
    CHECK(fcj4_test_append_ack_commit(
        context, record_bytes, sizeof(record_bytes) - 1U) == 0);

    CHECK(fcj4_test_append_destroy(context, &second_result) == 0);
    CHECK(gate_counter(&gate, 2) == 1);
    CHECK(gate_counter(&gate, 3) == 1);
    remove_fixture(path);
    gate_destroy(&gate);
}

static void test_independent_contexts_run_concurrently(void)
{
    char path_a[128], path_b[128];
    int fd_a = make_fixture(path_a);
    int fd_b = make_fixture(path_b);
    TestGate gate_a, gate_b;
    Fcj4TestAppend *context_a, *context_b;
    AppendThread result_a, result_b;
    pthread_t worker_a, worker_b;

    gate_init(&gate_a, true);
    gate_init(&gate_b, true);
    context_a = init_context(fd_a, &gate_a);
    context_b = init_context(fd_b, &gate_b);

    start_blocked_append(&gate_a, context_a, &result_a, &worker_a);
    start_blocked_append(&gate_b, context_b, &result_b, &worker_b);

    gate_release_callback(&gate_a);
    gate_release_callback(&gate_b);
    CHECK_PTHREAD(pthread_join(worker_a, NULL));
    CHECK_PTHREAD(pthread_join(worker_b, NULL));

    CHECK(result_a.rc == 0 && result_b.rc == 0);
    CHECK(result_a.result.code == FCJ4_TEST_APPEND_DURABLE);
    CHECK(result_b.result.code == FCJ4_TEST_APPEND_DURABLE);
    CHECK(gate_counter(&gate_a, 0) == 1);
    CHECK(gate_counter(&gate_b, 0) == 1);

    acknowledge_and_destroy(context_a, path_a, &gate_a);
    acknowledge_and_destroy(context_b, path_b, &gate_b);
    gate_destroy(&gate_a);
    gate_destroy(&gate_b);
}

void test_fcj4_cross_thread_concurrency(void)
{
    test_append_rejected_while_another_thread_appends();
    test_destroy_rejected_while_append_is_active();
    test_acknowledgement_rejected_while_append_is_active();
    test_poison_contention_does_not_change_active_append();
    test_independent_contexts_run_concurrently();
}
