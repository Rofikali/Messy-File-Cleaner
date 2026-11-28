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

int main(void)
{
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
