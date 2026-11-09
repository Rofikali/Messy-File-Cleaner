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
    Fcj4WriterState *imported=NULL;
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
    CHECK(fcj4_writer_state_create(&imported)==0);
    CHECK(fcj4_writer_state_import_replay(imported,&replay)==0);
    CHECK(fcj4_writer_revision(imported)==8U);
    fcj4_replay_destroy(&replay);

    /* Imported state must own all strings after the replay is destroyed. */
    memset(&r,0,sizeof(r));
    r.kind=FCJ4_RECORD_ACTION;
    r.value.action=(Fcj4ActionRecord){
        opid,actionid,2U,FCJ4_ACTION_STARTED,0U
    };
    {
        char original_line[FCJ4_MAX_LINE_BYTES];
        char imported_line[FCJ4_MAX_LINE_BYTES];
        size_t original_n=0U, imported_n=0U;
        Fcj4PreparedRecord original_p, imported_p;
        CHECK(fcj4_writer_prepare(state,&r,original_line,sizeof(original_line),
                                  &original_n,&original_p)==0);
        CHECK(fcj4_writer_prepare(imported,&r,imported_line,sizeof(imported_line),
                                  &imported_n,&imported_p)==0);
        CHECK(original_n==imported_n &&
              memcmp(original_line,imported_line,original_n)==0);
        CHECK(fcj4_writer_abort_prepare(state,&original_p)==0);
        CHECK(fcj4_writer_abort_prepare(imported,&imported_p)==0);
    }
    CHECK(fcj4_writer_revision(state)==8U);
    fcj4_writer_state_destroy(imported);
    fcj4_writer_state_destroy(state);
}

static void commit_without_journal(Fcj4WriterState *state,
                                     const Fcj4Record *record)
{
    char output[FCJ4_MAX_LINE_BYTES];
    size_t written=0U;
    Fcj4PreparedRecord prepared;
    CHECK(fcj4_writer_prepare(state,record,output,sizeof(output),
                              &written,&prepared)==0);
    CHECK(fcj4_writer_commit(state,&prepared,output,written)==0);
}

static void setup_evidence_state(Fcj4WriterState **state)
{
    Fcj4Record r;
    CHECK(fcj4_writer_state_create(state)==0);
    memset(&r,0,sizeof(r));
    r.kind=FCJ4_RECORD_BEGIN;
    r.value.begin=(Fcj4BeginRecord){
        opid,"/tmp/source",1U,2U,1U,3U,1000U,1000U,384U,5U,
        1,2U,3,4U,"/tmp/dest",1U,4U,"dest",1U,5U
    };
    commit_without_journal(*state,&r);
    memset(&r,0,sizeof(r));
    r.kind=FCJ4_RECORD_SEALED;
    r.value.sealed=(Fcj4SealedRecord){
        opid,".fc-tmp-x",1U,6U,1000U,384U,5U,digest
    };
    commit_without_journal(*state,&r);
}

