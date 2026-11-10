#define _POSIX_C_SOURCE 200809L

#include "fcj4_test_append.h"
#include "journal/fcj4_parser.h"
#include "journal/fcj4_writer.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr,"CHECK failed at %s:%d: %s (errno=%d)\n", \
                __FILE__,__LINE__,#expr,errno); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

typedef enum {
    MOCK_NORMAL = 0,
    MOCK_SHORT_EINTR,
    MOCK_PARTIAL_EIO,
    MOCK_ZERO_WRITE,
    MOCK_FSYNC_EIO
} MockMode;

typedef struct {
    MockMode mode;
    int pwrite_calls;
    int fsync_calls;
    int unlock_calls;
    int close_calls;
    bool fail_unlock;
    bool fail_close;
    bool bad_identity;
} MockIo;

static const char operation_id[] =
    "11111111111111111111111111111111";
static const char digest[] =
    "0000000000000000000000000000000000000000000000000000000000000000";

static Fcj4Record make_begin(void)
{
    Fcj4Record record;
    memset(&record,0,sizeof(record));
    record.kind=FCJ4_RECORD_BEGIN;
    record.value.begin=(Fcj4BeginRecord){
        operation_id,"/tmp/fcj4-source",1U,2U,1U,3U,1000U,1000U,
        384U,5U,1,2U,3,4U,"/tmp/fcj4-destination",1U,4U,
        "destination",1U,5U
    };
    return record;
}

static Fcj4Record make_sealed(void)
{
    Fcj4Record record;
    memset(&record,0,sizeof(record));
    record.kind=FCJ4_RECORD_SEALED;
    record.value.sealed=(Fcj4SealedRecord){
        operation_id,".fc-tmp-test",1U,6U,1000U,384U,5U,digest
    };
    return record;
}

static int make_temp_file(char path[64])
{
    int fd;
    (void)snprintf(path,64U,"/tmp/fcj4-test-append-XXXXXX");
    fd=mkstemp(path);
    CHECK(fd>=0);
    CHECK(fcntl(fd,F_SETFD,FD_CLOEXEC)==0);
    return fd;
}

static Fcj4TestAppend *init_adapter(int fd,const Fcj4TestAppendOps *ops,
                                    uint64_t length)
{
    struct stat st;
    Fcj4TestAppend *ctx=NULL;
    CHECK(fstat(fd,&st)==0);
    CHECK(fcj4_test_append_init(&ctx,fd,st.st_dev,st.st_ino,length,ops)==0);
    CHECK(ctx!=NULL);
    return ctx;
}

static void prepare_record(Fcj4WriterState *writer,const Fcj4Record *record,
                           char line[FCJ4_MAX_LINE_BYTES],size_t *length,
                           Fcj4PreparedRecord *prepared)
{
    CHECK(fcj4_writer_prepare(writer,record,line,FCJ4_MAX_LINE_BYTES,
                              length,prepared)==0);
    CHECK(*length>0U && line[*length-1U]=='\n');
}

static size_t read_journal(const char *path,char *buffer,size_t capacity)
{
    int fd=open(path,O_RDONLY|O_CLOEXEC);
    struct stat st;
    size_t used=0U;
    CHECK(fd>=0);
    CHECK(fstat(fd,&st)==0);
    CHECK(st.st_size>=0 && (uint64_t)st.st_size<=capacity);
    while(used<(size_t)st.st_size) {
        ssize_t n=read(fd,buffer+used,(size_t)st.st_size-used);
        CHECK(n>0);
        used+=(size_t)n;
    }
    CHECK(close(fd)==0);
    return used;
}

static void check_replay(const char *path,size_t expected_records)
{
    char bytes[FCJ4_MAX_LINE_BYTES*2U];
    size_t length=read_journal(path,bytes,sizeof(bytes));
    Fcj4Replay replay;
    fcj4_replay_init(&replay);
    CHECK(fcj4_replay_parse(bytes,length,&replay)==0);
    CHECK(replay.record_count==expected_records);
    fcj4_replay_destroy(&replay);
}

