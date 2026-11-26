#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#ifdef FC_HAVE_URING_PROBE
#include <linux/io_uring.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

/* Test-only capability discovery and bounded private-fixture execution. */
enum { PROBE_OPS = 64 };

static int supports(const struct io_uring_probe *probe, unsigned int op)
{
    for (unsigned int i = 0; i < probe->ops_len; ++i)
        if (probe->ops[i].op == op)
            return (probe->ops[i].flags & IO_URING_OP_SUPPORTED) != 0;
    return 0;
}

/* Bounded test-only shared-ring access; no SQPOLL or registered resources.
 * Acquire/release pairs with kernel ring ownership. */
static unsigned int load_acquire(const unsigned int *p)
{
    return __atomic_load_n(p, __ATOMIC_ACQUIRE);
}
static void store_release(unsigned int *p, unsigned int v)
{
    __atomic_store_n(p, v, __ATOMIC_RELEASE);
}
static int field_ok(size_t offset, size_t bytes, size_t align, size_t length)
{
    return offset % align == 0 && offset <= length && bytes <= length - offset;
}

/* Static lifetime: a failed/short enter may leave work pending until ring
 * teardown. Keep iovecs and data alive through process exit on failure. */
enum { IO_BATCH = 4, IO_BYTES = 32 };
static unsigned char data[IO_BATCH][IO_BYTES];
static struct iovec vectors[IO_BATCH][2];
static unsigned char rejected_data[IO_BYTES], eof_data[IO_BYTES];
static struct iovec rejected_vector, eof_vector;

static int private_file(void)
{
    char path[] = "/tmp/fc-uring-io-XXXXXX";
    int fd = mkostemp(path, O_CLOEXEC);
    if (fd < 0)
        return -1;
    if (unlink(path) != 0)
    {
        close(fd);
        return -1;
    }
    return fd;
}

static unsigned char expected_byte(size_t i)
{
    return (unsigned char)((i * 17U + 29U) & 255U);
}

static int verify_file(int fd)
{
    unsigned char bytes[IO_BATCH * IO_BYTES];
    struct stat st;
    if (fstat(fd, &st) != 0 || st.st_size != (off_t)sizeof bytes ||
        pread(fd, bytes, sizeof bytes, 0) != (ssize_t)sizeof bytes ||
        lseek(fd, 0, SEEK_CUR) != 0)
        return -1;
    for (size_t i = 0; i < sizeof bytes; ++i)
        if (bytes[i] != expected_byte(i))
            return -1;
    return 0;
}