static Fcj4ActionDefRecord make_action_def(Fcj4ActionKind kind)
{
    Fcj4ActionDefRecord r;
    memset(&r,0,sizeof(r));
    r.operation_id=opid;
    r.expected_uid=1000U;
    r.expected_mode=384U;
    r.expected_size=5U;
    r.expected_sha256=digest;
    switch(kind) {
    case FCJ4_ACTION_KIND_QUARANTINE_STAGE:
        r.action_id="bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
        r.kind=kind; r.target_role=FCJ4_TARGET_QUARANTINE;
        r.expected_parent_dev=1U; r.expected_parent_ino=4U;
        r.expected_entry_dev=1U; r.expected_entry_ino=6U;
        r.source_component=".fc-tmp-x"; r.target_component="q-stage";
        break;
    case FCJ4_ACTION_KIND_QUARANTINE_SOURCE:
        r.action_id="cccccccccccccccccccccccccccccccc";
        r.kind=kind; r.target_role=FCJ4_TARGET_QUARANTINE;
        r.expected_parent_dev=1U; r.expected_parent_ino=2U;
        r.expected_entry_dev=1U; r.expected_entry_ino=3U;
        r.source_component="source"; r.target_component="q-source";
        break;
    case FCJ4_ACTION_KIND_RETIRE_QUARANTINED_SOURCE:
        r.action_id="dddddddddddddddddddddddddddddddd";
        r.kind=kind; r.target_role=FCJ4_TARGET_NONE;
        r.expected_parent_dev=1U; r.expected_parent_ino=5U;
        r.expected_entry_dev=1U; r.expected_entry_ino=3U;
        r.source_component="q-source"; r.target_component=NULL;
        break;
    default:
        r.action_id="eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
        r.kind=FCJ4_ACTION_KIND_REMOVE_QUARANTINED_STAGE;
        r.target_role=FCJ4_TARGET_NONE;
        r.expected_parent_dev=1U; r.expected_parent_ino=5U;
        r.expected_entry_dev=1U; r.expected_entry_ino=6U;
        r.source_component="q-stage"; r.target_component=NULL;
        break;
    }
    return r;
}

static void prepare_action_test_state(Fcj4WriterState **state,
                                      Fcj4ActionKind kind)
{
    Fcj4Record r;
    Fcj4ActionDefRecord prerequisite;
    setup_evidence_state(state);
    if(kind==FCJ4_ACTION_KIND_RETIRE_QUARANTINED_SOURCE) {
        prerequisite=make_action_def(FCJ4_ACTION_KIND_QUARANTINE_SOURCE);
        memset(&r,0,sizeof(r)); r.kind=FCJ4_RECORD_ACTION_DEF;
        r.value.action_def=prerequisite;
        commit_without_journal(*state,&r);
    } else if(kind==FCJ4_ACTION_KIND_REMOVE_QUARANTINED_STAGE) {
        prerequisite=make_action_def(FCJ4_ACTION_KIND_QUARANTINE_STAGE);
        memset(&r,0,sizeof(r)); r.kind=FCJ4_RECORD_ACTION_DEF;
        r.value.action_def=prerequisite;
        commit_without_journal(*state,&r);
    }
}

static void expect_action_def_rejected(Fcj4ActionDefRecord def)
{
    Fcj4WriterState *state=NULL;
    Fcj4Record r;
    Fcj4PreparedRecord prepared;
    char output[FCJ4_MAX_LINE_BYTES];
    size_t written=0U;
    prepare_action_test_state(&state,def.kind);
    memset(&r,0,sizeof(r));
    r.kind=FCJ4_RECORD_ACTION_DEF;
    r.value.action_def=def;
    errno=0;
    CHECK(fcj4_writer_prepare(state,&r,output,sizeof(output),
                              &written,&prepared)==-1);
    CHECK(errno==EINVAL);
    fcj4_writer_state_destroy(state);
}