static ssize_t mock_pwrite(void *user,int fd,const void *buffer,
                           size_t length,off_t offset)
{
    MockIo *mock=(MockIo *)user;
    mock->pwrite_calls++;
    if(mock->mode==MOCK_SHORT_EINTR && mock->pwrite_calls==1) {
        errno=EINTR;
        return -1;
    }
    if(mock->mode==MOCK_ZERO_WRITE) return 0;
    if(mock->mode==MOCK_PARTIAL_EIO) {
        if(mock->pwrite_calls==1) {
            size_t amount=length<3U ? length : 3U;
            return pwrite(fd,buffer,amount,offset);
        }
        errno=EIO;
        return -1;
    }
    if(mock->mode==MOCK_SHORT_EINTR && length>3U)
        length=3U;
    return pwrite(fd,buffer,length,offset);
}

static int mock_fsync(void *user,int fd)
{
    MockIo *mock=(MockIo *)user;
    mock->fsync_calls++;
    if(mock->mode==MOCK_FSYNC_EIO) {
        errno=EIO;
        return -1;
    }
    return fsync(fd);
}

static int mock_close(void *user,int fd)
{
    MockIo *mock=(MockIo *)user;
    int rc;
    mock->close_calls++;
    rc=close(fd);
    if(mock->fail_close) {
        errno=EIO;
        return -1;
    }
    return rc;
}

static int mock_flock(void *user,int fd,int operation)
{
    MockIo *mock=(MockIo *)user;
    if(operation==LOCK_UN) {
        mock->unlock_calls++;
        if(mock->fail_unlock) {
            errno=EIO;
            return -1;
        }
    }
    return flock(fd,operation);
}

static int mock_fstat(void *user,int fd,struct stat *out)
{
    MockIo *mock=(MockIo *)user;
    int rc=fstat(fd,out);
    if(rc==0 && mock->bad_identity) out->st_dev=(dev_t)(out->st_dev+1);
    return rc;
}

static int mock_getfl(void *user,int fd)
{
    (void)user;
    return fcntl(fd,F_GETFL);
}

static int mock_getfd(void *user,int fd)
{
    (void)user;
    return fcntl(fd,F_GETFD);
}

static Fcj4TestAppendOps make_mock_ops(MockIo *mock)
{
    Fcj4TestAppendOps ops;
    memset(&ops,0,sizeof(ops));
    ops.pwrite_fn=mock_pwrite;
    ops.fsync_fn=mock_fsync;
    ops.close_fn=mock_close;
    ops.flock_fn=mock_flock;
    ops.fstat_fn=mock_fstat;
    ops.getfl_fn=mock_getfl;
    ops.getfd_fn=mock_getfd;
    ops.user=mock;
    return ops;
}

static void test_real_append_commit_handshake(void)
{
    char path[64],line[FCJ4_MAX_LINE_BYTES];
    int fd=make_temp_file(path);
    Fcj4WriterState *writer=NULL;
    Fcj4TestAppend *ctx;
    Fcj4TestAppendResult result;
    Fcj4PreparedRecord prepared;
    Fcj4Record record;
    size_t length=0U;
    uint64_t offset=0U;

    CHECK(fcj4_writer_state_create(&writer)==0);
    ctx=init_adapter(fd,NULL,0U);

    record=make_begin();
    prepare_record(writer,&record,line,&length,&prepared);
    CHECK(fcj4_test_append_record(ctx,line,length,offset,&result)==0);
    CHECK(result.code==FCJ4_TEST_APPEND_DURABLE);
    CHECK(result.durable_length==length);
    errno=0;
    CHECK(fcj4_test_append_record(ctx,line,length,offset,&result)==-1);
    CHECK(errno==EBUSY);
    CHECK(fcj4_writer_commit(writer,&prepared,line,length)==0);
    CHECK(fcj4_test_append_ack_commit(ctx,line,length)==0);
    offset=result.durable_length;

    record=make_sealed();
    prepare_record(writer,&record,line,&length,&prepared);
    CHECK(fcj4_test_append_record(ctx,line,length,offset,&result)==0);
    CHECK(fcj4_writer_commit(writer,&prepared,line,length)==0);
    CHECK(fcj4_test_append_ack_commit(ctx,line,length)==0);
    CHECK(fcj4_writer_revision(writer)==2U);
    CHECK(fcj4_test_append_destroy(ctx,&result)==0);
    CHECK(result.durable_length==offset+length);

    check_replay(path,2U);
    fcj4_writer_state_destroy(writer);
    CHECK(unlink(path)==0);
}

