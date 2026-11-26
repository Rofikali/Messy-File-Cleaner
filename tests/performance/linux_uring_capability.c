#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#ifdef FC_HAVE_URING_PROBE
#include <linux/io_uring.h>
#include <sys/syscall.h>

/* Test-only capability discovery. No submission, mappings or file I/O. */
enum { PROBE_OPS = 64 };

static int supports(const struct io_uring_probe *probe, unsigned int op)
{
    for (unsigned int i = 0; i < probe->ops_len; ++i)
        if (probe->ops[i].op == op)
            return (probe->ops[i].flags & IO_URING_OP_SUPPORTED) != 0;
    return 0;
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