static void test_all_action_definition_kinds_and_mismatches(void)
{
    const Fcj4ActionKind kinds[]={
        FCJ4_ACTION_KIND_QUARANTINE_STAGE,
        FCJ4_ACTION_KIND_QUARANTINE_SOURCE,
        FCJ4_ACTION_KIND_RETIRE_QUARANTINED_SOURCE,
        FCJ4_ACTION_KIND_REMOVE_QUARANTINED_STAGE
    };
    for(size_t i=0U;i<sizeof(kinds)/sizeof(kinds[0]);i++) {
        Fcj4WriterState *state=NULL;
        Fcj4Record r;
        Fcj4ActionDefRecord def=make_action_def(kinds[i]);
        prepare_action_test_state(&state,kinds[i]);
        memset(&r,0,sizeof(r));
        r.kind=FCJ4_RECORD_ACTION_DEF;
        r.value.action_def=def;
        commit_without_journal(state,&r);
        fcj4_writer_state_destroy(state);

        def=make_action_def(kinds[i]); def.expected_parent_dev++;
        expect_action_def_rejected(def);
        def=make_action_def(kinds[i]); def.expected_parent_ino++;
        expect_action_def_rejected(def);
        def=make_action_def(kinds[i]); def.expected_entry_dev++;
        expect_action_def_rejected(def);
        def=make_action_def(kinds[i]); def.expected_entry_ino++;
        expect_action_def_rejected(def);
        def=make_action_def(kinds[i]); def.expected_uid++;
        expect_action_def_rejected(def);
        def=make_action_def(kinds[i]); def.expected_mode++;
        expect_action_def_rejected(def);
        def=make_action_def(kinds[i]); def.expected_size++;
        expect_action_def_rejected(def);
        def=make_action_def(kinds[i]);
        def.expected_sha256="1111111111111111111111111111111111111111111111111111111111111111";
        expect_action_def_rejected(def);
        def=make_action_def(kinds[i]); def.source_component="wrong";
        expect_action_def_rejected(def);
        def=make_action_def(kinds[i]);
        def.target_role=(kinds[i]==FCJ4_ACTION_KIND_QUARANTINE_STAGE ||
                         kinds[i]==FCJ4_ACTION_KIND_QUARANTINE_SOURCE)
            ? FCJ4_TARGET_NONE : FCJ4_TARGET_QUARANTINE;
        def.target_component=(def.target_role==FCJ4_TARGET_QUARANTINE)
            ? "unexpected" : NULL;
        expect_action_def_rejected(def);
    }
}