static void test_short_write_and_eintr(void)
{
    char path[64],line[FCJ4_MAX_LINE_BYTES];
    int fd=make_temp_file(path);
    MockIo mock;
    Fcj4TestAppendOps ops;
    Fcj4TestAppend *ctx;
    Fcj4TestAppendResult result;
    Fcj4WriterState *writer=NULL;
    Fcj4PreparedRecord prepared;
    Fcj4Record record=make_begin();
    size_t length=0U;

    memset(&mock,0,sizeof(mock));
    mock.mode=MOCK_SHORT_EINTR;
    ops=make_mock_ops(&mock);
    CHECK(fcj4_writer_state_create(&writer)==0);
    ctx=init_adapter(fd,&ops,0U);
    prepare_record(writer,&record,line,&length,&prepared);
    CHECK(fcj4_test_append_record(ctx,line,length,0U,&result)==0);
    CHECK(mock.pwrite_calls>=3);
    CHECK(mock.fsync_calls==1);
    CHECK(fcj4_writer_commit(writer,&prepared,line,length)==0);
    CHECK(fcj4_test_append_ack_commit(ctx,line,length)==0);
    CHECK(fcj4_test_append_destroy(ctx,&result)==0);
    check_replay(path,1U);
    fcj4_writer_state_destroy(writer);
    CHECK(unlink(path)==0);
}

static void test_partial_write_failure_is_poisoned(void)
{
    char path[64],line[FCJ4_MAX_LINE_BYTES],partial[64];
    int fd=make_temp_file(path);
    MockIo mock;
    Fcj4TestAppendOps ops;
    Fcj4TestAppend *ctx;
    Fcj4TestAppendResult result;
    Fcj4WriterState *writer=NULL;
    Fcj4PreparedRecord prepared;
    Fcj4Record record=make_begin();
    size_t length=0U,partial_length;
    Fcj4Replay replay;

    memset(&mock,0,sizeof(mock));
    mock.mode=MOCK_PARTIAL_EIO;
    ops=make_mock_ops(&mock);
    CHECK(fcj4_writer_state_create(&writer)==0);
    ctx=init_adapter(fd,&ops,0U);
    prepare_record(writer,&record,line,&length,&prepared);
    errno=0;
    CHECK(fcj4_test_append_record(ctx,line,length,0U,&result)==-1);
    CHECK(errno==EIO);
    CHECK(result.code==FCJ4_TEST_APPEND_UNCERTAIN_WRITE);
    CHECK(result.durable_length==0U);
    errno=0;
    CHECK(fcj4_test_append_record(ctx,line,length,0U,&result)==-1);
    CHECK(errno==EIO);
    CHECK(fcj4_writer_revision(writer)==0U);
    CHECK(fcj4_test_append_destroy(ctx,&result)==0);

    partial_length=read_journal(path,partial,sizeof(partial));
    CHECK(partial_length==3U);
    fcj4_replay_init(&replay);
    errno=0;
    CHECK(fcj4_replay_parse(partial,partial_length,&replay)==-1);
    CHECK(errno==EBADMSG);
    fcj4_replay_destroy(&replay);
    fcj4_writer_state_destroy(writer);
    CHECK(unlink(path)==0);
}

