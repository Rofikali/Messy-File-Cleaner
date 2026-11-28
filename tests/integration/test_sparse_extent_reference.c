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

/* Private reference only. Destination must be a fresh owned empty file. */
static int copy_extents(int src, int dst, off_t size, off_t *written)
{
    off_t pos = 0;
    unsigned char buf[CHUNK];
    *written = 0;
    if (size < 0 || size > SIZE || ftruncate(dst, size)) return 1;
    for (unsigned i = 0; pos < size && i < 1024; ++i) {
        off_t data = lseek(src, pos, SEEK_DATA);
        if (data < 0) {
            if (errno == ENXIO) return 0;
            return unsupported(errno) ? 77 : 1;
        }
        if (data < pos || data >= size) return 1;
        off_t hole = lseek(src, data, SEEK_HOLE);
        if (hole < 0) return unsupported(errno) ? 77 : 1;
        if (hole <= data || hole > size) return 1;
        while (data < hole) {
            size_t n = hole - data > CHUNK ? CHUNK : (size_t)(hole - data);
            if (pread(src, buf, n, data) != (ssize_t)n ||
                pwrite(dst, buf, n, data) != (ssize_t)n) return 1;
            data += (off_t)n;
            *written += (off_t)n;
        }
        pos = hole;
    }
    return pos == size ? 0 : 1;
}

static int copy_reference(void)
{
    FILE *source = tmpfile(), *copy = tmpfile(), *dense = tmpfile();
    int result = 1;
    if (!source || !copy || !dense) goto done;
    int s = fileno(source), c = fileno(copy), d = fileno(dense);
    if (s < 0 || c < 0 || d < 0 ||
        ftruncate(s, SIZE) ||
        pwrite(s, "A", 1, CHUNK) != 1 ||
        pwrite(s, "Z", 1, SECOND + 1) != 1 || fsync(s)) goto done;
    struct stat ss, cs, ds, after;
    if (fstat(s, &ss) || fstat(c, &cs) || fstat(d, &ds) ||
        !S_ISREG(ss.st_mode) || !S_ISREG(cs.st_mode) ||
        !S_ISREG(ds.st_mode) || cs.st_size != 0 || ds.st_size != 0 ||
        (ss.st_dev == cs.st_dev && ss.st_ino == cs.st_ino) ||
        (ss.st_dev == ds.st_dev && ss.st_ino == ds.st_ino) ||
        (cs.st_dev == ds.st_dev && cs.st_ino == ds.st_ino)) goto done;
    off_t written = 0;
    result = copy_extents(s, c, SIZE, &written);
    if (result) goto done;
    result = 1;
    unsigned char a[CHUNK], b[CHUNK], z[CHUNK];
    for (off_t pos = 0; pos < SIZE; pos += CHUNK) {
        if (pread(s, a, sizeof a, pos) != (ssize_t)sizeof a ||
            pread(c, b, sizeof b, pos) != (ssize_t)sizeof b ||
            pwrite(d, a, sizeof a, pos) != (ssize_t)sizeof a ||
            pread(d, z, sizeof z, pos) != (ssize_t)sizeof z ||
            memcmp(a, b, sizeof a) || memcmp(a, z, sizeof a)) goto done;
        for (size_t i = 0; i < sizeof a; ++i) {
            off_t offset = pos + (off_t)i;
            unsigned char expected = offset == CHUNK ? 'A' :
                offset == SECOND + 1 ? 'Z' : 0;
            if (a[i] != expected) goto done;
        }
    }
    if (fsync(c) || fsync(d) || fstat(c, &cs) || fstat(d, &ds) ||
        fstat(s, &after) || ss.st_dev != after.st_dev ||
        ss.st_ino != after.st_ino || after.st_size != SIZE ||
        cs.st_size != SIZE || ds.st_size != SIZE ||
        written <= 0 || written > SIZE) goto done;
    printf("logical_bytes=%d extent_bytes_written=%jd buffered_bytes_written=%d source_blocks_512=%jd extent_blocks_512=%jd buffered_blocks_512=%jd\n",
           SIZE, (intmax_t)written, SIZE, (intmax_t)after.st_blocks,
           (intmax_t)cs.st_blocks, (intmax_t)ds.st_blocks);
    result = written == SIZE ? 77 : 0;
done:
    if (source && fclose(source)) result = 1;
    if (copy && fclose(copy)) result = 1;
    if (dense && fclose(dense)) result = 1;
    if (result == 77) puts("NOT VERIFIED: sparse extent copying capability unavailable");
    return result;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--copy") == 0) return copy_reference();
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
