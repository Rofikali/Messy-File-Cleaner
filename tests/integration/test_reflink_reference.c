#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/vfs.h>
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

/* Narrow capability policy: other errors are failures, never silent fallback. */
static int fallback_allowed(int err)
{
    return err == EOPNOTSUPP || err == ENOTTY || err == ENOSYS || err == EXDEV;
}

static int error_policy_reference(void)
{
    const int allowed[] = { EOPNOTSUPP, ENOTTY, ENOSYS, EXDEV };
    const int denied[] = { 0, EBADF, EINVAL, EIO, ENOSPC, EDQUOT,
                           EACCES, EPERM, EINTR, ENOMEM, EFBIG, EROFS };
    for (size_t i = 0; i < sizeof allowed / sizeof allowed[0]; ++i)
        if (!fallback_allowed(allowed[i])) return 1;
    for (size_t i = 0; i < sizeof denied / sizeof denied[0]; ++i)
        if (fallback_allowed(denied[i])) return 1;
    FILE *source = tmpfile();
    if (!source) return 1;
    int rc = 1, fd = fileno(source);
    struct stat before, after;
    char byte = 0;
    if (fd < 0 || pwrite(fd, "S", 1, 0) != 1 ||
        fsync(fd) || fstat(fd, &before)) goto done;
    /* Actual invalid destination descriptor: no write or fallback permitted. */
    errno = 0;
    if (ioctl(-1, FICLONE, fd) != -1) goto done;
    int err = errno;
    if (err != EBADF || fallback_allowed(err) ||
        fstat(fd, &after) || before.st_dev != after.st_dev ||
        before.st_ino != after.st_ino || before.st_size != 1 ||
        after.st_size != 1 || pread(fd, &byte, 1, 0) != 1 ||
        byte != 'S') goto done;
    puts("policy_cases=16 actual_invalid_destination=EBADF source_preserved=YES");
    rc = 0;
done:
    if (fclose(source)) rc = 1;
    return rc;
}

static int buffered_copy(int src, int dst)
{
    unsigned char buf[CHUNK];
    for (off_t pos = 0; pos < SIZE; pos += CHUNK) {
        if (pread(src, buf, sizeof buf, pos) != (ssize_t)sizeof buf ||
            pwrite(dst, buf, sizeof buf, pos) != (ssize_t)sizeof buf) return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--errors") == 0) return error_policy_reference();
    int fallback = argc == 2 && strcmp(argv[1], "--fallback") == 0;
    if (argc != 1 && !fallback) return 1;
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
    struct statfs fs;
    if (fstatfs(s, &fs)) goto done;
    printf("filesystem_magic=0x%lx\n", (unsigned long)fs.f_type);
    if (ioctl(d, FICLONE, s) != 0) {
        int err = errno;
        fprintf(stderr, "FICLONE errno=%d (%s)\n", err, strerror(err));
        if (fallback_allowed(err)) {
            if (verify(s, 0, (SIZE - 1) % 251)) goto done;
            if (!fallback) {
                puts("NOT VERIFIED: clone capability unavailable");
                rc = 77;
                goto done;
            }
            /* Discard the attempted destination rather than reuse possibly
             * modified contents. The replacement must be a fresh owned file. */
            FILE *fresh = tmpfile();
            if (!fresh) goto done;
            int closed = fclose(dest);
            dest = fresh;
            if (closed) goto done;
            d = fileno(dest);
            if (d < 0 || fstat(d, &ds) || !S_ISREG(ds.st_mode) ||
                ds.st_size != 0 ||
                (ds.st_dev == ss.st_dev && ds.st_ino == ss.st_ino) ||
                buffered_copy(s, d)) goto done;
            puts("fallback=buffered actual_unsupported_ioctl=YES");
        } else {
            goto done;
        }
    } else if (fallback) {
        puts("NOT VERIFIED: actual unsupported capability not observed");
        rc = 77;
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
    printf("method=%s logical_bytes=%d bidirectional_isolation=PASS\n",
           fallback ? "buffered-fallback" : "FICLONE", SIZE);
    rc = 0;
done:
    if (source && fclose(source)) rc = 1;
    if (dest && fclose(dest)) rc = 1;
    return rc;
}
#endif