static void test_fsync_failure_requires_replay(void)
{
    char path[64],line[FCJ4_MAX_LINE_BYTES],bytes[FCJ4_MAX_LINE_BYTES];
    int fd=make_temp_file(path);
    MockIo mock;
    Fcj4TestAppendOps ops;
    Fcj4TestAppend *ctx;
    Fcj4TestAppendResult result;
    Fcj4WriterState *writer=NULL,*recovered=NULL;
    Fcj4PreparedRecord prepared;
    Fcj4Record record=make_begin();
    Fcj4Replay replay;
    size_t length=0U,read_length;

    memset(&mock,0,sizeof(mock));
    mock.mode=MOCK_FSYNC_EIO;
    ops=make_mock_ops(&mock);
    CHECK(fcj4_writer_state_create(&writer)==0);
    ctx=init_adapter(fd,&ops,0U);
    prepare_record(writer,&record,line,&length,&prepared);
    errno=0;
    CHECK(fcj4_test_append_record(ctx,line,length,0U,&result)==-1);
    CHECK(errno==EIO);
    CHECK(result.code==FCJ4_TEST_APPEND_UNCERTAIN_FSYNC);
    CHECK(result.durable_length==0U);
    CHECK(fcj4_writer_revision(writer)==0U);
    CHECK(fcj4_test_append_destroy(ctx,&result)==0);

    read_length=read_journal(path,bytes,sizeof(bytes));
    CHECK(read_length==length);
    fcj4_replay_init(&replay);
    CHECK(fcj4_replay_parse(bytes,read_length,&replay)==0);
    CHECK(fcj4_writer_state_create(&recovered)==0);
    CHECK(fcj4_writer_state_import_replay(recovered,&replay)==0);
    CHECK(fcj4_writer_revision(recovered)==1U);
    fcj4_replay_destroy(&replay);
    fcj4_writer_state_destroy(recovered);
    fcj4_writer_state_destroy(writer);
    CHECK(unlink(path)==0);
}

static void test_writer_commit_failure_forces_replay(void)
{
    char path[64],line[FCJ4_MAX_LINE_BYTES],bytes[FCJ4_MAX_LINE_BYTES];
    int fd=make_temp_file(path);
    Fcj4TestAppend *ctx=init_adapter(fd,NULL,0U);
    Fcj4TestAppendResult result;
    Fcj4WriterState *writer=NULL,*recovered=NULL;
    Fcj4PreparedRecord prepared,bad_prepared;
    Fcj4Record record=make_begin();
    Fcj4Replay replay;
    size_t length=0U,read_length;

    CHECK(fcj4_writer_state_create(&writer)==0);
    prepare_record(writer,&record,line,&length,&prepared);
    CHECK(fcj4_test_append_record(ctx,line,length,0U,&result)==0);
    bad_prepared=prepared;
    bad_prepared.token++;
    errno=0;
    CHECK(fcj4_writer_commit(writer,&bad_prepared,line,length)==-1);
    fcj4_test_append_poison(ctx);
    errno=0;
    CHECK(fcj4_test_append_ack_commit(ctx,line,length)==-1);
    CHECK(fcj4_test_append_destroy(ctx,&result)==0);
    CHECK(result.code==FCJ4_TEST_APPEND_RECONCILIATION_REQUIRED);
    CHECK(result.primary_errno==EIO);
    CHECK(result.durable_length==length);
    CHECK(fcj4_writer_revision(writer)==0U);

    read_length=read_journal(path,bytes,sizeof(bytes));
    fcj4_replay_init(&replay);
    CHECK(fcj4_replay_parse(bytes,read_length,&replay)==0);
    CHECK(fcj4_writer_state_create(&recovered)==0);
    CHECK(fcj4_writer_state_import_replay(recovered,&replay)==0);
    CHECK(fcj4_writer_revision(recovered)==1U);
    fcj4_replay_destroy(&replay);
    fcj4_writer_state_destroy(recovered);
    fcj4_writer_state_destroy(writer);
    CHECK(unlink(path)==0);
}

