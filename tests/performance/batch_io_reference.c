#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

/* Independent content-only reference, not the production mover. */
enum { SEGMENT = 32768, SEGMENTS = 4, CAPACITY = SEGMENT * SEGMENTS };
static unsigned char buffer[CAPACITY];

static unsigned char pattern(size_t n)
{
    return (unsigned char)((n ^ (n >> 8) ^ (n >> 16)) & 255U);
}

static int fixture(void)
{
    char name[] = "/tmp/fc-batch-io-XXXXXX";
    int fd = mkstemp(name);
    if (fd < 0)
        return -1;
    if (unlink(name) != 0 || fcntl(fd, F_SETFD, FD_CLOEXEC) != 0)
    {
        close(fd);
        return -1;
    }
    return fd;
}

static int populate(int fd, size_t size)
{
    for (size_t off = 0; off < size;)
    {
        size_t n = size - off < CAPACITY ? size - off : CAPACITY;
        for (size_t i = 0; i < n; ++i)
            buffer[i] = pattern(off + i);
        size_t done = 0;
        while (done < n)
        {
            ssize_t w = write(fd, buffer + done, n - done);
            if (w < 0 && errno == EINTR)
                continue;
            if (w <= 0 || (size_t)w > n - done)
                return -1;
            done += (size_t)w;
        }
        off += n;
    }
    return lseek(fd, 0, SEEK_SET) == 0 ? 0 : -1;
}

static int verify(int fd, size_t size)
{
    struct stat st;
    if (fstat(fd, &st) != 0 || st.st_size != (off_t)size ||
        lseek(fd, 0, SEEK_SET) != 0)
        return -1;
    size_t off = 0;
    for (;;)
    {
        ssize_t n = read(fd, buffer, sizeof buffer);
        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0 || (size_t)n > size - off)
            return -1;
        if (n == 0)
            return off == size ? 0 : -1;
        for (size_t i = 0; i < (size_t)n; ++i)
            if (buffer[i] != pattern(off + i))
                return -1;
        off += (size_t)n;
    }
}

/* Reconstruct only the remaining valid range after a partial transfer,
 * preserving segment boundaries. Callers bound offset+length by CAPACITY. */
static int make_vectors(struct iovec v[SEGMENTS], size_t offset, size_t length)
{
    int count = 0;
    while (length != 0)
    {
        size_t n = SEGMENT - offset % SEGMENT;
        if (n > length)
            n = length;
        v[count].iov_base = buffer + offset;
        v[count].iov_len = n;
        ++count;
        offset += n;
        length -= n;
    }
    return count;
}

enum mode { SMALL, LARGE, VECTOR };
static int64_t elapsed_ns(struct timespec start, struct timespec end)
{
    return (int64_t)(end.tv_sec - start.tv_sec) * INT64_C(1000000000)
           + (int64_t)end.tv_nsec - (int64_t)start.tv_nsec;
}

static int transfer(int in, int out, enum mode mode, size_t size, unsigned int round)
{
    size_t reads = 0, writes = 0, total = 0;
    size_t request = mode == SMALL ? SEGMENT : CAPACITY;
    struct timespec wall_start = {0}, wall_end = {0}, cpu_start = {0}, cpu_end = {0};
    if (round != 0 &&
        (clock_gettime(CLOCK_MONOTONIC, &wall_start) != 0 ||
         clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cpu_start) != 0))
        return -1;
    for (;;)
    {
        struct iovec v[SEGMENTS];
        ssize_t n;
        do {
            ++reads;
            if (mode == VECTOR)
            {
                int count = make_vectors(v, 0, request);
                n = readv(in, v, count);
            }
            else
                n = read(in, buffer, request);
        } while (n < 0 && errno == EINTR);
        if (n < 0 || (size_t)n > request || (size_t)n > size - total)
            return -1;
        if (n == 0)
            break;
        size_t done = 0;
        while (done < (size_t)n)
        {
            ssize_t w;
            do {
                ++writes;
                if (mode == VECTOR)
                {
                    int count = make_vectors(v, done, (size_t)n - done);
                    w = writev(out, v, count);
                }
                else
                    w = write(out, buffer + done, (size_t)n - done);
            } while (w < 0 && errno == EINTR);
            if (w <= 0 || (size_t)w > (size_t)n - done)
                return -1;
            done += (size_t)w;
        }
        total += (size_t)n;
    }
    if (round != 0 &&
        (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cpu_end) != 0 ||
         clock_gettime(CLOCK_MONOTONIC, &wall_end) != 0))
        return -1;
    if (total != size || lseek(in, 0, SEEK_CUR) != (off_t)size ||
        lseek(out, 0, SEEK_CUR) != (off_t)size ||
        verify(in, size) != 0 || verify(out, size) != 0)
        return -1;
    const char *label = mode == SMALL ? "scalar32k" :
                        mode == LARGE ? "scalar128k" : "vector4x32k";
    if (round == 0)
        printf("batch_io bytes=%zu mode=%s reads=%zu writes=%zu total_calls=%zu PASS\n",
               size, label, reads, writes, reads + writes);
    else
    {
        int64_t wall = elapsed_ns(wall_start, wall_end);
        int64_t cpu = elapsed_ns(cpu_start, cpu_end);
        if (wall < 0 || cpu < 0)
            return -1;
        printf("batch_io_timing bytes=%zu mode=%s round=%u calls=%zu wall_ns=%" PRId64
               " cpu_ns=%" PRId64 " PASS\n",
               size, label, round, reads + writes, wall, cpu);
    }
    return 0;
}

static int sample(size_t size, enum mode mode, unsigned int round)
{
    int in = fixture(), out = -1, result = -1;
    if (in < 0)
        return -1;
    out = fixture();
    if (out >= 0 && populate(in, size) == 0)
        result = transfer(in, out, mode, size, round);
    if (out >= 0 && close(out) != 0)
        result = -1;
    if (close(in) != 0)
        result = -1;
    return result;
}

int main(void)
{
    const size_t sizes[] = {0, 1, SEGMENT - 1, SEGMENT, SEGMENT + 1,
                            CAPACITY, CAPACITY + 17U};
    for (size_t i = 0; i < sizeof sizes / sizeof sizes[0]; ++i)
        for (int mode = SMALL; mode <= VECTOR; ++mode)
            if (sample(sizes[i], (enum mode)mode, 0) != 0)
                return 1;
    puts("batch_io_reference=PASS samples=21");
    /* Rotate the first method so each occupies each position once per size.
     * Fresh source and destination per sample; no cache manipulation. */
    const size_t measured[] = {CAPACITY, 1024U * 1024U + 17U};
    for (unsigned int round = 1; round <= 3; ++round)
        for (size_t i = 0; i < sizeof measured / sizeof measured[0]; ++i)
            for (unsigned int position = 0; position < 3; ++position)
            {
                enum mode selected = (enum mode)((round - 1 + position) % 3);
                if (sample(measured[i], selected, round) != 0)
                    return 1;
            }
    puts("batch_io_timing=PASS samples=18 backing_bytes=131072");
    return 0;
}
