#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>

/* Independent test-only content-transfer model. No production mover calls. */
enum { CHUNK = 128 * 1024 };
static unsigned char buffer[CHUNK];

static unsigned char pattern(size_t i)
{
    return (unsigned char)((i ^ (i >> 8) ^ (i >> 16)) & 255U);
}

static int write_all(int fd, const unsigned char *data, size_t size)
{
    size_t done = 0;
    while (done < size)
    {
        ssize_t n = write(fd, data + done, size - done);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return -1;
        done += (size_t)n;
    }
    return 0;
}

static int populate(int fd, size_t size)
{
    for (size_t offset = 0; offset < size;)
    {
        size_t n = size - offset < CHUNK ? size - offset : CHUNK;
        for (size_t i = 0; i < n; ++i)
            buffer[i] = pattern(offset + i);
        if (write_all(fd, buffer, n) != 0)
            return -1;
        offset += n;
    }
    return 0;
}

static int verify(int fd, size_t size)
{
    struct stat st;
    if (fstat(fd, &st) != 0 || st.st_size != (off_t)size ||
        lseek(fd, 0, SEEK_SET) != 0)
        return -1;
    size_t offset = 0;
    for (;;)
    {
        ssize_t n = read(fd, buffer, sizeof buffer);
        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0 || (size_t)n > size - offset)
            return -1;
        if (n == 0)
            return offset == size ? 0 : -1;
        for (size_t i = 0; i < (size_t)n; ++i)
            if (buffer[i] != pattern(offset + i))
                return -1;
        offset += (size_t)n;
    }
}

static int buffered(int in, int out)
{
    for (;;)
    {
        ssize_t n = read(in, buffer, sizeof buffer);
        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0)
            return -1;
        if (n == 0)
            return 0;
        if (write_all(out, buffer, (size_t)n) != 0)
            return -1;
    }
}

/* Return 77 only for an unsupported first nonempty transfer, not for a
 * partial transfer or unexpected error. Never switch to another method. */
static int ranged(int in, int out, size_t size, int allow_skip)
{
    size_t done = 0;
    while (done < size)
    {
        size_t request = size - done < CHUNK ? size - done : CHUNK;
        ssize_t n = copy_file_range(in, NULL, out, NULL, request, 0);
        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0 && done == 0 && allow_skip &&
            (errno == ENOSYS || errno == EOPNOTSUPP || errno == EXDEV))
        {
            printf("copy_file_range capability NOT VERIFIED errno=%d\n", errno);
            return 77;
        }
        if (n <= 0 || (size_t)n > request)
            return -1;
        done += (size_t)n;
    }
    ssize_t end;
    do { end = copy_file_range(in, NULL, out, NULL, 1, 0); }
    while (end < 0 && errno == EINTR);
    if (end < 0 && size == 0 && allow_skip &&
        (errno == ENOSYS || errno == EOPNOTSUPP || errno == EXDEV))
        return 77;
    return end == 0 ? 0 : -1;
}

/* Time only the content-transfer loop; verify offsets and every byte
 * outside the interval. No fsync, preparation or production mover timing. */
static int transfer(int in, int out, size_t size, int reference,
                    int allow_skip, int run)
{
    struct timespec start, end;
    if (lseek(in, 0, SEEK_SET) != 0 ||
        clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        return -1;
    int rc = reference ? ranged(in, out, size, allow_skip) : buffered(in, out);
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0)
        return -1;
    if (rc != 0)
        return rc;
    if (lseek(in, 0, SEEK_CUR) != (off_t)size ||
        lseek(out, 0, SEEK_CUR) != (off_t)size ||
        verify(out, size) != 0 || verify(in, size) != 0)
        return -1;
    int64_t elapsed = (int64_t)(end.tv_sec - start.tv_sec) * INT64_C(1000000000)
                      + (int64_t)end.tv_nsec - (int64_t)start.tv_nsec;
    if (elapsed < 0)
        return -1;
    if (run > 0)
        printf("io_timing bytes=%zu mode=%s run=%d elapsed_ns=%" PRId64 "\n",
               size, reference ? "copy_file_range" : "buffered", run, elapsed);
    return 0;
}

static int sample(int dir, size_t size, int *capability, int run)
{
    int in = -1, baseline = -1, candidate = -1, result = -1;
    in = openat(dir, "source", O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600);
    if (in < 0)
        goto cleanup;
    baseline = openat(dir, "baseline", O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600);
    if (baseline < 0)
        goto cleanup;
    candidate = openat(dir, "candidate", O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600);
    if (candidate < 0)
        goto cleanup;
    if (populate(in, size) != 0)
        goto cleanup;
    /* Alternate first transfer to reduce fixed ordering bias. */
    for (int step = 0; step < 2; ++step)
    {
        int reference = (run % 2 == 0) ? step : 1 - step;
        if (reference && *capability == 77)
            continue;
        int rc = transfer(in, reference ? candidate : baseline, size,
                          reference, *capability == 0, run);
        if (reference && rc == 77)
            *capability = 77;
        else
        {
            if (rc != 0)
                goto cleanup;
            if (reference && size != 0)
                *capability = 1;
        }
    }
    printf("io_reference bytes=%zu baseline=PASS candidate=%s\n",
           size, *capability == 77 ? "NOT_VERIFIED" : "PASS");
    result = 0;
cleanup:
    if (candidate >= 0)
    {
        if (close(candidate) != 0) result = -1;
        if (unlinkat(dir, "candidate", 0) != 0) result = -1;
    }
    if (baseline >= 0)
    {
        if (close(baseline) != 0) result = -1;
        if (unlinkat(dir, "baseline", 0) != 0) result = -1;
    }
    if (in >= 0)
    {
        if (close(in) != 0) result = -1;
        if (unlinkat(dir, "source", 0) != 0) result = -1;
    }
    return result;
}

