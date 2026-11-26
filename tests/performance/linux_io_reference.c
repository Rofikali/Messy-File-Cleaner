#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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

static int sample(int dir, size_t size, int *capability)
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
    if (populate(in, size) != 0 || lseek(in, 0, SEEK_SET) != 0 ||
        buffered(in, baseline) != 0 || verify(baseline, size) != 0 ||
        verify(in, size) != 0 || lseek(in, 0, SEEK_SET) != 0)
        goto cleanup;
    if (*capability != 77)
    {
        int rc = ranged(in, candidate, size, *capability == 0);
        if (rc == 77)
            *capability = 77;
        else
        {
            if (rc != 0 || lseek(in, 0, SEEK_CUR) != (off_t)size ||
                lseek(candidate, 0, SEEK_CUR) != (off_t)size ||
                verify(candidate, size) != 0 || verify(in, size) != 0)
                goto cleanup;
            if (size != 0)
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
            if (sample(dir, sizes[i], &capability) != 0)
            {
                result = 1;
                break;
            }
        if (close(dir) != 0)
            result = 1;
    }
    if (rmdir(root) != 0)
        result = 1;
    return result ? result : capability == 77 ? 77 : 0;
}
