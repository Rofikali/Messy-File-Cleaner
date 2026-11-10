#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "fcj4_test_append.h"
#include "journal/fcj4_parser.h"

#include <errno.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

typedef enum {
    APPEND_READY = 0,
    APPEND_WRITING,
    APPEND_DURABLE_PENDING,
    APPEND_POISONED
} AppendState;

struct Fcj4TestAppend {
    int fd;
    dev_t device;
    ino_t inode;
    uint64_t known_length;
    AppendState state;
    atomic_flag busy;
    Fcj4TestAppendOps ops;
    char pending[FCJ4_MAX_LINE_BYTES];
    size_t pending_length;
    Fcj4TestAppendResultCode last_code;
    int primary_errno;
};

static ssize_t real_pwrite(void *user, int fd, const void *buf,
                           size_t len, off_t offset)
{
    (void)user;
    return pwrite(fd,buf,len,offset);
}
static int real_fsync(void *user, int fd)
{
    (void)user;
    return fsync(fd);
}
static int real_close(void *user, int fd)
{
    (void)user;
    return close(fd);
}
static int real_flock(void *user, int fd, int operation)
{
    (void)user;
    return flock(fd,operation);
}
static int real_fstat(void *user, int fd, struct stat *out)
{
    (void)user;
    return fstat(fd,out);
}
static int real_getfl(void *user, int fd)
{
    (void)user;
    return fcntl(fd,F_GETFL);
}
static int real_getfd(void *user, int fd)
{
    (void)user;
    return fcntl(fd,F_GETFD);
}

static Fcj4TestAppendOps default_ops(void)
{
    Fcj4TestAppendOps ops;
    ops.pwrite_fn=real_pwrite;
    ops.fsync_fn=real_fsync;
    ops.close_fn=real_close;
    ops.flock_fn=real_flock;
    ops.fstat_fn=real_fstat;
    ops.getfl_fn=real_getfl;
    ops.getfd_fn=real_getfd;
    ops.user=NULL;
    return ops;
}

static int complete_ops(const Fcj4TestAppendOps *ops)
{
    return ops && ops->pwrite_fn && ops->fsync_fn && ops->close_fn &&
           ops->flock_fn && ops->fstat_fn && ops->getfl_fn && ops->getfd_fn;
}

static int fail_with(int error)
{
    errno=error;
    return -1;
}

static void set_result(Fcj4TestAppendResult *out,
                       Fcj4TestAppendResultCode code,
                       int primary_errno, int cleanup_errno,
                       uint64_t durable_length)
{
    if (!out) return;
    out->code=code;
    out->primary_errno=primary_errno;
    out->cleanup_errno=cleanup_errno;
    out->durable_length=durable_length;
}

static int validate_line(const char *bytes, size_t length)
{
    if (!bytes || length==0U || length>FCJ4_MAX_LINE_BYTES)
        return 0;
    if (length<6U || memcmp(bytes,"FCJ4|",5U)!=0 || bytes[length-1U]!='\n')
        return 0;
    if (memchr(bytes,'\0',length)!=NULL ||
        memchr(bytes,'\r',length)!=NULL ||
        memchr(bytes,'\n',length-1U)!=NULL)
        return 0;
    return 1;
}

static int verify_descriptor(Fcj4TestAppend *ctx, int check_size)
{
    struct stat st;
    int flags, fdflags;
    if (ctx->ops.fstat_fn(ctx->ops.user,ctx->fd,&st)!=0)
        return -1;
    if (!S_ISREG(st.st_mode) || st.st_dev!=ctx->device ||
        st.st_ino!=ctx->inode || st.st_size<0)
        return fail_with(ESTALE);
    if (check_size && (uint64_t)st.st_size!=ctx->known_length)
        return fail_with(ESTALE);
    flags=ctx->ops.getfl_fn(ctx->ops.user,ctx->fd);
    if (flags<0) return -1;
    if ((flags&O_APPEND)!=0) return fail_with(EINVAL);
    fdflags=ctx->ops.getfd_fn(ctx->ops.user,ctx->fd);
    if (fdflags<0) return -1;
    if ((fdflags&FD_CLOEXEC)==0) return fail_with(EINVAL);
    return 0;
}

