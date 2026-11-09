#include "journal/fcj4_parser.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static const char *opid = "11111111111111111111111111111111";
static const char *actionid = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
static const char *actionid2 = "22222222222222222222222222222222";
static const char *actionid3 = "33333333333333333333333333333333";
static const char *actionid4 = "44444444444444444444444444444444";
static const char *digest =
    "0000000000000000000000000000000000000000000000000000000000000000";

static size_t make_valid(char *out, size_t capacity)
{
    size_t used=0U;
#define APPEND(...) do { \
    int n=snprintf(out+used,capacity-used,__VA_ARGS__); \
    assert(n>0 && (size_t)n<capacity-used); \
    used+=(size_t)n; \
} while (0)
    APPEND("FCJ4|BEGIN|1|%s|11|2f746d702f736f75726365|1|2|1|3|REGULAR|1000|1000|384|5|1|2|3|4|9|2f746d702f64657374|1|4|4|64657374|1|5\n",opid);
    APPEND("FCJ4|SEALED|1|%s|9|2e66632d746d702d78|1|6|1000|384|5|%s\n",opid,digest);
    APPEND("FCJ4|MOVE|1|%s|1|STAGED|0\n",opid);
    APPEND("FCJ4|MOVE|1|%s|2|STARTED|0\n",opid);
    APPEND("FCJ4|MOVE|1|%s|3|PUBLISHED|0\n",opid);
    APPEND("FCJ4|MOVE|1|%s|4|COMPLETED|0\n",opid);
    APPEND("FCJ4|ACTION_DEF|1|%s|%s|QUARANTINE_STAGE|QUARANTINE|1|4|1|6|REGULAR|1000|384|5|%s|9|2e66632d746d702d78|7|712d7374616765\n",opid,actionid,digest);
    APPEND("FCJ4|ACTION_DEF|1|%s|%s|QUARANTINE_SOURCE|QUARANTINE|1|2|1|3|REGULAR|1000|384|5|%s|6|736f75726365|8|712d736f75726365\n",opid,actionid2,digest);
    APPEND("FCJ4|ACTION_DEF|1|%s|%s|RETIRE_QUARANTINED_SOURCE|NONE|1|5|1|3|REGULAR|1000|384|5|%s|8|712d736f75726365|0|-\n",opid,actionid3,digest);
    APPEND("FCJ4|ACTION_DEF|1|%s|%s|REMOVE_QUARANTINED_STAGE|NONE|1|5|1|6|REGULAR|1000|384|5|%s|7|712d7374616765|0|-\n",opid,actionid4,digest);
    APPEND("FCJ4|ACTION|1|%s|%s|1|ACTION_PREPARED|0\n",opid,actionid);
    APPEND("FCJ4|ACTION|1|%s|%s|2|ACTION_STARTED|0\n",opid,actionid);
    APPEND("FCJ4|ACTION|1|%s|%s|3|ACTION_APPLIED|0\n",opid,actionid);
    APPEND("FCJ4|ACTION|1|%s|%s|4|ACTION_SYNCED|0\n",opid,actionid);
    APPEND("FCJ4|ACTION|1|%s|%s|5|ACTION_COMPLETED|0\n",opid,actionid);
#undef APPEND
    return used;
}

static void test_valid_complete_replay(void)
{
    char bytes[8192];
    size_t length=make_valid(bytes,sizeof(bytes));
    Fcj4Replay replay;
    Fcj4OperationView view;
    fcj4_replay_init(&replay);
    if (fcj4_replay_parse(bytes,length,&replay)!=0) {
        size_t offset=0U, line_number=0U;
        while (offset<length) {
            const char *newline=memchr(bytes+offset,'\n',length-offset);
            Fcj4Replay prefix;
            assert(newline!=NULL);
            offset=(size_t)(newline-bytes)+1U;
            line_number++;
            fcj4_replay_init(&prefix);
            if (fcj4_replay_parse(bytes,offset,&prefix)!=0) {
                fprintf(stderr,"FCJ4 rejected prefix ending at record %zu (errno=%d)\n",
                        line_number,errno);
                fcj4_replay_destroy(&prefix);
                break;
            }
            fcj4_replay_destroy(&prefix);
        }
        assert(!"valid FCJ4 fixture was rejected");
    }
    assert(replay.count==1U);
    assert(replay.record_count==15U);
    assert(fcj4_replay_operation(&replay,0U,&view)==0);
    assert(strcmp(view.operation_id,opid)==0);
    assert(view.action_count==4U);
    assert(view.move_state==FCJ4_MOVE_COMPLETED);
    assert(fcj4_replay_operation(&replay,1U,&view)==-1);
    fcj4_replay_destroy(&replay);
    assert(replay.operations==NULL && replay.count==0U);
}

static void test_all_or_nothing_tail(void)
{
    char bytes[8192];
    size_t length=make_valid(bytes,sizeof(bytes));
    Fcj4Replay replay;
    memcpy(bytes+length,"FCJ4|UNKNOWN|1|x\n",17U);
    fcj4_replay_init(&replay);
    errno=0;
    assert(fcj4_replay_parse(bytes,length+17U,&replay)==-1);
    assert(errno==EBADMSG);
    assert(replay.operations==NULL && replay.count==0U &&
           replay.record_count==0U && replay.capacity==0U);
}