static void test_zero_write_poisoned(void)
{
    char path[64],line[FCJ4_MAX_LINE_BYTES];
    int fd=make_temp_file(path);
    MockIo mock;
    Fcj4TestAppendOps ops;
    Fcj4TestAppend *ctx;
    Fcj4TestAppendResult result;
    Fcj4WriterState *writer=NULL;
    Fcj4PreparedRecord prepared;
    Fcj4Record record=make_begin();
    size_t length=0U;

    memset(&mock,0,sizeof(mock));
    mock.mode=MOCK_ZERO_WRITE;
    ops=make_mock_ops(&mock);
    CHECK(fcj4_writer_state_create(&writer)==0);
    ctx=init_adapter(fd,&ops,0U);
    prepare_record(writer,&record,line,&length,&prepared);
    errno=0;
    CHECK(fcj4_test_append_record(ctx,line,length,0U,&result)==-1);
    CHECK(errno==EIO);
    CHECK(result.code==FCJ4_TEST_APPEND_UNCERTAIN_WRITE);
    CHECK(fcj4_test_append_destroy(ctx,&result)==0);
    CHECK(read_journal(path,line,sizeof(line))==0U);
    fcj4_writer_state_destroy(writer);
    CHECK(unlink(path)==0);
}


static void test_invalid_framing_rejected_before_write(void)
{
    char path[64],line[FCJ4_MAX_LINE_BYTES],bad[FCJ4_MAX_LINE_BYTES];
    int fd=make_temp_file(path);
    MockIo mock;
    Fcj4TestAppendOps ops;
    Fcj4TestAppend *ctx;
    Fcj4TestAppendResult result;
    Fcj4WriterState *writer=NULL;
    Fcj4PreparedRecord prepared;
    Fcj4Record record=make_begin();
    size_t length=0U;

    memset(&mock,0,sizeof(mock));
    ops=make_mock_ops(&mock);
    CHECK(fcj4_writer_state_create(&writer)==0);
    ctx=init_adapter(fd,&ops,0U);
    prepare_record(writer,&record,line,&length,&prepared);
    memcpy(bad,line,length);
    bad[5]='\n';
    errno=0;
    CHECK(fcj4_test_append_record(ctx,bad,length,0U,&result)==-1);
    CHECK(errno==EINVAL);
    CHECK(mock.pwrite_calls==0);
    CHECK(fcj4_test_append_record(ctx,line,length,0U,&result)==0);
    CHECK(fcj4_writer_commit(writer,&prepared,line,length)==0);
    CHECK(fcj4_test_append_ack_commit(ctx,line,length)==0);
    CHECK(fcj4_test_append_destroy(ctx,&result)==0);
    check_replay(path,1U);
    fcj4_writer_state_destroy(writer);
    CHECK(unlink(path)==0);
}

static void test_init_rollback_preserves_original_error_and_fd(void)
{
    char path[64];
    int fd=make_temp_file(path);
    struct stat st;
    MockIo mock;
    Fcj4TestAppendOps ops;
    Fcj4TestAppend *ctx=NULL;

    memset(&mock,0,sizeof(mock));
    mock.bad_identity=true;
    mock.fail_unlock=true;
    ops=make_mock_ops(&mock);
    CHECK(fstat(fd,&st)==0);
    errno=0;
    CHECK(fcj4_test_append_init(&ctx,fd,st.st_dev,st.st_ino,0U,&ops)==-1);
    CHECK(errno==EINVAL);
    CHECK(ctx==NULL);
    CHECK(mock.unlock_calls==1);
    CHECK(fcntl(fd,F_GETFD)>=0);
    CHECK(close(fd)==0);
    CHECK(unlink(path)==0);
}