static void test_all_action_kinds_parse_after_writer_acceptance(void)
{
    char journal[8192];
    size_t used=0U;
    Fcj4WriterState *state=NULL;
    Fcj4Replay replay;
    Fcj4Record r;
    Fcj4ActionDefRecord def;

    CHECK(fcj4_writer_state_create(&state)==0);
    memset(&r,0,sizeof(r));
    r.kind=FCJ4_RECORD_BEGIN;
    r.value.begin=(Fcj4BeginRecord){
        opid,"/tmp/source",1U,2U,1U,3U,1000U,1000U,384U,5U,
        1,2U,3,4U,"/tmp/dest",1U,4U,"dest",1U,5U
    };
    append_record(journal,sizeof(journal),&used,state,&r);
    memset(&r,0,sizeof(r));
    r.kind=FCJ4_RECORD_SEALED;
    r.value.sealed=(Fcj4SealedRecord){
        opid,".fc-tmp-x",1U,6U,1000U,384U,5U,digest
    };
    append_record(journal,sizeof(journal),&used,state,&r);

    def=make_action_def(FCJ4_ACTION_KIND_QUARANTINE_STAGE);
    memset(&r,0,sizeof(r)); r.kind=FCJ4_RECORD_ACTION_DEF;
    r.value.action_def=def; append_record(journal,sizeof(journal),&used,state,&r);
    def=make_action_def(FCJ4_ACTION_KIND_QUARANTINE_SOURCE);
    r.value.action_def=def; append_record(journal,sizeof(journal),&used,state,&r);
    def=make_action_def(FCJ4_ACTION_KIND_RETIRE_QUARANTINED_SOURCE);
    r.value.action_def=def; append_record(journal,sizeof(journal),&used,state,&r);
    def=make_action_def(FCJ4_ACTION_KIND_REMOVE_QUARANTINED_STAGE);
    r.value.action_def=def; append_record(journal,sizeof(journal),&used,state,&r);

    fcj4_replay_init(&replay);
    CHECK(fcj4_replay_parse(journal,used,&replay)==0);
    CHECK(replay.count==1U && replay.record_count==6U);
    {
        Fcj4OperationView view;
        CHECK(fcj4_replay_operation(&replay,0U,&view)==0);
        CHECK(view.action_count==4U);
    }
    fcj4_replay_destroy(&replay);
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

static void test_replay_import_failure_is_atomic(void)
{
    Fcj4WriterState *state=NULL;
    Fcj4Replay invalid_replay;
    CHECK(fcj4_writer_state_create(&state)==0);
    fcj4_replay_init(&invalid_replay);
    /* A forged/inconsistent public replay descriptor must be rejected. */
    invalid_replay.count=1U;
    errno=0;
    CHECK(fcj4_writer_state_import_replay(state,&invalid_replay)==-1);
    CHECK(errno==EINVAL);
    CHECK(fcj4_writer_revision(state)==0U);
    /* The failed import must leave the destination usable and empty. */
    {
        Fcj4Record begin;
        char output[FCJ4_MAX_LINE_BYTES];
        size_t written=0U;
        Fcj4PreparedRecord prepared;
        memset(&begin,0,sizeof(begin));
        begin.kind=FCJ4_RECORD_BEGIN;
        begin.value.begin=(Fcj4BeginRecord){
            opid,"/tmp/source",1U,2U,1U,3U,1000U,1000U,384U,5U,
            1,2U,3,4U,"/tmp/dest",1U,4U,"dest",1U,5U
        };
        CHECK(fcj4_writer_prepare(state,&begin,output,sizeof(output),
                                  &written,&prepared)==0);
        CHECK(fcj4_writer_abort_prepare(state,&prepared)==0);
    }
    invalid_replay.count=0U;
    fcj4_writer_state_destroy(state);
}

static void test_cross_record_evidence_rejected(void)
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
    CHECK(fcj4_writer_commit(state,&prepared,output,written)==0);

    memset(&r,0,sizeof(r));
    r.kind=FCJ4_RECORD_SEALED;
    r.value.sealed=(Fcj4SealedRecord){
        opid,".fc-tmp-x",1U,6U,1000U,384U,6U,digest
    };
    errno=0;
    CHECK(fcj4_writer_prepare(state,&r,output,sizeof(output),&written,&prepared)==-1);
    CHECK(errno==EINVAL);
    CHECK(fcj4_writer_revision(state)==1U);

    r.value.sealed.stage_size=5U;
    CHECK(fcj4_writer_prepare(state,&r,output,sizeof(output),&written,&prepared)==0);
    CHECK(fcj4_writer_commit(state,&prepared,output,written)==0);

    memset(&r,0,sizeof(r));
    r.kind=FCJ4_RECORD_ACTION_DEF;
    r.value.action_def=(Fcj4ActionDefRecord){
        opid,actionid,FCJ4_ACTION_KIND_QUARANTINE_STAGE,
        FCJ4_TARGET_QUARANTINE,1U,99U,1U,6U,1000U,384U,5U,
        digest,".fc-tmp-x","q-stage"
    };
    errno=0;
    CHECK(fcj4_writer_prepare(state,&r,output,sizeof(output),&written,&prepared)==-1);
    CHECK(errno==EINVAL);
    CHECK(fcj4_writer_revision(state)==2U);

    /* Missing evidence strings must fail closed instead of being dereferenced. */
    r.value.action_def.expected_parent_ino=4U;
    r.value.action_def.expected_sha256=NULL;
    errno=0;
    CHECK(fcj4_writer_prepare(state,&r,output,sizeof(output),&written,&prepared)==-1);
    CHECK(errno==EINVAL);
    CHECK(fcj4_writer_revision(state)==2U);
    fcj4_writer_state_destroy(state);
}


static void append_serialized_limit_record(char *journal,size_t capacity,
                                           size_t *used,
                                           const Fcj4Record *record)
{
    char line[FCJ4_MAX_LINE_BYTES];
    size_t length=0U;
    CHECK(fcj4_serialize(record,line,sizeof(line),&length)==0);
    CHECK(length>0U && line[length-1U]=='\n');
    CHECK(*used<=capacity && length<=capacity-*used);
    memcpy(journal+*used,line,length);
    *used+=length;
}

