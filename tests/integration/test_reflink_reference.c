#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/fs.h>
#include <sys/ioctl.h>

#ifndef FICLONE
int main(void)
{
    puts("NOT VERIFIED: installed headers lack FICLONE");
    return 77;
}
#else
enum { CHUNK = 4096, SIZE = 16 * CHUNK };

static int verify(int fd, unsigned char first, unsigned char last)
{
    unsigned char buf[CHUNK];
    for (off_t pos = 0; pos < SIZE; pos += CHUNK) {
        if (pread(fd, buf, sizeof buf, pos) != (ssize_t)sizeof buf) return 1;
        for (size_t i = 0; i < sizeof buf; ++i) {
            off_t off = pos + (off_t)i;
            unsigned char expected = off == 0 ? first :
                off == SIZE - 1 ? last : (unsigned char)(off % 251);
            if (buf[i] != expected) return 1;
        }
    }
    return 0;
}

int main(void)
{
    FILE *source = tmpfile(), *dest = tmpfile();
    int rc = 1;
    if (!source || !dest) goto done;
    int s = fileno(source), d = fileno(dest);
    struct stat ss, ds, sa, da;
    if (s < 0 || d < 0 || fstat(s, &ss) || fstat(d, &ds) ||
        !S_ISREG(ss.st_mode) || !S_ISREG(ds.st_mode) ||
        ss.st_size != 0 || ds.st_size != 0 || ss.st_dev != ds.st_dev ||
        ss.st_ino == ds.st_ino) goto done;
    unsigned char buf[CHUNK];
    for (off_t pos = 0; pos < SIZE; pos += CHUNK) {
        for (size_t i = 0; i < sizeof buf; ++i)
            buf[i] = (unsigned char)((pos + (off_t)i) % 251);
        if (pwrite(s, buf, sizeof buf, pos) != (ssize_t)sizeof buf) goto done;
    }
    if (fsync(s) || verify(s, 0, (SIZE - 1) % 251)) goto done;
    if (ioctl(d, FICLONE, s) != 0) {
        int err = errno;
        fprintf(stderr, "FICLONE errno=%d (%s)\n", err, strerror(err));
        if (err == EOPNOTSUPP || err == ENOTTY || err == ENOSYS || err == EXDEV) {
            if (verify(s, 0, (SIZE - 1) % 251)) goto done;
            puts("NOT VERIFIED: clone capability unavailable");
            rc = 77;
        }
        goto done;
    }
    if (fsync(d) || verify(d, 0, (SIZE - 1) % 251) ||
        verify(s, 0, (SIZE - 1) % 251)) goto done;
    /* Mutate only disposable files; prove independence in both directions. */
    if (pwrite(d, "X", 1, 0) != 1 || fsync(d) ||
        verify(d, 'X', (SIZE - 1) % 251) ||
        verify(s, 0, (SIZE - 1) % 251) ||
        pwrite(s, "Y", 1, SIZE - 1) != 1 || fsync(s) ||
        verify(s, 0, 'Y') || verify(d, 'X', (SIZE - 1) % 251))
        goto done;
    if (fstat(s, &sa) || fstat(d, &da) ||
        sa.st_dev != ss.st_dev || sa.st_ino != ss.st_ino ||
        da.st_dev != ds.st_dev || da.st_ino != ds.st_ino ||
        sa.st_size != SIZE || da.st_size != SIZE) goto done;
    printf("FICLONE logical_bytes=%d bidirectional_isolation=PASS\n", SIZE);
    rc = 0;
done:
    if (source && fclose(source)) rc = 1;
    if (dest && fclose(dest)) rc = 1;
    return rc;
}
#endif