/* Explicit offsets must not alter either open file description's position.
 * Use existing bytes in a private destination to detect unintended writes. */
static int explicit_transfer(int in, int out, off_t src, off_t dst,
                             size_t requested, size_t expected)
{
    size_t done = 0;
    off_t input = src, output = dst;
    while (done < requested)
    {
        ssize_t n = copy_file_range(in, &input, out, &output, requested - done, 0);
        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0 || (size_t)n > requested - done)
            return -1;
        if (n == 0)
            break;
        done += (size_t)n;
    }
    return done == expected && input == src + (off_t)expected &&
           output == dst + (off_t)expected && lseek(in, 0, SEEK_CUR) == 7 &&
           lseek(out, 0, SEEK_CUR) == 13 ? 0 : -1;
}

static int explicit_offset_fixture(int dir)
{
    enum { SIZE = 4096 };
    unsigned char actual[SIZE];
    int in = -1, out = -1, result = -1;
    in = openat(dir, "offset-source", O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600);
    if (in < 0)
        goto cleanup;
    out = openat(dir, "offset-dest", O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600);
    if (out < 0)
        goto cleanup;
    memset(actual, 0xa5, sizeof actual);
    if (populate(in, SIZE) != 0 || write_all(out, actual, sizeof actual) != 0 ||
        lseek(in, 7, SEEK_SET) != 7 || lseek(out, 13, SEEK_SET) != 13)
        goto cleanup;
    /* Disjoint destination ranges; second request crosses input EOF. */
    if (explicit_transfer(in, out, 123, 700, 511, 511) != 0 ||
        explicit_transfer(in, out, SIZE - 19, 2100, 100, 19) != 0 ||
        explicit_transfer(in, out, SIZE, 3000, 100, 0) != 0)
        goto cleanup;
    struct stat st;
    if (fstat(out, &st) != 0 || st.st_size != SIZE ||
        lseek(out, 0, SEEK_SET) != 0)
        goto cleanup;
    size_t done = 0;
    while (done < sizeof actual)
    {
        ssize_t n = read(out, actual + done, sizeof actual - done);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            goto cleanup;
        done += (size_t)n;
    }
    for (size_t i = 0; i < sizeof actual; ++i)
    {
        unsigned char expected = 0xa5;
        if (i >= 700 && i < 1211)
            expected = pattern(123 + i - 700);
        else if (i >= 2100 && i < 2119)
            expected = pattern(SIZE - 19 + i - 2100);
        if (actual[i] != expected)
            goto cleanup;
    }
    if (verify(in, SIZE) != 0)
        goto cleanup;
    puts("io_explicit_offsets=PASS ranges=3 bytes_copied=530 untouched=3566");
    result = 0;
cleanup:
    if (out >= 0)
    {
        if (close(out) != 0) result = -1;
        if (unlinkat(dir, "offset-dest", 0) != 0) result = -1;
    }
    if (in >= 0)
    {
        if (close(in) != 0) result = -1;
        if (unlinkat(dir, "offset-source", 0) != 0) result = -1;
    }
    return result;
}

int main(void)
{
    char root[] = "/tmp/fc-io-reference-XXXXXX";
    if (!mkdtemp(root))
        return 1;
    int dir = open(root, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    int result = 0, capability = 0;
    const size_t sizes[] = {1, 0, CHUNK - 1U, CHUNK, CHUNK + 1U, 1024U * 1024U + 17U};
    if (dir < 0)
        result = 1;
    else
    {
        for (size_t i = 0; i < sizeof sizes / sizeof sizes[0]; ++i)
            if (sample(dir, sizes[i], &capability, 0) != 0)
            {
                result = 1;
                break;
            }
        const size_t measured[] = {CHUNK, 1024U * 1024U + 17U};
        for (int run = 1; run <= 3 && result == 0 && capability == 1; ++run)
            for (size_t i = 0; i < sizeof measured / sizeof measured[0]; ++i)
                if (sample(dir, measured[i], &capability, run) != 0)
                {
                    result = 1;
                    break;
                }
        if (result == 0 && capability == 1 && explicit_offset_fixture(dir) != 0)
            result = 1;
        if (close(dir) != 0)
            result = 1;
    }
    if (rmdir(root) != 0)
        result = 1;
    return result ? result : capability == 77 ? 77 : 0;
}