static int execute_batches(int fd, const struct io_uring_params *p)
{
    enum { BATCH = IO_BATCH, NOP_ROUNDS = 4, MIXED_ROUND = 6,
           RECOVERY_ROUND = 7, ROUNDS = 8, LIMIT = 1024 * 1024 };
    if (p->sq_entries < BATCH || p->sq_entries > 64 ||
        p->cq_entries < BATCH || p->cq_entries > 128 ||
        p->sq_off.array > LIMIT || p->cq_off.cqes > LIMIT)
        return 1;
    size_t sq_len = (size_t)p->sq_off.array + p->sq_entries * sizeof(unsigned int);
    size_t cq_len = (size_t)p->cq_off.cqes + p->cq_entries * sizeof(struct io_uring_cqe);
    size_t entries_len = p->sq_entries * sizeof(struct io_uring_sqe);
    int shared = (p->features & IORING_FEAT_SINGLE_MMAP) != 0;
    if (shared)
        sq_len = cq_len = sq_len > cq_len ? sq_len : cq_len;
    if (sq_len > LIMIT || cq_len > LIMIT)
        return 1;
#define CHECK_FIELD(offset, length) \
    field_ok((offset), sizeof(unsigned int), _Alignof(unsigned int), (length))
    if (!CHECK_FIELD(p->sq_off.head, sq_len) ||
        !CHECK_FIELD(p->sq_off.tail, sq_len) ||
        !CHECK_FIELD(p->sq_off.ring_mask, sq_len) ||
        !CHECK_FIELD(p->sq_off.ring_entries, sq_len) ||
        !CHECK_FIELD(p->sq_off.dropped, sq_len) ||
        !CHECK_FIELD(p->cq_off.head, cq_len) ||
        !CHECK_FIELD(p->cq_off.tail, cq_len) ||
        !CHECK_FIELD(p->cq_off.ring_mask, cq_len) ||
        !CHECK_FIELD(p->cq_off.ring_entries, cq_len) ||
        !CHECK_FIELD(p->cq_off.overflow, cq_len) ||
        !field_ok(p->sq_off.array, p->sq_entries * sizeof(unsigned int),
                  _Alignof(unsigned int), sq_len) ||
        !field_ok(p->cq_off.cqes, p->cq_entries * sizeof(struct io_uring_cqe),
                  _Alignof(struct io_uring_cqe), cq_len))
        return 1;
#undef CHECK_FIELD
    int result = 1, source = -1, destination = -1;
    char *sq = mmap(NULL, sq_len, PROT_READ | PROT_WRITE, MAP_SHARED,
                    fd, IORING_OFF_SQ_RING);
    char *cq = MAP_FAILED;
    struct io_uring_sqe *entries = MAP_FAILED;
    if (sq == MAP_FAILED)
        goto cleanup;
    cq = shared ? sq : mmap(NULL, cq_len, PROT_READ | PROT_WRITE, MAP_SHARED,
                            fd, IORING_OFF_CQ_RING);
    if (cq == MAP_FAILED)
        goto cleanup;
    entries = mmap(NULL, entries_len, PROT_READ | PROT_WRITE, MAP_SHARED,
                   fd, IORING_OFF_SQES);
    if (entries == MAP_FAILED)
        goto cleanup;
#define PTR(base, offset) ((unsigned int *)((base) + (offset)))
    unsigned int *sh = PTR(sq, p->sq_off.head), *st = PTR(sq, p->sq_off.tail);
    unsigned int *ch = PTR(cq, p->cq_off.head), *ct = PTR(cq, p->cq_off.tail);
    unsigned int *array = PTR(sq, p->sq_off.array);
    struct io_uring_cqe *completions = (void *)(cq + p->cq_off.cqes);
    unsigned int sm = load_acquire(PTR(sq, p->sq_off.ring_mask));
    unsigned int cm = load_acquire(PTR(cq, p->cq_off.ring_mask));
    if (load_acquire(PTR(sq, p->sq_off.ring_entries)) != p->sq_entries ||
        load_acquire(PTR(cq, p->cq_off.ring_entries)) != p->cq_entries ||
        sm != p->sq_entries - 1 || cm != p->cq_entries - 1 ||
        (p->sq_entries & sm) || (p->cq_entries & cm))
        goto cleanup;
    source = private_file();
    destination = private_file();
    if (source < 0 || destination < 0)
        goto cleanup;
    unsigned char initial[BATCH * IO_BYTES];
    for (size_t i = 0; i < sizeof initial; ++i)
        initial[i] = expected_byte(i);
    if (pwrite(source, initial, sizeof initial, 0) != (ssize_t)sizeof initial)
        goto cleanup;
    for (unsigned int i = 0; i < BATCH; ++i)
    {
        vectors[i][0].iov_base = data[i];
        vectors[i][0].iov_len = 13;
        vectors[i][1].iov_base = data[i] + 13;
        vectors[i][1].iov_len = IO_BYTES - 13;
    }
    memset(rejected_data, 0xa5, sizeof rejected_data);
    memset(eof_data, 0x5a, sizeof eof_data);
    rejected_vector = (struct iovec){.iov_base = rejected_data, .iov_len = sizeof rejected_data};
    eof_vector = (struct iovec){.iov_base = eof_data, .iov_len = sizeof eof_data};
    for (unsigned int round = 0; round < ROUNDS; ++round)
    {
        unsigned int tail = load_acquire(st), head = load_acquire(sh);
        unsigned int c_head = load_acquire(ch);
        if (tail != head || load_acquire(ct) != c_head)
            goto cleanup;
        for (unsigned int i = 0; i < BATCH; ++i)
        {
            unsigned int slot = (tail + i) & sm;
            memset(&entries[slot], 0, sizeof entries[slot]);
            if (round < NOP_ROUNDS)
                entries[slot].opcode = IORING_OP_NOP;
            else if (round == MIXED_ROUND)
            {
                /* Prior write batch fully completed before FSYNC submission.
                 * All other requests here are independent of that sync. */
                if (i == 0)
                {
                    entries[slot].opcode = IORING_OP_FSYNC;
                    entries[slot].fd = destination;
                }
                else if (i == 3)
                    entries[slot].opcode = IORING_OP_NOP;
                else
                {
                    entries[slot].opcode = IORING_OP_READV;
                    entries[slot].fd = i == 1 ? -1 : source;
                    entries[slot].off = i == 1 ? 0 : BATCH * IO_BYTES;
                    entries[slot].addr = (uint64_t)(uintptr_t)
                        (i == 1 ? &rejected_vector : &eof_vector);
                    entries[slot].len = 1;
                }
            }
            else
            {
                entries[slot].opcode = round == NOP_ROUNDS + 1 ?
                    IORING_OP_WRITEV : IORING_OP_READV;
                entries[slot].fd = round == NOP_ROUNDS ? source : destination;
                entries[slot].off = (uint64_t)i * IO_BYTES;
                entries[slot].addr = (uint64_t)(uintptr_t)vectors[i];
                entries[slot].len = 2;
            }
            entries[slot].user_data = (uint64_t)round * BATCH + i + 1;
            array[slot] = slot;
        }
        store_release(st, tail + BATCH);
        /* Without SQPOLL, enter submits explicitly. A short submit is a
         * failed test, never silently accepted. CTest bounds blocking. */
        long rc = syscall(SYS_io_uring_enter, fd, BATCH, BATCH,
                          IORING_ENTER_GETEVENTS, NULL, 0U);
        if (rc != BATCH)
            goto cleanup;
        if (load_acquire(sh) != tail + BATCH ||
            load_acquire(ct) - c_head != BATCH ||
            load_acquire(PTR(sq, p->sq_off.dropped)) != 0 ||
            load_acquire(PTR(cq, p->cq_off.overflow)) != 0)
            goto cleanup;
        unsigned int seen = 0;
        for (unsigned int i = 0; i < BATCH; ++i)
        {
            struct io_uring_cqe c = completions[(c_head + i) & cm];
            uint64_t first = (uint64_t)round * BATCH + 1;
            if (c.flags != 0 || c.user_data < first ||
                c.user_data - first >= BATCH)
                goto cleanup;
            unsigned int index = (unsigned int)(c.user_data - first);
            int expected = round < NOP_ROUNDS ? 0 : IO_BYTES;
            if (round == MIXED_ROUND)
                expected = index == 1 ? -EBADF : 0;
            if (c.res != expected)
                goto cleanup;
            unsigned int bit = 1U << index;
            if (seen & bit)
                goto cleanup;
            seen |= bit;
        }
        if (seen != (1U << BATCH) - 1U)
            goto cleanup;
        store_release(ch, c_head + BATCH);
        if (round == NOP_ROUNDS - 1)
            puts("uring_nop=PASS batches=4 completions=16");
        if (round == NOP_ROUNDS || round == RECOVERY_ROUND)
            for (unsigned int i = 0; i < BATCH; ++i)
                for (size_t j = 0; j < IO_BYTES; ++j)
                    if (data[i][j] != expected_byte((size_t)i * IO_BYTES + j))
                        goto cleanup;
        if (round == NOP_ROUNDS + 1)
        {
            if (verify_file(source) != 0 || verify_file(destination) != 0)
                goto cleanup;
            puts("uring_vectored=PASS readv=4 writev=4 bytes=128 iovecs_per_request=2");
        }
        if (round == MIXED_ROUND)
        {
            for (size_t j = 0; j < IO_BYTES; ++j)
                if (rejected_data[j] != 0xa5 || eof_data[j] != 0x5a)
                    goto cleanup;
            /* All mixed completions consumed; reuse existing read buffers
             * with distinct sentinel content for the recovery read batch. */
            memset(data, 0, sizeof data);
        }
    }
    if (verify_file(source) != 0 || verify_file(destination) != 0)
        goto cleanup;
    puts("uring_sync_errors=PASS fsync=1 ebadf=1 eof=1 nop=1 recovery_readv=4 total_completions=32");
    result = 0;
#undef PTR
cleanup:
    if (destination >= 0 && close(destination) != 0) result = 1;
    if (source >= 0 && close(source) != 0) result = 1;
    if (entries != MAP_FAILED && munmap(entries, entries_len) != 0)
        result = 1;
    if (!shared && cq != MAP_FAILED && munmap(cq, cq_len) != 0)
        result = 1;
    if (sq != MAP_FAILED && munmap(sq, sq_len) != 0)
        result = 1;
    return result;
}