static void test_rejects_truncation_and_empty(void)
{
    char bytes[8192];
    size_t length=make_valid(bytes,sizeof(bytes));
    Fcj4Replay replay;
    fcj4_replay_init(&replay);
    errno=0;
    assert(fcj4_replay_parse(bytes,length-1U,&replay)==-1);
    assert(errno==EBADMSG);
    errno=0;
    assert(fcj4_replay_parse(bytes,0U,&replay)==-1);
    assert(errno==EBADMSG);
}

static void test_rejects_illegal_move_transition(void)
{
    char bytes[8192];
    size_t length=make_valid(bytes,sizeof(bytes));
    char *started=strstr(bytes,"|1|STAGED|0\n");
    Fcj4Replay replay;
    assert(started!=NULL);
    memcpy(started,"|1|FAILED|1\n",12U);
    fcj4_replay_init(&replay);
    errno=0;
    assert(fcj4_replay_parse(bytes,length,&replay)==-1);
    assert(errno==EBADMSG);
    assert(replay.operations==NULL);
}

static void test_rejects_noncanonical_and_unknown(void)
{
    const char noncanonical[]="FCJ4|BEGIN|1|11111111111111111111111111111111|01\n";
    const char unknown[]="FCJ4|NOPE|1|x\n";
    Fcj4Replay replay;
    fcj4_replay_init(&replay);
    errno=0;
    assert(fcj4_replay_parse(noncanonical,sizeof(noncanonical)-1U,&replay)==-1);
    assert(errno==EBADMSG);
    errno=0;
    assert(fcj4_replay_parse(unknown,sizeof(unknown)-1U,&replay)==-1);
    assert(errno==EBADMSG);
}


static void test_rejects_action_identity_mismatch(void)
{
    char bytes[8192];
    size_t length=make_valid(bytes,sizeof(bytes));
    char *record=strstr(bytes,"QUARANTINE_STAGE|QUARANTINE|1|4|1|6|REGULAR");
    Fcj4Replay replay;
    assert(record!=NULL);
    record=strstr(record,"|1|6|REGULAR");
    assert(record!=NULL);
    memcpy(record,"|1|7|REGULAR",12U);
    fcj4_replay_init(&replay);
    errno=0;
    assert(fcj4_replay_parse(bytes,length,&replay)==-1);
    assert(errno==EBADMSG);
    assert(replay.operations==NULL);
}

static void test_rejects_action_parent_and_sentinel_mismatch(void)
{
    char bytes[8192];
    size_t length=make_valid(bytes,sizeof(bytes));
    char *record=strstr(bytes,"QUARANTINE_STAGE|QUARANTINE|1|4|");
    Fcj4Replay replay;
    assert(record!=NULL);
    record=strstr(record,"|1|4|");
    assert(record!=NULL);
    memcpy(record,"|1|2|",5U);
    fcj4_replay_init(&replay);
    errno=0;
    assert(fcj4_replay_parse(bytes,length,&replay)==-1);
    assert(errno==EBADMSG);
    assert(replay.operations==NULL);

    length=make_valid(bytes,sizeof(bytes));
    record=strstr(bytes,"RETIRE_QUARANTINED_SOURCE|NONE");
    assert(record!=NULL);
    record=strstr(record,"|0|-\n");
    assert(record!=NULL);
    memcpy(record,"|0|0\n",5U);
    fcj4_replay_init(&replay);
    errno=0;
    assert(fcj4_replay_parse(bytes,length,&replay)==-1);
    assert(errno==EBADMSG);
    assert(replay.operations==NULL);
}


static void test_rejects_unbound_quarantine_followup(void)
{
    char bytes[8192];
    size_t length=make_valid(bytes,sizeof(bytes));
    char *record=strstr(bytes,"RETIRE_QUARANTINED_SOURCE|NONE");
    Fcj4Replay replay;
    assert(record!=NULL);
    record=strstr(record,"|8|712d736f75726365|0|-");
    assert(record!=NULL);
    memcpy(record,"|8|712d6f7468657273|0|-",23U);
    fcj4_replay_init(&replay);
    errno=0;
    assert(fcj4_replay_parse(bytes,length,&replay)==-1);
    assert(errno==EBADMSG);
    assert(replay.operations==NULL);
}

static void test_rejects_resource_and_framing_limits(void)
{
    char byte='x';
    char oversized[FCJ4_MAX_LINE_BYTES+1U];
    Fcj4Replay replay;
    fcj4_replay_init(&replay);
    errno=0;
    assert(fcj4_replay_parse(&byte,(size_t)FCJ4_MAX_JOURNAL_BYTES+1U,&replay)==-1);
    assert(errno==E2BIG);

    memset(oversized,'A',sizeof(oversized));
    oversized[sizeof(oversized)-1U]='\n';
    errno=0;
    assert(fcj4_replay_parse(oversized,sizeof(oversized),&replay)==-1);
    assert(errno==E2BIG);

    {
        const char crlf[]="FCJ4|UNKNOWN|1|x\r\n";
        errno=0;
        assert(fcj4_replay_parse(crlf,sizeof(crlf)-1U,&replay)==-1);
        assert(errno==EBADMSG);
    }
}

int main(void)
{
    test_valid_complete_replay();
    test_all_or_nothing_tail();
    test_rejects_truncation_and_empty();
    test_rejects_illegal_move_transition();
    test_rejects_noncanonical_and_unknown();
    test_rejects_action_identity_mismatch();
    test_rejects_action_parent_and_sentinel_mismatch();
    test_rejects_resource_and_framing_limits();
    test_rejects_unbound_quarantine_followup();
    puts("FCJ4 parser/replay unit tests passed");
    return 0;
}