static void test_init_rejects_append_flag_without_ownership_transfer(void)
{
    char path[64];
    int fd=make_temp_file(path);
    int flags=fcntl(fd,F_GETFL);
    struct stat st;
    Fcj4TestAppend *ctx=NULL;

    CHECK(flags>=0);
    CHECK(fcntl(fd,F_SETFL,flags|O_APPEND)==0);
    CHECK(fstat(fd,&st)==0);
    errno=0;
    CHECK(fcj4_test_append_init(&ctx,fd,st.st_dev,st.st_ino,0U,NULL)==-1);
    CHECK(errno==EINVAL);
    CHECK(ctx==NULL);
    CHECK(fcntl(fd,F_GETFD)>=0);
    CHECK(fcntl(fd,F_SETFL,flags)==0);
    CHECK(close(fd)==0);
    CHECK(unlink(path)==0);
}

static void test_lock_contention_preserves_caller_descriptor(void)
{
    char path[64];
    int first=make_temp_file(path);
    int second=open(path,O_RDWR|O_CLOEXEC);
    struct stat st;
    Fcj4TestAppend *ctx=NULL;

    CHECK(second>=0);
    CHECK(fstat(first,&st)==0);
    CHECK(flock(first,LOCK_EX|LOCK_NB)==0);
    errno=0;
    CHECK(fcj4_test_append_init(&ctx,second,st.st_dev,st.st_ino,0U,NULL)==-1);
    CHECK(errno==EWOULDBLOCK || errno==EAGAIN);
    CHECK(ctx==NULL);
    CHECK(fcntl(second,F_GETFD)>=0);
    CHECK(flock(first,LOCK_UN)==0);
    CHECK(close(first)==0);
    CHECK(close(second)==0);
    CHECK(unlink(path)==0);
}

static void test_external_size_change_poisoned(void)
{
    char path[64],line[FCJ4_MAX_LINE_BYTES];
    int fd=make_temp_file(path);
    int other=open(path,O_WRONLY|O_CLOEXEC);
    Fcj4TestAppend *ctx;
    Fcj4TestAppendResult result;
    Fcj4WriterState *writer=NULL;
    Fcj4PreparedRecord prepared;
    Fcj4Record record=make_begin();
    size_t length=0U;

    CHECK(other>=0);
    CHECK(fcj4_writer_state_create(&writer)==0);
    ctx=init_adapter(fd,NULL,0U);
    CHECK(pwrite(other,"x",1U,0)==1);
    prepare_record(writer,&record,line,&length,&prepared);
    errno=0;
    CHECK(fcj4_test_append_record(ctx,line,length,0U,&result)==-1);
    CHECK(errno==ESTALE);
    CHECK(fcj4_test_append_record(ctx,line,length,0U,&result)==-1);
    CHECK(errno==EIO);
    CHECK(fcj4_test_append_destroy(ctx,&result)==0);
    CHECK(close(other)==0);
    fcj4_writer_state_destroy(writer);
    CHECK(unlink(path)==0);
}