static int probe_ring(void)
{
    struct io_uring_params params = {0};
    int fd = (int)syscall(SYS_io_uring_setup, 8U, &params);
    if (fd < 0)
    {
        int error = errno;
        if (error == ENOSYS || error == EPERM || error == EACCES)
        {
            printf("uring_setup=NOT_VERIFIED errno=%d\n", error);
            return 77;
        }
        fprintf(stderr, "unexpected io_uring_setup errno=%d\n", error);
        return 1;
    }
    int result = 1;
    struct io_uring_probe *probe = NULL;
    if (params.sq_entries < 8U || params.cq_entries < params.sq_entries)
        goto cleanup;
    probe = calloc(1, sizeof *probe + PROBE_OPS * sizeof probe->ops[0]);
    if (!probe)
        goto cleanup;
    long rc;
    do {
        rc = syscall(SYS_io_uring_register, fd, IORING_REGISTER_PROBE,
                     probe, PROBE_OPS);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0)
    {
        int error = errno;
        if (error == EINVAL || error == ENOSYS || error == EPERM ||
            error == EACCES || error == EOPNOTSUPP)
        {
            printf("uring_setup=PASS probe=NOT_VERIFIED errno=%d\n", error);
            result = 77;
        }
        else
            fprintf(stderr, "unexpected io_uring_register errno=%d\n", error);
        goto cleanup;
    }
    if (rc != 0 || probe->ops_len > PROBE_OPS)
        goto cleanup;
    for (unsigned int i = 0; i < probe->ops_len; ++i)
    {
        if (probe->ops[i].op > probe->last_op)
            goto cleanup;
        for (unsigned int j = 0; j < i; ++j)
            if (probe->ops[i].op == probe->ops[j].op)
                goto cleanup;
    }
    int nop = supports(probe, IORING_OP_NOP);
    int readv = supports(probe, IORING_OP_READV);
    int writev = supports(probe, IORING_OP_WRITEV);
    int sync = supports(probe, IORING_OP_FSYNC);
    printf("uring_probe sq=%u cq=%u features=0x%x ops=%u nop=%d readv=%d writev=%d fsync=%d\n",
           params.sq_entries, params.cq_entries, params.features,
           (unsigned int)probe->ops_len, nop, readv, writev, sync);
    result = nop && readv && writev && sync ? 0 : 77;
    if (result == 77)
        puts("uring_required_ops=NOT_VERIFIED");
    else if (result == 0)
        result = execute_batches(fd, &params);
cleanup:
    free(probe);
    if (close(fd) != 0)
        result = 1;
    return result;
}
#endif

int main(void)
{
#ifdef FC_HAVE_URING_PROBE
    return probe_ring();
#else
    puts("uring_build_capability=NOT_VERIFIED");
    return 77;
#endif
}