int fcj4_test_append_init(Fcj4TestAppend **out, int fd,
                          dev_t expected_dev, ino_t expected_ino,
                          uint64_t expected_length,
                          const Fcj4TestAppendOps *provided_ops)
{
    Fcj4TestAppendOps ops;
    Fcj4TestAppend *ctx;
    struct stat st;
    int locked=0, saved=0, flags, fdflags;

    if (!out || fd<0 || expected_ino==0)
        return fail_with(EINVAL);
    *out=NULL;
    if (provided_ops) {
        if (!complete_ops(provided_ops)) return fail_with(EINVAL);
        ops=*provided_ops;
    } else {
        ops=default_ops();
    }

    ctx=(Fcj4TestAppend *)calloc(1U,sizeof(*ctx));
    if (!ctx) return -1;
    ctx->fd=fd;
    ctx->device=expected_dev;
    ctx->inode=expected_ino;
    ctx->known_length=expected_length;
    ctx->state=APPEND_READY;
    ctx->ops=ops;
    ctx->last_code=FCJ4_TEST_APPEND_REJECTED;
    atomic_flag_clear_explicit(&ctx->busy,memory_order_release);

    if (ctx->ops.flock_fn(ctx->ops.user,fd,LOCK_EX|LOCK_NB)!=0) {
        saved=errno;
        free(ctx);
        return fail_with(saved);
    }
    locked=1;

    if (ctx->ops.fstat_fn(ctx->ops.user,fd,&st)!=0) saved=errno;
    else if (!S_ISREG(st.st_mode) || st.st_dev!=expected_dev ||
             st.st_ino!=expected_ino || st.st_size<0 ||
             (uint64_t)st.st_size!=expected_length) saved=EINVAL;

    if (saved==0) {
        flags=ctx->ops.getfl_fn(ctx->ops.user,fd);
        if (flags<0) saved=errno;
        else if ((flags&O_APPEND)!=0) saved=EINVAL;
    }
    if (saved==0) {
        fdflags=ctx->ops.getfd_fn(ctx->ops.user,fd);
        if (fdflags<0) saved=errno;
        else if ((fdflags&FD_CLOEXEC)==0) saved=EINVAL;
    }

    if (saved!=0) {
        if (locked) {
            int original=saved;
            (void)ctx->ops.flock_fn(ctx->ops.user,fd,LOCK_UN);
            saved=original;
        }
        free(ctx);
        return fail_with(saved);
    }

    *out=ctx;
    return 0;
}

int fcj4_test_append_record(Fcj4TestAppend *ctx, const char *bytes,
                            size_t length, uint64_t expected_offset,
                            Fcj4TestAppendResult *result)
{
    size_t done=0U;
    int error=0;

    set_result(result,FCJ4_TEST_APPEND_REJECTED,0,0,
               ctx ? ctx->known_length : 0U);
    if (!ctx || !result || !validate_line(bytes,length))
        return fail_with(EINVAL);
    if (atomic_flag_test_and_set_explicit(&ctx->busy,memory_order_acquire))
        return fail_with(EBUSY);

    if (ctx->state!=APPEND_READY) {
        atomic_flag_clear_explicit(&ctx->busy,memory_order_release);
        return fail_with(ctx->state==APPEND_POISONED ? EIO : EBUSY);
    }
    if (expected_offset!=ctx->known_length ||
        (uint64_t)length>UINT64_MAX-ctx->known_length ||
        expected_offset>(uint64_t)INT64_MAX-(uint64_t)length) {
        atomic_flag_clear_explicit(&ctx->busy,memory_order_release);
        return fail_with(EINVAL);
    }
    if (verify_descriptor(ctx,1)!=0) {
        error=errno;
        if (error==ESTALE || error==EBADF || error==EIO)
            ctx->state=APPEND_POISONED;
        ctx->primary_errno=error;
        atomic_flag_clear_explicit(&ctx->busy,memory_order_release);
        set_result(result,FCJ4_TEST_APPEND_REJECTED,error,0,
                   ctx->known_length);
        return fail_with(error);
    }

    ctx->state=APPEND_WRITING;
    while (done<length) {
        ssize_t n=ctx->ops.pwrite_fn(ctx->ops.user,ctx->fd,bytes+done,
                                    length-done,(off_t)(expected_offset+done));
        if (n<0) {
            if (errno==EINTR) continue;
            error=errno ? errno : EIO;
            ctx->state=APPEND_POISONED;
            ctx->primary_errno=error;
            ctx->last_code=FCJ4_TEST_APPEND_UNCERTAIN_WRITE;
            atomic_flag_clear_explicit(&ctx->busy,memory_order_release);
            set_result(result,ctx->last_code,error,0,ctx->known_length);
            return fail_with(error);
        }
        if (n==0) {
            error=EIO;
            ctx->state=APPEND_POISONED;
            ctx->primary_errno=error;
            ctx->last_code=FCJ4_TEST_APPEND_UNCERTAIN_WRITE;
            atomic_flag_clear_explicit(&ctx->busy,memory_order_release);
            set_result(result,ctx->last_code,error,0,ctx->known_length);
            return fail_with(error);
        }
        if ((size_t)n>length-done) {
            error=EIO;
            ctx->state=APPEND_POISONED;
            ctx->primary_errno=error;
            ctx->last_code=FCJ4_TEST_APPEND_UNCERTAIN_WRITE;
            atomic_flag_clear_explicit(&ctx->busy,memory_order_release);
            set_result(result,ctx->last_code,error,0,ctx->known_length);
            return fail_with(error);
        }
        done+=(size_t)n;
    }

    if (ctx->ops.fsync_fn(ctx->ops.user,ctx->fd)!=0) {
        error=errno ? errno : EIO;
        ctx->state=APPEND_POISONED;
        ctx->primary_errno=error;
        ctx->last_code=FCJ4_TEST_APPEND_UNCERTAIN_FSYNC;
        atomic_flag_clear_explicit(&ctx->busy,memory_order_release);
        set_result(result,ctx->last_code,error,0,ctx->known_length);
        return fail_with(error);
    }

    memcpy(ctx->pending,bytes,length);
    ctx->pending_length=length;
    ctx->known_length+=(uint64_t)length;
    ctx->state=APPEND_DURABLE_PENDING;
    ctx->last_code=FCJ4_TEST_APPEND_DURABLE;
    atomic_flag_clear_explicit(&ctx->busy,memory_order_release);
    set_result(result,FCJ4_TEST_APPEND_DURABLE,0,0,ctx->known_length);
    return 0;
}

