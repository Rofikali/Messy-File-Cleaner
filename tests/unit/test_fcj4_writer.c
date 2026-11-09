#include "journal/fcj4_writer.h"

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr,"CHECK failed at %s:%d: %s (errno=%d)\n", \
                __FILE__,__LINE__,#expr,errno); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static const char opid[]="11111111111111111111111111111111";
static const char actionid[]="bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
static const char digest[]=
    "0000000000000000000000000000000000000000000000000000000000000000";

static void append_record(char *journal,size_t capacity,size_t *used,
                          Fcj4WriterState *state,const Fcj4Record *record)
{
    char line[FCJ4_MAX_LINE_BYTES];
    size_t length=0U;
    Fcj4PreparedRecord prepared;
    CHECK(fcj4_writer_prepare(state,record,line,sizeof(line),&length,&prepared)==0);
    CHECK(length>0U && line[length-1U]=='\n');
    CHECK(*used<=capacity && length<=capacity-*used);
    memcpy(journal+*used,line,length);
    *used+=length;
    CHECK(fcj4_writer_commit(state,&prepared,line,length)==0);
}

static void test_round_trip_and_transitions(void)
{
    char journal[8192];
    size_t used=0U;
    Fcj4WriterState *state=NULL;
    Fcj4Replay replay;
    Fcj4Record r;
    Fcj4PreparedRecord prepared;
    char line[FCJ4_MAX_LINE_BYTES];
    size_t length=0U;

    CHECK(fcj4_writer_state_create(&state)==0);
    memset(&r,0,sizeof(r));
    r.kind=FCJ4_RECORD_BEGIN;
    r.value.begin=(Fcj4BeginRecord){
        opid,"/tmp/source",1U,2U,1U,3U,1000U,1000U,384U,5U,
        1,2U,3,4U,"/tmp/dest",1U,4U,"dest",1U,5U
    };

    CHECK(fcj4_writer_prepare(state,&r,line,sizeof(line),&length,&prepared)==0);
    CHECK(fcj4_writer_revision(state)==0U);
    errno=0;
    CHECK(fcj4_writer_prepare(state,&r,line,sizeof(line),&length,&prepared)==-1);
    CHECK(errno==EBUSY);
    /* Abort proves prepare alone does not advance the in-memory state. */
    CHECK(fcj4_writer_abort_prepare(state,&prepared)==0);
    append_record(journal,sizeof(journal),&used,state,&r);

    memset(&r,0,sizeof(r));
    r.kind=FCJ4_RECORD_SEALED;
    r.value.sealed=(Fcj4SealedRecord){opid,".fc-tmp-x",1U,6U,1000U,384U,5U,digest};
    append_record(journal,sizeof(journal),&used,state,&r);

    memset(&r,0,sizeof(r));
    r.kind=FCJ4_RECORD_MOVE;
    r.value.move=(Fcj4MoveRecord){opid,1U,FCJ4_MOVE_STAGED,0U};
    append_record(journal,sizeof(journal),&used,state,&r);
    r.value.move.sequence=2U;
    r.value.move.state=FCJ4_MOVE_STARTED;
    append_record(journal,sizeof(journal),&used,state,&r);
    r.value.move.sequence=3U;
    r.value.move.state=FCJ4_MOVE_PUBLISHED;
    append_record(journal,sizeof(journal),&used,state,&r);
    r.value.move.sequence=4U;
    r.value.move.state=FCJ4_MOVE_COMPLETED;
    append_record(journal,sizeof(journal),&used,state,&r);

    memset(&r,0,sizeof(r));
    r.kind=FCJ4_RECORD_ACTION_DEF;
    r.value.action_def=(Fcj4ActionDefRecord){
        opid,actionid,FCJ4_ACTION_KIND_QUARANTINE_STAGE,
        FCJ4_TARGET_QUARANTINE,1U,4U,1U,6U,1000U,384U,5U,
        digest,".fc-tmp-x","q-stage"
    };
    append_record(journal,sizeof(journal),&used,state,&r);

    memset(&r,0,sizeof(r));
    r.kind=FCJ4_RECORD_ACTION;
    r.value.action=(Fcj4ActionRecord){opid,actionid,1U,FCJ4_ACTION_PREPARED,0U};
    append_record(journal,sizeof(journal),&used,state,&r);

    fcj4_replay_init(&replay);
    CHECK(fcj4_replay_parse(journal,used,&replay)==0);
    CHECK(replay.count==1U && replay.record_count==8U);
    fcj4_replay_destroy(&replay);
    CHECK(fcj4_writer_revision(state)==8U);
    fcj4_writer_state_destroy(state);
}

static void test_serializer_rejects_noncanonical_inputs(void)
{
    Fcj4Record r;
    char output[FCJ4_MAX_LINE_BYTES];
    size_t written=99U;
    memset(&r,0,sizeof(r));
    r.kind=FCJ4_RECORD_MOVE;
    r.value.move=(Fcj4MoveRecord){opid,1U,FCJ4_MOVE_FAILED,0U};
    errno=0;
    CHECK(fcj4_serialize(&r,output,sizeof(output),&written)==-1);
    CHECK(errno==EINVAL && written==0U);

    r.value.move=(Fcj4MoveRecord){opid,1U,FCJ4_MOVE_STAGED,0U};
    errno=0;
    CHECK(fcj4_serialize(&r,output,2U,&written)==-1);
    CHECK(errno==ENOSPC && written==0U);
}

static void test_stale_and_duplicate_commit_rejected(void)
{
    Fcj4WriterState *state=NULL;
    Fcj4Record r;
    Fcj4PreparedRecord prepared;
    char output[FCJ4_MAX_LINE_BYTES];
    size_t written=0U;
    CHECK(fcj4_writer_state_create(&state)==0);
    memset(&r,0,sizeof(r));
    r.kind=FCJ4_RECORD_BEGIN;
    r.value.begin=(Fcj4BeginRecord){
        opid,"/tmp/source",1U,2U,1U,3U,1000U,1000U,384U,5U,
        1,2U,3,4U,"/tmp/dest",1U,4U,"dest",1U,5U
    };
    CHECK(fcj4_writer_prepare(state,&r,output,sizeof(output),&written,&prepared)==0);
    {
        char altered[FCJ4_MAX_LINE_BYTES];
        memcpy(altered,output,written);
        altered[0]='X';
        errno=0;
        CHECK(fcj4_writer_commit(state,&prepared,altered,written)==-1);
        CHECK(errno==EINVAL);
    }
    CHECK(fcj4_writer_commit(state,&prepared,output,written)==0);
    errno=0;
    CHECK(fcj4_writer_commit(state,&prepared,output,written)==-1);
    CHECK(errno==EINVAL);
    fcj4_writer_state_destroy(state);
}

int main(void)
{
    test_round_trip_and_transitions();
    test_serializer_rejects_noncanonical_inputs();
    test_stale_and_duplicate_commit_rejected();
    puts("FCJ4 serializer/state unit tests passed");
    return 0;
}
