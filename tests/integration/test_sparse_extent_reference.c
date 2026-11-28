#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

enum { SIZE = 4 * 1024 * 1024, SECOND = 3 * 1024 * 1024, CHUNK = 4096 };

static int unsupported(int err)
{
    return err == EINVAL || err == EOPNOTSUPP || err == ENOSYS;
}

static int probe(int fd)
{
    off_t pos = 0;
    int first = 0, second = 0, gap = 0;
    for (unsigned i = 0; pos < SIZE && i < 1024; ++i) {
        off_t data = lseek(fd, pos, SEEK_DATA);
        if (data < 0) {
            if (unsupported(errno)) return 77;
            if (errno == ENXIO && first && second) {
                gap = 1;
                pos = SIZE;
                break;
            }
            return 1;
        }
        if (data < pos || data >= SIZE) return 1;
        if (data > pos) gap = 1;
        off_t hole = lseek(fd, data, SEEK_HOLE);
        if (hole < 0) return unsupported(errno) ? 77 : 1;
        if (hole <= data || hole > SIZE) return 1;
        if (data == 0) first = 1;
        if (data <= SECOND && hole > SECOND) second = 1;
        pos = hole;
    }
    if (pos != SIZE || !first || !second) return 1;
    return gap ? 0 : 77;
}

/* Independent boundary control; no mutation outside an owned tmpfile. */
static int expect_enxio(int fd, off_t offset, int whence)
{
    errno = 0;
    if (lseek(fd, offset, whence) != (off_t)-1) return 1;
    if (errno == ENXIO) return 0;
    return unsupported(errno) ? 77 : 1;
}

static int boundaries(void)
{
    FILE *file = tmpfile();
    int result = 1;
    if (!file) return 1;
    int fd = fileno(file);
    if (fd < 0) goto done;
    errno = 0;
    if (lseek(-1, 0, SEEK_DATA) != (off_t)-1 || errno != EBADF) goto done;
    const int modes[] = { SEEK_DATA, SEEK_HOLE };
    for (size_t i = 0; i < sizeof modes / sizeof modes[0]; ++i) {
        result = expect_enxio(fd, 0, modes[i]);
        if (result) goto done;
    }
    result = 1;
    if (ftruncate(fd, 2 * CHUNK) || fsync(fd)) goto done;
    for (size_t i = 0; i < sizeof modes / sizeof modes[0]; ++i) {
        result = expect_enxio(fd, 2 * CHUNK, modes[i]);
        if (result) goto done;
        result = expect_enxio(fd, 2 * CHUNK + 1, modes[i]);
        if (result) goto done;
    }
    result = 1;
    unsigned char buf[CHUNK];
    for (off_t pos = 0; pos < 2 * CHUNK; pos += CHUNK) {
        if (pread(fd, buf, sizeof buf, pos) != (ssize_t)sizeof buf) goto done;
        for (size_t i = 0; i < sizeof buf; ++i)
            if (buf[i] != 0) goto done;
    }
    off_t hole = lseek(fd, 0, SEEK_HOLE);
    if (hole < 0) { result = unsupported(errno) ? 77 : 1; goto done; }
    if (hole > 2 * CHUNK) goto done;
    off_t data = lseek(fd, 0, SEEK_DATA);
    if (data < 0) {
        if (unsupported(errno)) { result = 77; goto done; }
        if (errno != ENXIO || hole != 0) goto done;
    } else if (data >= 2 * CHUNK || (hole == 2 * CHUNK && data != 0)) {
        goto done;
    }
    struct stat st;
    if (fstat(fd, &st) || st.st_size != 2 * CHUNK) goto done;
    /* A filesystem may conservatively report all bytes as data. */
    result = hole == 2 * CHUNK ? 77 : 0;
done:
    if (fclose(file)) result = 1;
    if (result == 77) puts("NOT VERIFIED: extent boundary capability unavailable");
    return result;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--boundaries") == 0) return boundaries();
    if (argc != 1) return 1;
    FILE *sparse = tmpfile(), *dense = tmpfile();
    int result = 1;
    if (!sparse || !dense) goto cleanup;
    int s = fileno(sparse), d = fileno(dense);
    if (ftruncate(s, SIZE) || pwrite(s, "A", 1, 0) != 1 ||
        pwrite(s, "Z", 1, SECOND) != 1) goto cleanup;
    unsigned char a[CHUNK], b[CHUNK];
    for (off_t pos = 0; pos < SIZE; pos += CHUNK) {
        if (pread(s, a, sizeof a, pos) != (ssize_t)sizeof a) goto cleanup;
        for (size_t i = 0; i < sizeof a; ++i) {
            off_t offset = pos + (off_t)i;
            unsigned char expected = offset == 0 ? 'A' : offset == SECOND ? 'Z' : 0;
            if (a[i] != expected) goto cleanup;
        }
        if (pwrite(d, a, sizeof a, pos) != (ssize_t)sizeof a ||
            pread(d, b, sizeof b, pos) != (ssize_t)sizeof b ||
            memcmp(a, b, sizeof a)) goto cleanup;
    }
    if (fsync(s) || fsync(d)) goto cleanup;
    struct stat ss, ds;
    if (fstat(s, &ss) || fstat(d, &ds) ||
        ss.st_size != SIZE || ds.st_size != SIZE) goto cleanup;
    printf("logical_bytes=%d sparse_blocks_512=%jd buffered_blocks_512=%jd\n",
           SIZE, (intmax_t)ss.st_blocks, (intmax_t)ds.st_blocks);
    result = probe(s);
    if (result == 77) puts("NOT VERIFIED: extent support or observable hole unavailable");
cleanup:
    if (sparse && fclose(sparse)) result = 1;
    if (dense && fclose(dense)) result = 1;
    return result;
}