int fcj4_test_append_ack_commit(Fcj4TestAppend *ctx, const char *bytes,
                                size_t length)
{
    if (!ctx || !bytes) return fail_with(EINVAL);
    if (atomic_flag_test_and_set_explicit(&ctx->busy,memory_order_acquire))
        return fail_with(EBUSY);
    if (ctx->state!=APPEND_DURABLE_PENDING ||
        length!=ctx->pending_length ||
        memcmp(bytes,ctx->pending,length)!=0) {
        ctx->state=APPEND_POISONED;
        ctx->primary_errno=EINVAL;
        ctx->last_code=FCJ4_TEST_APPEND_REJECTED;
        atomic_flag_clear_explicit(&ctx->busy,memory_order_release);
        return fail_with(EINVAL);
    }
    memset(ctx->pending,0,ctx->pending_length);
    ctx->pending_length=0U;
    ctx->state=APPEND_READY;
    atomic_flag_clear_explicit(&ctx->busy,memory_order_release);
    return 0;
}

void fcj4_test_append_poison(Fcj4TestAppend *ctx)
{
    if (!ctx) return;
    if (atomic_flag_test_and_set_explicit(&ctx->busy,memory_order_acquire))
        return;
    ctx->state=APPEND_POISONED;
    if (ctx->primary_errno==0) ctx->primary_errno=EIO;
    atomic_flag_clear_explicit(&ctx->busy,memory_order_release);
}

int fcj4_test_append_destroy(Fcj4TestAppend *ctx,
                             Fcj4TestAppendResult *result)
{
    int cleanup_error=0, primary_error, unlock_result, close_result;
    Fcj4TestAppendResultCode code;
    uint64_t durable_length;
    if (!ctx || !result) return fail_with(EINVAL);

    primary_error=ctx->primary_errno;
    code=ctx->last_code;
    durable_length=ctx->known_length;

    unlock_result=ctx->ops.flock_fn(ctx->ops.user,ctx->fd,LOCK_UN);
    if (unlock_result!=0) cleanup_error=errno ? errno : EIO;

    close_result=ctx->ops.close_fn(ctx->ops.user,ctx->fd);
    ctx->fd=-1;
    if (close_result!=0 && cleanup_error==0)
        cleanup_error=errno ? errno : EIO;

    set_result(result,cleanup_error ? FCJ4_TEST_APPEND_CLEANUP_ERROR : code,
               primary_error,cleanup_error,durable_length);
    memset(ctx->pending,0,sizeof(ctx->pending));
    free(ctx);
    if (cleanup_error) return fail_with(cleanup_error);
    return 0;
}