static void test_cleanup_errors_do_not_erase_durable_result(void)
{
    char path[64],line[FCJ4_MAX_LINE_BYTES];
    int fd=make_temp_file(path);
    MockIo mock;
    Fcj4TestAppendOps ops;
    Fcj4TestAppend *ctx;
    Fcj4TestAppendResult result;
    Fcj4WriterState *writer=NULL;
    Fcj4PreparedRecord prepared;
    Fcj4Record record=make_begin();
    size_t length=0U;

    memset(&mock,0,sizeof(mock));
    mock.fail_unlock=true;
    mock.fail_close=true;
    ops=make_mock_ops(&mock);
    CHECK(fcj4_writer_state_create(&writer)==0);
    ctx=init_adapter(fd,&ops,0U);
    prepare_record(writer,&record,line,&length,&prepared);
    CHECK(fcj4_test_append_record(ctx,line,length,0U,&result)==0);
    CHECK(fcj4_writer_commit(writer,&prepared,line,length)==0);
    CHECK(fcj4_test_append_ack_commit(ctx,line,length)==0);
    errno=0;
    CHECK(fcj4_test_append_destroy(ctx,&result)==-1);
    CHECK(errno==EIO);
    CHECK(result.code==FCJ4_TEST_APPEND_CLEANUP_ERROR);
    CHECK(result.primary_errno==0);
    CHECK(result.cleanup_errno==EIO);
    CHECK(result.durable_length==length);
    CHECK(mock.unlock_calls==1);
    CHECK(mock.close_calls==1);
    errno=0;
    CHECK(fcntl(fd,F_GETFD)==-1 && errno==EBADF);
    check_replay(path,1U);
    fcj4_writer_state_destroy(writer);
    CHECK(unlink(path)==0);
}

static void test_process_exit_restart_replay(void)
{
    char path[64],bytes[FCJ4_MAX_LINE_BYTES];
    int fd=make_temp_file(path);
    pid_t child;
    int status=0;
    size_t length;
    Fcj4Replay replay;

    CHECK(close(fd)==0);
    child=fork();
    CHECK(child>=0);
    if(child==0) {
        int child_fd=open(path,O_RDWR|O_CLOEXEC);
        Fcj4WriterState *writer=NULL;
        Fcj4TestAppend *ctx;
        Fcj4TestAppendResult result;
        Fcj4PreparedRecord prepared;
        Fcj4Record record=make_begin();
        char line[FCJ4_MAX_LINE_BYTES];
        size_t line_length=0U;
        struct stat st;
        if(child_fd<0 || fstat(child_fd,&st)!=0 ||
           fcj4_writer_state_create(&writer)!=0)
            _exit(10);
        ctx=NULL;
        if(fcj4_test_append_init(&ctx,child_fd,st.st_dev,st.st_ino,0U,NULL)!=0)
            _exit(11);
        if(fcj4_writer_prepare(writer,&record,line,sizeof(line),
                               &line_length,&prepared)!=0)
            _exit(12);
        if(fcj4_test_append_record(ctx,line,line_length,0U,&result)!=0)
            _exit(13);
        if(fcj4_writer_commit(writer,&prepared,line,line_length)!=0)
            _exit(14);
        if(fcj4_test_append_ack_commit(ctx,line,line_length)!=0)
            _exit(15);
        /* _exit models process termination only, not power loss. */
        _exit(0);
    }
    CHECK(waitpid(child,&status,0)==child);
    CHECK(WIFEXITED(status) && WEXITSTATUS(status)==0);
    length=read_journal(path,bytes,sizeof(bytes));
    fcj4_replay_init(&replay);
    CHECK(fcj4_replay_parse(bytes,length,&replay)==0);
    CHECK(replay.record_count==1U);
    fcj4_replay_destroy(&replay);
    CHECK(unlink(path)==0);
}

int main(void)
{
    test_real_append_commit_handshake();
    test_short_write_and_eintr();
    test_partial_write_failure_is_poisoned();
    test_fsync_failure_requires_replay();
    test_writer_commit_failure_forces_replay();
    test_zero_write_poisoned();
    test_invalid_framing_rejected_before_write();
    test_init_rollback_preserves_original_error_and_fd();
    test_init_rejects_append_flag_without_ownership_transfer();
    test_lock_contention_preserves_caller_descriptor();
    test_external_size_change_poisoned();
    test_cleanup_errors_do_not_erase_durable_result();
    test_process_exit_restart_replay();
    puts("FCJ4 test append adapter: all tests passed");
    return EXIT_SUCCESS;
}