static void append_limit_operation(char *journal,size_t capacity,size_t *used,
                                   size_t operation_index,size_t action_events)
{
    char operation_id[FCJ4_OPERATION_ID_HEX_LENGTH+1U];
    char source_path[64],destination_path[64],destination_name[32];
    Fcj4Record record;
    static const Fcj4MoveState moves[]={
        FCJ4_MOVE_STAGED,FCJ4_MOVE_STARTED,
        FCJ4_MOVE_PUBLISHED,FCJ4_MOVE_COMPLETED
    };
    static const Fcj4ActionState actions[]={
        FCJ4_ACTION_PREPARED,FCJ4_ACTION_STARTED,FCJ4_ACTION_APPLIED,
        FCJ4_ACTION_SYNCED,FCJ4_ACTION_COMPLETED
    };

    CHECK(snprintf(operation_id,sizeof(operation_id),"%032zx",
                   operation_index+1U)>0);
    CHECK(snprintf(source_path,sizeof(source_path),"/tmp/source-%04zu",
                   operation_index)>0);
    CHECK(snprintf(destination_path,sizeof(destination_path),"/tmp/dest-%04zu",
                   operation_index)>0);
    CHECK(snprintf(destination_name,sizeof(destination_name),"dest-%04zu",
                   operation_index)>0);

    memset(&record,0,sizeof(record));
    record.kind=FCJ4_RECORD_BEGIN;
    record.value.begin=(Fcj4BeginRecord){
        operation_id,source_path,1U,2U,1U,3U,1000U,1000U,384U,5U,
        1,2U,3,4U,destination_path,1U,4U,destination_name,1U,5U
    };
    append_serialized_limit_record(journal,capacity,used,&record);

    memset(&record,0,sizeof(record));
    record.kind=FCJ4_RECORD_SEALED;
    record.value.sealed=(Fcj4SealedRecord){
        operation_id,".fc-tmp-x",1U,6U,1000U,384U,5U,digest
    };
    append_serialized_limit_record(journal,capacity,used,&record);

    for(size_t i=0U;i<sizeof(moves)/sizeof(moves[0]);i++) {
        memset(&record,0,sizeof(record));
        record.kind=FCJ4_RECORD_MOVE;
        record.value.move=(Fcj4MoveRecord){
            operation_id,(uint64_t)i+1U,moves[i],0U
        };
        append_serialized_limit_record(journal,capacity,used,&record);
    }

    for(size_t i=0U;i<FCJ4_MAX_ACTIONS_PER_OPERATION;i++) {
        char id[FCJ4_ACTION_ID_HEX_LENGTH+1U];
        char target[NAME_MAX];
        CHECK(snprintf(id,sizeof(id),"%032zx",i+1U)>0);
        CHECK(snprintf(target,sizeof(target),"q-%02zu",i)>0);
        memset(&record,0,sizeof(record));
        record.kind=FCJ4_RECORD_ACTION_DEF;
        record.value.action_def=(Fcj4ActionDefRecord){
            operation_id,id,FCJ4_ACTION_KIND_QUARANTINE_STAGE,
            FCJ4_TARGET_QUARANTINE,1U,4U,1U,6U,1000U,384U,5U,
            digest,".fc-tmp-x",target
        };
        append_serialized_limit_record(journal,capacity,used,&record);
    }

    for(size_t i=0U;i<action_events;i++) {
        size_t action_index=i/5U;
        size_t state_index=i%5U;
        char id[FCJ4_ACTION_ID_HEX_LENGTH+1U];
        CHECK(action_index<FCJ4_MAX_ACTIONS_PER_OPERATION);
        CHECK(snprintf(id,sizeof(id),"%032zx",action_index+1U)>0);
        memset(&record,0,sizeof(record));
        record.kind=FCJ4_RECORD_ACTION;
        record.value.action=(Fcj4ActionRecord){
            operation_id,id,(uint64_t)state_index+1U,actions[state_index],0U
        };
        append_serialized_limit_record(journal,capacity,used,&record);
    }
}

