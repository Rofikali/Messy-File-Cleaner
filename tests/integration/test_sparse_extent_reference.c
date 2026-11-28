#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
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
    struct stat src_stat, dst_stat;
    if (size < 0 || size > SIZE ||
        fstat(src, &src_stat) || fstat(dst, &dst_stat) ||
        !S_ISREG(src_stat.st_mode) || !S_ISREG(dst_stat.st_mode) ||
        src_stat.st_size != size || dst_stat.st_size != 0 ||
        (src_stat.st_dev == dst_stat.st_dev && src_stat.st_ino == dst_stat.st_ino) ||
        ftruncate(dst, size)) return 1;
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

/* Timing includes destination sizing/copy/sync, excludes fixture creation and oracle. */
static int measure_copy(int src, int dst, int sparse, off_t *written, double *ms)
{
    struct timespec start, end;
    unsigned char buf[CHUNK];
    if (clock_gettime(CLOCK_MONOTONIC, &start)) return 1;
    if (sparse) {
        int rc = copy_extents(src, dst, SIZE, written);
        if (rc) return rc;
    } else {
        *written = 0;
        if (ftruncate(dst, SIZE)) return 1;
        for (off_t pos = 0; pos < SIZE; pos += CHUNK) {
            if (pread(src, buf, sizeof buf, pos) != (ssize_t)sizeof buf ||
                pwrite(dst, buf, sizeof buf, pos) != (ssize_t)sizeof buf) return 1;
            *written += CHUNK;
        }
    }
    if (fsync(dst) || clock_gettime(CLOCK_MONOTONIC, &end)) return 1;
    *ms = ((double)end.tv_sec - (double)start.tv_sec) * 1000.0 +
          ((double)end.tv_nsec - (double)start.tv_nsec) / 1000000.0;
    return *ms < 0.0 ? 1 : 0;
}

static int measurement_trial(unsigned trial)
{
    FILE *files[3] = {tmpfile(), tmpfile(), tmpfile()};
    int rc = 1, fd[3];
    struct stat st[3];
    off_t bytes[2] = {0, 0};
    double ms[2] = {0.0, 0.0};
    unsigned char buf[3][CHUNK];
    for (unsigned i = 0; i < 3; ++i) {
        if (!files[i] || (fd[i] = fileno(files[i])) < 0 ||
            fstat(fd[i], &st[i]) || !S_ISREG(st[i].st_mode) ||
            st[i].st_size != 0) goto done;
        for (unsigned j = 0; j < i; ++j)
            if (st[i].st_dev == st[j].st_dev && st[i].st_ino == st[j].st_ino)
                goto done;
    }
    if (ftruncate(fd[0], SIZE) || pwrite(fd[0], "A", 1, CHUNK) != 1 ||
        pwrite(fd[0], "Z", 1, SECOND + 1) != 1 || fsync(fd[0])) goto done;
    /* Alternate order; cache state is uncontrolled, not a cold-storage benchmark. */
    for (unsigned i = 0; i < 2; ++i) {
        unsigned k = (i + trial) % 2;
        rc = measure_copy(fd[0], fd[k + 1], k == 0, &bytes[k], &ms[k]);
        if (rc) goto done;
    }
    rc = 1;
    for (off_t pos = 0; pos < SIZE; pos += CHUNK) {
        for (unsigned i = 0; i < 3; ++i)
            if (pread(fd[i], buf[i], CHUNK, pos) != CHUNK) goto done;
        if (memcmp(buf[0], buf[1], CHUNK) || memcmp(buf[0], buf[2], CHUNK))
            goto done;
        for (size_t i = 0; i < CHUNK; ++i) {
            off_t off = pos + (off_t)i;
            unsigned char expected = off == CHUNK ? 'A' : off == SECOND + 1 ? 'Z' : 0;
            if (buf[0][i] != expected) goto done;
        }
    }
    for (unsigned i = 0; i < 3; ++i) {
        struct stat now;
        if (fstat(fd[i], &now) || now.st_dev != st[i].st_dev ||
            now.st_ino != st[i].st_ino || now.st_size != SIZE) goto done;
        st[i] = now;
    }
    if (bytes[0] <= 0 || bytes[0] > SIZE || bytes[1] != SIZE) goto done;
    printf("trial=%u order=%s logical_bytes=%d extent_bytes=%jd buffered_bytes=%jd extent_blocks_512=%jd buffered_blocks_512=%jd extent_ms=%.3f buffered_ms=%.3f\n",
           trial + 1, trial % 2 ? "buffered-first" : "extent-first", SIZE,
           (intmax_t)bytes[0], (intmax_t)bytes[1],
           (intmax_t)st[1].st_blocks, (intmax_t)st[2].st_blocks, ms[0], ms[1]);
    rc = bytes[0] == SIZE ? 77 : 0;
done:
    for (unsigned i = 0; i < 3; ++i)
        if (files[i] && fclose(files[i])) rc = 1;
    return rc;
}

static int measurement_reference(void)
{
    for (unsigned i = 0; i < 4; ++i) {
        int rc = measurement_trial(i);
        if (rc) {
            if (rc == 77) puts("NOT VERIFIED: sparse measurement capability unavailable");
            return rc;
        }
    }
    return 0;
}

/* Rejection is tested before any destination sizing or copying. */
static int copy_rejections(void)
{
    FILE *source = tmpfile(), *dest = tmpfile(), *empty = tmpfile();
    int rc = 1;
    off_t written = -1;
    char a = 0, b = 0;
    struct stat ss, ds, es;
    if (!source || !dest || !empty) goto done;
    int s = fileno(source), d = fileno(dest), e = fileno(empty);
    if (s < 0 || d < 0 || e < 0 ||
        pwrite(s, "A", 1, 0) != 1 || pwrite(d, "B", 1, 0) != 1)
        goto done;
    if (copy_extents(s, d, 1, &written) != 1 || written != 0 ||
        copy_extents(s, s, 1, &written) != 1 || written != 0 ||
        copy_extents(e, e, 0, &written) != 1 || written != 0 ||
        copy_extents(s, e, 0, &written) != 1 || written != 0 ||
        copy_extents(s, -1, 1, &written) != 1 || written != 0 ||
        copy_extents(-1, e, 0, &written) != 1 || written != 0)
        goto done;
    if (fstat(s, &ss) || fstat(d, &ds) || fstat(e, &es) ||
        ss.st_size != 1 || ds.st_size != 1 || es.st_size != 0 ||
        pread(s, &a, 1, 0) != 1 || a != 'A' ||
        pread(d, &b, 1, 0) != 1 || b != 'B') goto done;
    rc = 0;
done:
    if (source && fclose(source)) rc = 1;
    if (dest && fclose(dest)) rc = 1;
    if (empty && fclose(empty)) rc = 1;
    return rc;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--reject") == 0) return copy_rejections();
    if (argc == 2 && strcmp(argv[1], "--measure") == 0) return measurement_reference();
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