static void test_max_record_boundary_and_abort_accounting(void)
{
    const size_t complete_operations=980U;
    char *journal=(char *)malloc(FCJ4_MAX_JOURNAL_BYTES);
    size_t used=0U;
    Fcj4Replay replay;
    Fcj4WriterState *state=NULL;
    Fcj4Record record;
    Fcj4PreparedRecord prepared;
    char output[FCJ4_MAX_LINE_BYTES];
    size_t written=0U;
    char final_operation_id[FCJ4_OPERATION_ID_HEX_LENGTH+1U];
    char final_action_id[FCJ4_ACTION_ID_HEX_LENGTH+1U];

    CHECK(journal!=NULL);
    for(size_t i=0U;i<complete_operations;i++)
        append_limit_operation(journal,FCJ4_MAX_JOURNAL_BYTES,&used,i,80U);
    /* 980 complete operations contribute 99,960 records. This final
       operation contributes 39 records, producing a valid 99,999-record
       parser result without modifying any replay descriptor fields. */
    append_limit_operation(journal,FCJ4_MAX_JOURNAL_BYTES,&used,
                           complete_operations,17U);

    fcj4_replay_init(&replay);
    CHECK(fcj4_replay_parse(journal,used,&replay)==0);
    CHECK(replay.record_count==FCJ4_MAX_RECORDS-1U);
    CHECK(replay.count==complete_operations+1U);
    CHECK(fcj4_writer_state_create(&state)==0);
    CHECK(fcj4_writer_state_import_replay(state,&replay)==0);
    CHECK(fcj4_writer_revision(state)==FCJ4_MAX_RECORDS-1U);

    CHECK(snprintf(final_operation_id,sizeof(final_operation_id),"%032zx",
                   complete_operations+1U)>0);
    CHECK(snprintf(final_action_id,sizeof(final_action_id),"%032x",4U)>0);
    memset(&record,0,sizeof(record));
    record.kind=FCJ4_RECORD_ACTION;
    record.value.action=(Fcj4ActionRecord){
        final_operation_id,final_action_id,3U,FCJ4_ACTION_APPLIED,0U
    };

    CHECK(fcj4_writer_prepare(state,&record,output,sizeof(output),
                              &written,&prepared)==0);
    CHECK(fcj4_writer_revision(state)==FCJ4_MAX_RECORDS-1U);
    CHECK(fcj4_writer_abort_prepare(state,&prepared)==0);
    CHECK(fcj4_writer_revision(state)==FCJ4_MAX_RECORDS-1U);

    /* Abort must not consume the final record slot: the same record can be
       prepared again and committed exactly once. */
    CHECK(fcj4_writer_prepare(state,&record,output,sizeof(output),
                              &written,&prepared)==0);
    CHECK(fcj4_writer_commit(state,&prepared,output,written)==0);
    CHECK(fcj4_writer_revision(state)==FCJ4_MAX_RECORDS);

    record.value.action.sequence=4U;
    record.value.action.state=FCJ4_ACTION_SYNCED;
    errno=0;
    CHECK(fcj4_writer_prepare(state,&record,output,sizeof(output),
                              &written,&prepared)==-1);
    CHECK(errno==E2BIG);
    CHECK(fcj4_writer_revision(state)==FCJ4_MAX_RECORDS);

    fcj4_writer_state_destroy(state);
    fcj4_replay_destroy(&replay);
    free(journal);
}

int main(void)
{
    test_max_record_boundary_and_abort_accounting();
    test_round_trip_and_transitions();
    test_cross_record_evidence_rejected();
    test_all_action_definition_kinds_and_mismatches();
    test_all_action_kinds_parse_after_writer_acceptance();
    test_replay_import_failure_is_atomic();
    test_serializer_rejects_noncanonical_inputs();
    test_stale_and_duplicate_commit_rejected();
    puts("FCJ4 serializer/state unit tests passed");
    return 0;
}
