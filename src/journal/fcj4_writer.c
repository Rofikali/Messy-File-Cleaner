#define _POSIX_C_SOURCE 200809L

#include "journal/fcj4_writer.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Fcj4ActionDefRecord record;
    char source_component[NAME_MAX];
    char target_component[NAME_MAX];
    char digest[65];
    char id[33];
    Fcj4ActionState state;
    uint64_t sequence;
} WriterAction;

typedef struct {
    char id[33];
    Fcj4BeginRecord begin;
    char source_path[PATH_MAX];
    char destination_path[PATH_MAX];
    char destination_basename[NAME_MAX];
    Fcj4SealedRecord sealed;
    char stage_component[NAME_MAX];
    char stage_digest[65];
    Fcj4MoveState move_state;
    uint64_t move_sequence;
    int has_begin, has_sealed;
    WriterAction actions[FCJ4_MAX_ACTIONS_PER_OPERATION];
    size_t action_count;
} WriterOperation;

struct Fcj4WriterState {
    WriterOperation *operations;
    size_t operation_count;
    uint64_t revision;
    uint64_t next_token;
    int pending;
    Fcj4PreparedRecord prepared;
    Fcj4RecordKind pending_kind;
    Fcj4MoveState pending_move;
    Fcj4ActionState pending_action;
    WriterOperation pending_evidence;
    char pending_bytes[FCJ4_MAX_LINE_BYTES];
    size_t pending_length;
};

typedef struct {
    char *data;
    size_t capacity;
    size_t used;
    int failed;
} Buffer;

static int fail(int error) { errno = error; return -1; }

static int is_lower_hex(const char *s, size_t expected)
{
    if (!s || strnlen(s,expected+1U) != expected) return 0;
    for (size_t i=0U; i<expected; ++i) {
        if (!((s[i]>='0' && s[i]<='9') || (s[i]>='a' && s[i]<='f')))
            return 0;
    }
    return 1;
}

static int valid_component(const char *s)
{
    size_t n;
    if (!s || !s[0]) return 0;
    n=strnlen(s,NAME_MAX);
    if (n==0U || n>=NAME_MAX || (n==1U && s[0]=='.') ||
        (n==2U && s[0]=='.' && s[1]=='.')) return 0;
    for (size_t i=0U;i<n;i++) {
        unsigned char c=(unsigned char)s[i];
        if (c=='/' || c<0x20U || c==0x7fU) return 0;
    }
    return 1;
}

static int valid_absolute_path(const char *s)
{
    const char *p;
    size_t n;
    if (!s || s[0]!='/' || !s[1]) return 0;
    n=strnlen(s,PATH_MAX);
    if (n>=PATH_MAX) return 0;
    p=s+1;
    while (*p) {
        const char *start=p;
        while (*p && *p!='/') {
            unsigned char c=(unsigned char)*p;
            if (c<0x20U || c==0x7fU) return 0;
            ++p;
        }
        if (p==start || ((size_t)(p-start)==1U && start[0]=='.') ||
            ((size_t)(p-start)==2U && start[0]=='.' && start[1]=='.'))
            return 0;
        if (*p=='/') {
            ++p;
            if (!*p) return 0;
        }
    }
    return 1;
}

static const char *base_name(const char *path)
{
    const char *p=strrchr(path,'/');
    return p ? p+1 : path;
}

static void append_bytes(Buffer *b, const char *s, size_t n)
{
    if (b->failed) return;
    if (n>b->capacity-b->used) { b->failed=1; return; }
    memcpy(b->data+b->used,s,n);
    b->used+=n;
}

static void append_text(Buffer *b, const char *s)
{
    append_bytes(b,s,strlen(s));
}

static void append_u64(Buffer *b, uint64_t value)
{
    char tmp[32];
    int n=snprintf(tmp,sizeof(tmp),"%" PRIu64,value);
    if (n<0 || (size_t)n>=sizeof(tmp)) { b->failed=1; return; }
    append_bytes(b,tmp,(size_t)n);
}

static void append_i64(Buffer *b, int64_t value)
{
    char tmp[32];
    int n=snprintf(tmp,sizeof(tmp),"%" PRId64,value);
    if (n<0 || (size_t)n>=sizeof(tmp)) { b->failed=1; return; }
    append_bytes(b,tmp,(size_t)n);
}

static void field_sep(Buffer *b) { append_text(b,"|"); }

static void append_hex(Buffer *b, const char *s)
{
    static const char digits[]="0123456789abcdef";
    size_t n=strlen(s);
    for (size_t i=0U;i<n;i++) {
        unsigned char c=(unsigned char)s[i];
        char pair[2]={digits[c>>4],digits[c&15U]};
        append_bytes(b,pair,sizeof(pair));
    }
}

static void append_string_field(Buffer *b, const char *s)
{
    append_u64(b,(uint64_t)strlen(s));
    field_sep(b);
    append_hex(b,s);
}

static void begin_record(Buffer *b, const char *kind)
{
    append_text(b,"FCJ4|");
    append_text(b,kind);
    append_text(b,"|1");
}

static void next_field(Buffer *b) { field_sep(b); }

static int valid_move_transition(Fcj4MoveState old, Fcj4MoveState next,
                                 uint32_t error)
{
    if ((next==FCJ4_MOVE_FAILED) ? error==0U : error!=0U) return 0;
    return (old==FCJ4_MOVE_NONE && next==FCJ4_MOVE_STAGED) ||
           (old==FCJ4_MOVE_STAGED &&
              (next==FCJ4_MOVE_STARTED || next==FCJ4_MOVE_FAILED)) ||
           (old==FCJ4_MOVE_STARTED &&
              (next==FCJ4_MOVE_PUBLISHED || next==FCJ4_MOVE_FAILED)) ||
           (old==FCJ4_MOVE_PUBLISHED && next==FCJ4_MOVE_COMPLETED);
}

static const char *move_name(Fcj4MoveState state)
{
    switch(state) {
    case FCJ4_MOVE_STAGED: return "STAGED";
    case FCJ4_MOVE_STARTED: return "STARTED";
    case FCJ4_MOVE_PUBLISHED: return "PUBLISHED";
    case FCJ4_MOVE_COMPLETED: return "COMPLETED";
    case FCJ4_MOVE_FAILED: return "FAILED";
    default: return NULL;
    }
}

static const char *action_name(Fcj4ActionState state)
{
    switch(state) {
    case FCJ4_ACTION_PREPARED: return "ACTION_PREPARED";
    case FCJ4_ACTION_STARTED: return "ACTION_STARTED";
    case FCJ4_ACTION_APPLIED: return "ACTION_APPLIED";
    case FCJ4_ACTION_SYNCED: return "ACTION_SYNCED";
    case FCJ4_ACTION_COMPLETED: return "ACTION_COMPLETED";
    case FCJ4_ACTION_MANUAL_REVIEW: return "ACTION_MANUAL_REVIEW";
    default: return NULL;
    }
}

static const char *action_kind_name(Fcj4ActionKind kind)
{
    switch(kind) {
    case FCJ4_ACTION_KIND_QUARANTINE_STAGE: return "QUARANTINE_STAGE";
    case FCJ4_ACTION_KIND_QUARANTINE_SOURCE: return "QUARANTINE_SOURCE";
    case FCJ4_ACTION_KIND_RETIRE_QUARANTINED_SOURCE: return "RETIRE_QUARANTINED_SOURCE";
    case FCJ4_ACTION_KIND_REMOVE_QUARANTINED_STAGE: return "REMOVE_QUARANTINED_STAGE";
    default: return NULL;
    }
}

static const char *role_name(Fcj4TargetRole role)
{
    if (role==FCJ4_TARGET_QUARANTINE) return "QUARANTINE";
    if (role==FCJ4_TARGET_NONE) return "NONE";
    return NULL;
}

static int valid_action_transition(Fcj4ActionState old, Fcj4ActionState next,
                                   uint32_t error)
{
    if ((next==FCJ4_ACTION_MANUAL_REVIEW) ? error==0U : error!=0U) return 0;
    return (old==FCJ4_ACTION_NONE && next==FCJ4_ACTION_PREPARED) ||
           (old==FCJ4_ACTION_PREPARED &&
              (next==FCJ4_ACTION_STARTED || next==FCJ4_ACTION_MANUAL_REVIEW)) ||
           (old==FCJ4_ACTION_STARTED &&
              (next==FCJ4_ACTION_APPLIED || next==FCJ4_ACTION_MANUAL_REVIEW)) ||
           (old==FCJ4_ACTION_APPLIED &&
              (next==FCJ4_ACTION_SYNCED || next==FCJ4_ACTION_MANUAL_REVIEW)) ||
           (old==FCJ4_ACTION_SYNCED &&
              (next==FCJ4_ACTION_COMPLETED || next==FCJ4_ACTION_MANUAL_REVIEW));
}

static int serialize_begin(Buffer *b, const Fcj4BeginRecord *r)
{
    if (!r || !is_lower_hex(r->operation_id,32U) ||
        !valid_absolute_path(r->source_path) ||
        !valid_absolute_path(r->destination_path) ||
        !valid_component(r->destination_basename) ||
        strcmp(base_name(r->destination_path),r->destination_basename)!=0 ||
        !valid_component(base_name(r->source_path)) ||
        r->source_ino==0U || r->source_parent_ino==0U ||
        r->destination_parent_ino==0U || r->quarantine_parent_ino==0U ||
        r->source_mtime_nsec>999999999U || r->source_ctime_nsec>999999999U)
        return -1;
    begin_record(b,"BEGIN");
#define F(name) next_field(b); append_text(b,(name))
#define U(name) next_field(b); append_u64(b,(name))
    F(r->operation_id); next_field(b); append_string_field(b,r->source_path);
    U(r->source_parent_dev); U(r->source_parent_ino); U(r->source_dev);
    U(r->source_ino); F("REGULAR"); U(r->source_uid); U(r->source_gid);
    U(r->source_mode); U(r->source_size); next_field(b); append_i64(b,r->source_mtime_sec);
    U(r->source_mtime_nsec); next_field(b); append_i64(b,r->source_ctime_sec);
    U(r->source_ctime_nsec); next_field(b); append_string_field(b,r->destination_path);
    U(r->destination_parent_dev); U(r->destination_parent_ino);
    next_field(b); append_string_field(b,r->destination_basename);
    U(r->quarantine_parent_dev); U(r->quarantine_parent_ino);
#undef F
#undef U
    return 0;
}

static int serialize_sealed(Buffer *b, const Fcj4SealedRecord *r)
{
    size_t n;
    if (!r || !is_lower_hex(r->operation_id,32U) ||
        !valid_component(r->stage_component) ||
        !is_lower_hex(r->sha256,64U) || r->stage_ino==0U ||
        r->stage_dev==0U) return -1;
    n=strlen(r->stage_component);
    if (n<=8U || strncmp(r->stage_component,".fc-tmp-",8U)!=0) return -1;
    begin_record(b,"SEALED");
    next_field(b); append_text(b,r->operation_id);
    next_field(b); append_string_field(b,r->stage_component);
    next_field(b); append_u64(b,r->stage_dev);
    next_field(b); append_u64(b,r->stage_ino);
    next_field(b); append_u64(b,r->stage_uid);
    next_field(b); append_u64(b,r->stage_mode);
    next_field(b); append_u64(b,r->stage_size);
    next_field(b); append_text(b,r->sha256);
    return 0;
}

static int serialize_move(Buffer *b, const Fcj4MoveRecord *r)
{
    const char *name;
    if (!r || !is_lower_hex(r->operation_id,32U) ||
        r->sequence==0U || r->error_code>(uint32_t)INT_MAX ||
        !(name=move_name(r->state)) ||
        ((r->state==FCJ4_MOVE_FAILED) ? r->error_code==0U : r->error_code!=0U))
        return -1;
    begin_record(b,"MOVE");
    next_field(b); append_text(b,r->operation_id);
    next_field(b); append_u64(b,r->sequence);
    next_field(b); append_text(b,name);
    next_field(b); append_u64(b,r->error_code);
    return 0;
}

static int serialize_action_def(Buffer *b, const Fcj4ActionDefRecord *r)
{
    const char *kind, *role;
    if (!r || !is_lower_hex(r->operation_id,32U) ||
        !is_lower_hex(r->action_id,32U) ||
        !(kind=action_kind_name(r->kind)) || !(role=role_name(r->target_role)) ||
        !valid_component(r->source_component) ||
        !is_lower_hex(r->expected_sha256,64U) ||
        r->expected_parent_ino==0U || r->expected_entry_ino==0U)
        return -1;
    if ((r->kind==FCJ4_ACTION_KIND_QUARANTINE_STAGE ||
         r->kind==FCJ4_ACTION_KIND_QUARANTINE_SOURCE) !=
        (r->target_role==FCJ4_TARGET_QUARANTINE)) return -1;
    if (r->target_role==FCJ4_TARGET_QUARANTINE) {
        if (!valid_component(r->target_component)) return -1;
    } else if (r->target_component!=NULL) return -1;
    begin_record(b,"ACTION_DEF");
    next_field(b); append_text(b,r->operation_id);
    next_field(b); append_text(b,r->action_id);
    next_field(b); append_text(b,kind);
    next_field(b); append_text(b,role);
    next_field(b); append_u64(b,r->expected_parent_dev);
    next_field(b); append_u64(b,r->expected_parent_ino);
    next_field(b); append_u64(b,r->expected_entry_dev);
    next_field(b); append_u64(b,r->expected_entry_ino);
    next_field(b); append_text(b,"REGULAR");
    next_field(b); append_u64(b,r->expected_uid);
    next_field(b); append_u64(b,r->expected_mode);
    next_field(b); append_u64(b,r->expected_size);
    next_field(b); append_text(b,r->expected_sha256);
    next_field(b); append_string_field(b,r->source_component);
    if (r->target_role==FCJ4_TARGET_QUARANTINE) {
        next_field(b); append_string_field(b,r->target_component);
    } else {
        next_field(b); append_text(b,"0");
        next_field(b); append_text(b,"-");
    }
    return 0;
}

static int serialize_action(Buffer *b, const Fcj4ActionRecord *r)
{
    const char *name;
    if (!r || !is_lower_hex(r->operation_id,32U) ||
        !is_lower_hex(r->action_id,32U) || r->sequence==0U ||
        r->error_code>(uint32_t)INT_MAX ||
        !(name=action_name(r->state)) ||
        ((r->state==FCJ4_ACTION_MANUAL_REVIEW) ?
          r->error_code==0U : r->error_code!=0U)) return -1;
    begin_record(b,"ACTION");
    next_field(b); append_text(b,r->operation_id);
    next_field(b); append_text(b,r->action_id);
    next_field(b); append_u64(b,r->sequence);
    next_field(b); append_text(b,name);
    next_field(b); append_u64(b,r->error_code);
    return 0;
}

int fcj4_serialize(const Fcj4Record *record, char *output,
                   size_t capacity, size_t *written)
{
    Buffer b;
    int rc=-1;
    if (written) *written=0U;
    if (!record || !output || !written || capacity==0U) return fail(EINVAL);
    b.data=output; b.capacity=capacity; b.used=0U; b.failed=0;
    switch(record->kind) {
    case FCJ4_RECORD_BEGIN: rc=serialize_begin(&b,&record->value.begin); break;
    case FCJ4_RECORD_SEALED: rc=serialize_sealed(&b,&record->value.sealed); break;
    case FCJ4_RECORD_MOVE: rc=serialize_move(&b,&record->value.move); break;
    case FCJ4_RECORD_ACTION_DEF: rc=serialize_action_def(&b,&record->value.action_def); break;
    case FCJ4_RECORD_ACTION: rc=serialize_action(&b,&record->value.action); break;
    default: return fail(EINVAL);
    }
    if (rc!=0) return fail(EINVAL);
    append_text(&b,"\n");
    if (b.failed) return fail(ENOSPC);
    if (b.used>FCJ4_MAX_LINE_BYTES) return fail(E2BIG);
    *written=b.used;
    return 0;
}

static WriterOperation *find_op(Fcj4WriterState *s,const char *id)
{
    for(size_t i=0U;i<s->operation_count;i++)
        if(strcmp(s->operations[i].id,id)==0) return &s->operations[i];
    return NULL;
}

static size_t find_action(const WriterOperation *op,const char *id)
{
    for(size_t i=0U;i<op->action_count;i++)
        if(strcmp(op->actions[i].id,id)==0) return i;
    return SIZE_MAX;
}

static int copy_text(char *dst,size_t capacity,const char *src)
{
    size_t n;
    if(!dst || !src || capacity==0U) return -1;
    n=strnlen(src,capacity);
    if(n==0U || n>=capacity) return -1;
    memcpy(dst,src,n+1U);
    return 0;
}

static int copy_begin(WriterOperation *dst,const Fcj4BeginRecord *src)
{
    if(!dst || !src || copy_text(dst->source_path,PATH_MAX,src->source_path) ||
       copy_text(dst->destination_path,PATH_MAX,src->destination_path) ||
       copy_text(dst->destination_basename,NAME_MAX,src->destination_basename))
        return -1;
    dst->begin=*src;
    dst->begin.source_path=dst->source_path;
    dst->begin.destination_path=dst->destination_path;
    dst->begin.destination_basename=dst->destination_basename;
    memcpy(dst->id,src->operation_id,33U);
    dst->begin.operation_id=dst->id;
    dst->has_begin=1;
    return 0;
}

static int copy_sealed(WriterOperation *dst,const Fcj4SealedRecord *src)
{
    if(!dst || !src ||
       copy_text(dst->stage_component,NAME_MAX,src->stage_component) ||
       copy_text(dst->stage_digest,sizeof(dst->stage_digest),src->sha256))
        return -1;
    dst->sealed=*src;
    dst->sealed.operation_id=dst->id;
    dst->sealed.stage_component=dst->stage_component;
    dst->sealed.sha256=dst->stage_digest;
    dst->has_sealed=1;
    return 0;
}

static int copy_action_def(WriterOperation *dst,size_t index,
                           const Fcj4ActionDefRecord *src)
{
    WriterAction *a;
    if(!dst || !src || index>=FCJ4_MAX_ACTIONS_PER_OPERATION) return -1;
    a=&dst->actions[index];
    if(copy_text(a->source_component,NAME_MAX,src->source_component) ||
       copy_text(a->digest,sizeof(a->digest),src->expected_sha256))
        return -1;
    if(src->target_role==FCJ4_TARGET_QUARANTINE &&
       copy_text(a->target_component,NAME_MAX,src->target_component))
        return -1;
    a->record=*src;
    a->record.operation_id=dst->id;
    a->record.action_id=a->id;
    a->record.source_component=a->source_component;
    a->record.target_component=src->target_role==FCJ4_TARGET_QUARANTINE
        ? a->target_component : NULL;
    a->record.expected_sha256=a->digest;
    memcpy(a->id,src->action_id,33U);
    a->record.action_id=a->id;
    return 0;
}

static int validate_action_evidence(const WriterOperation *op,
                                    const Fcj4ActionDefRecord *r)
{
    const char *expected_component=NULL;
    uint64_t parent_dev,parent_ino,entry_dev,entry_ino,uid,mode,size;
    if(!op || !r || !op->has_begin || !op->has_sealed ||
       !r->source_component || !is_lower_hex(r->expected_sha256,64U))
        return 0;
    switch(r->kind) {
    case FCJ4_ACTION_KIND_QUARANTINE_STAGE:
        parent_dev=op->begin.destination_parent_dev;
        parent_ino=op->begin.destination_parent_ino;
        entry_dev=op->sealed.stage_dev; entry_ino=op->sealed.stage_ino;
        uid=op->sealed.stage_uid; mode=op->sealed.stage_mode;
        size=op->sealed.stage_size; expected_component=op->stage_component;
        break;
    case FCJ4_ACTION_KIND_QUARANTINE_SOURCE:
        parent_dev=op->begin.source_parent_dev;
        parent_ino=op->begin.source_parent_ino;
        entry_dev=op->begin.source_dev; entry_ino=op->begin.source_ino;
        uid=op->begin.source_uid; mode=op->begin.source_mode;
        size=op->begin.source_size;
        expected_component=base_name(op->begin.source_path);
        break;
    case FCJ4_ACTION_KIND_RETIRE_QUARANTINED_SOURCE:
    case FCJ4_ACTION_KIND_REMOVE_QUARANTINED_STAGE: {
        Fcj4ActionKind required = r->kind==FCJ4_ACTION_KIND_RETIRE_QUARANTINED_SOURCE
            ? FCJ4_ACTION_KIND_QUARANTINE_SOURCE
            : FCJ4_ACTION_KIND_QUARANTINE_STAGE;
        size_t i;
        for(i=0U;i<op->action_count;i++)
            if(op->actions[i].record.kind==required &&
               op->actions[i].record.target_component &&
               strcmp(op->actions[i].record.target_component,r->source_component)==0)
                break;
        if(i==op->action_count) return 0;
        parent_dev=op->begin.quarantine_parent_dev;
        parent_ino=op->begin.quarantine_parent_ino;
        if(required==FCJ4_ACTION_KIND_QUARANTINE_SOURCE) {
            entry_dev=op->begin.source_dev; entry_ino=op->begin.source_ino;
            uid=op->begin.source_uid; mode=op->begin.source_mode;
            size=op->begin.source_size;
        } else {
            entry_dev=op->sealed.stage_dev; entry_ino=op->sealed.stage_ino;
            uid=op->sealed.stage_uid; mode=op->sealed.stage_mode;
            size=op->sealed.stage_size;
        }
        break;
    }
    default: return 0;
    }
    if(r->expected_parent_dev!=parent_dev ||
       r->expected_parent_ino!=parent_ino ||
       r->expected_entry_dev!=entry_dev ||
       r->expected_entry_ino!=entry_ino ||
       r->expected_uid!=uid || r->expected_mode!=mode ||
       r->expected_size!=size ||
       strcmp(r->expected_sha256,op->stage_digest)!=0)
        return 0;
    if(expected_component &&
       strcmp(r->source_component,expected_component)!=0)
        return 0;
    return 1;
}

static int validate_next(Fcj4WriterState *s,const Fcj4Record *r,
                         Fcj4PreparedRecord *p,
                         Fcj4MoveState *move, Fcj4ActionState *action)
{
    const char *opid=NULL,*actionid=NULL;
    WriterOperation *op;
    uint64_t seq=0U;
    *move=FCJ4_MOVE_NONE; *action=FCJ4_ACTION_NONE;
    switch(r->kind) {
    case FCJ4_RECORD_BEGIN: opid=r->value.begin.operation_id; break;
    case FCJ4_RECORD_SEALED: opid=r->value.sealed.operation_id; break;
    case FCJ4_RECORD_MOVE: opid=r->value.move.operation_id; seq=r->value.move.sequence; *move=r->value.move.state; break;
    case FCJ4_RECORD_ACTION_DEF: opid=r->value.action_def.operation_id; actionid=r->value.action_def.action_id; break;
    case FCJ4_RECORD_ACTION: opid=r->value.action.operation_id; actionid=r->value.action.action_id; seq=r->value.action.sequence; *action=r->value.action.state; break;
    default: return fail(EINVAL);
    }
    if(!opid || !is_lower_hex(opid,32U)) return fail(EINVAL);
    memcpy(p->operation_id,opid,33U);
    op=find_op(s,opid);
    if(r->kind==FCJ4_RECORD_BEGIN) {
        if(op || s->operation_count>=FCJ4_MAX_OPERATIONS) return fail(EINVAL);
    } else if(!op || !op->has_begin) return fail(EINVAL);
    if(r->kind==FCJ4_RECORD_SEALED) {
        if(op->has_sealed ||
           r->value.sealed.stage_dev!=op->begin.destination_parent_dev ||
           r->value.sealed.stage_size!=op->begin.source_size)
            return fail(EINVAL);
    } else if(r->kind!=FCJ4_RECORD_BEGIN && !op->has_sealed) return fail(EINVAL);
    if(r->kind==FCJ4_RECORD_MOVE) {
        if(seq!=op->move_sequence+1U ||
           !valid_move_transition(op->move_state,*move,r->value.move.error_code))
            return fail(EINVAL);
    }
    if(r->kind==FCJ4_RECORD_ACTION_DEF) {
        if(!is_lower_hex(actionid,32U) ||
           op->action_count>=FCJ4_MAX_ACTIONS_PER_OPERATION ||
           find_action(op,actionid)!=SIZE_MAX) return fail(EINVAL);
        if(!validate_action_evidence(op,&r->value.action_def))
            return fail(EINVAL);
        memcpy(p->action_id,actionid,33U);
    }
    if(r->kind==FCJ4_RECORD_ACTION) {
        size_t ai;
        if(!is_lower_hex(actionid,32U) || (ai=find_action(op,actionid))==SIZE_MAX ||
           seq!=op->actions[ai].sequence+1U ||
           !valid_action_transition(op->actions[ai].state,*action,
                                    r->value.action.error_code)) return fail(EINVAL);
        memcpy(p->action_id,actionid,33U);
    }
    p->kind=r->kind;
    p->sequence=seq;
    return 0;
}

int fcj4_writer_state_create(Fcj4WriterState **out)
{
    Fcj4WriterState *s;
    if(!out) return fail(EINVAL);
    *out=NULL;
    s=(Fcj4WriterState *)calloc(1U,sizeof(*s));
    if(!s) return fail(ENOMEM);
    s->operations=(WriterOperation *)calloc(FCJ4_MAX_OPERATIONS,sizeof(WriterOperation));
    if(!s->operations) { free(s); return fail(ENOMEM); }
    s->next_token=1U;
    *out=s;
    return 0;
}

void fcj4_writer_state_destroy(Fcj4WriterState *s)
{
    if(!s) return;
    free(s->operations);
    memset(s,0,sizeof(*s));
    free(s);
}

int fcj4_writer_state_import_replay(Fcj4WriterState *s,
                                    const Fcj4Replay *replay)
{
    if(!s || !replay || s->pending || s->revision!=0U ||
       s->operation_count!=0U || replay->count>FCJ4_MAX_OPERATIONS ||
       replay->capacity>FCJ4_MAX_OPERATIONS ||
       replay->count>replay->capacity ||
       replay->record_count>FCJ4_MAX_RECORDS ||
       replay->record_count<replay->count ||
       (replay->count!=0U && !replay->operations))
        return fail(EINVAL);
    for(size_t i=0U;i<replay->count;i++) {
        Fcj4OperationView v;
        WriterOperation *op=&s->operations[i];
        Fcj4BeginRecord begin;
        if(fcj4_replay_operation(replay,i,&v)!=0) goto invalid;
        memset(&begin,0,sizeof(begin));
        begin.operation_id=v.operation_id;
        begin.source_path=v.source_path;
        begin.source_parent_dev=v.source_parent_dev;
        begin.source_parent_ino=v.source_parent_ino;
        begin.source_dev=v.source_dev; begin.source_ino=v.source_ino;
        begin.source_uid=v.source_uid; begin.source_gid=v.source_gid;
        begin.source_mode=v.source_mode; begin.source_size=v.source_size;
        begin.source_mtime_sec=v.source_mtime_sec;
        begin.source_mtime_nsec=v.source_mtime_nsec;
        begin.source_ctime_sec=v.source_ctime_sec;
        begin.source_ctime_nsec=v.source_ctime_nsec;
        begin.destination_path=v.destination_path;
        begin.destination_parent_dev=v.destination_parent_dev;
        begin.destination_parent_ino=v.destination_parent_ino;
        begin.destination_basename=v.destination_basename;
        begin.quarantine_parent_dev=v.quarantine_parent_dev;
        begin.quarantine_parent_ino=v.quarantine_parent_ino;
        if(copy_begin(op,&begin)!=0) goto invalid;
        op->move_state=v.move_state;
        op->move_sequence=v.move_sequence;
        if(v.stage_component) {
            Fcj4SealedRecord sealed;
            memset(&sealed,0,sizeof(sealed));
            sealed.operation_id=v.operation_id;
            sealed.stage_component=v.stage_component;
            sealed.stage_dev=v.stage_dev; sealed.stage_ino=v.stage_ino;
            sealed.stage_uid=v.stage_uid; sealed.stage_mode=v.stage_mode;
            sealed.stage_size=v.stage_size; sealed.sha256=v.stage_sha256;
            if(copy_sealed(op,&sealed)!=0) goto invalid;
        }
        for(size_t j=0U;j<v.action_count;j++) {
            Fcj4ActionView av;
            Fcj4ActionDefRecord def;
            WriterAction *wa;
            if(fcj4_replay_action(replay,i,j,&av)!=0) goto invalid;
            memset(&def,0,sizeof(def));
            def.operation_id=v.operation_id;
            def.action_id=av.action_id;
            def.kind=av.kind; def.target_role=av.target_role;
            def.expected_parent_dev=av.expected_parent_dev;
            def.expected_parent_ino=av.expected_parent_ino;
            def.expected_entry_dev=av.expected_entry_dev;
            def.expected_entry_ino=av.expected_entry_ino;
            def.expected_uid=av.expected_uid; def.expected_mode=av.expected_mode;
            def.expected_size=av.expected_size;
            def.expected_sha256=av.expected_sha256;
            def.source_component=av.source_component;
            def.target_component=av.target_component;
            if(copy_action_def(op,j,&def)!=0) goto invalid;
            wa=&op->actions[j];
            wa->state=av.state;
            wa->sequence=av.sequence;
            op->action_count++;
        }
        s->operation_count++;
    }
    s->revision=(uint64_t)replay->record_count;
    s->next_token=1U;
    return 0;
invalid:
    memset(s->operations,0,FCJ4_MAX_OPERATIONS*sizeof(*s->operations));
    s->operation_count=0U;
    s->revision=0U;
    s->next_token=1U;
    return fail(EBADMSG);
}

int fcj4_writer_prepare(Fcj4WriterState *s,const Fcj4Record *r,
                        char *output,size_t capacity,size_t *written,
                        Fcj4PreparedRecord *prepared)
{
    Fcj4PreparedRecord p;
    Fcj4MoveState move;
    Fcj4ActionState action;
    size_t n=0U;
    if(written) *written=0U;
    if(!s || !r || !output || !written || !prepared) return fail(EINVAL);
    if(s->pending) return fail(EBUSY);
    /* revision counts committed records; a prepared record is not counted
       until commit, so abort leaves the limit accounting unchanged. */
    if(s->revision>=FCJ4_MAX_RECORDS) return fail(E2BIG);
    memset(&p,0,sizeof(p));
    if(validate_next(s,r,&p,&move,&action)!=0) return -1;
    if(fcj4_serialize(r,output,capacity,&n)!=0) return -1;
    if(s->next_token==UINT64_MAX || s->revision==UINT64_MAX) return fail(EOVERFLOW);
    p.revision=s->revision;
    p.token=s->next_token++;
    s->prepared=p;
    s->pending_kind=r->kind;
    s->pending_move=move;
    s->pending_action=action;
    memset(&s->pending_evidence,0,sizeof(s->pending_evidence));
    if(r->kind==FCJ4_RECORD_BEGIN &&
       copy_begin(&s->pending_evidence,&r->value.begin)!=0) {
        s->pending=0; return fail(EINVAL);
    }
    if(r->kind==FCJ4_RECORD_SEALED &&
       copy_sealed(&s->pending_evidence,&r->value.sealed)!=0) {
        s->pending=0; return fail(EINVAL);
    }
    if(r->kind==FCJ4_RECORD_ACTION_DEF &&
       copy_action_def(&s->pending_evidence,0U,&r->value.action_def)!=0) {
        s->pending=0; return fail(EINVAL);
    }
    memcpy(s->pending_bytes,output,n);
    s->pending_length=n;
    s->pending=1;
    *prepared=p;
    *written=n;
    return 0;
}

int fcj4_writer_commit(Fcj4WriterState *s,const Fcj4PreparedRecord *p,
                        const char *committed_bytes,size_t committed_length)
{
    WriterOperation *op;
    if(!s || !p || !committed_bytes || !s->pending ||
       committed_length!=s->pending_length ||
       memcmp(committed_bytes,s->pending_bytes,s->pending_length)!=0 ||
       p->token!=s->prepared.token ||
       p->revision!=s->prepared.revision || p->kind!=s->prepared.kind ||
       memcmp(p->operation_id,s->prepared.operation_id,
              sizeof(s->prepared.operation_id))!=0 ||
       memcmp(p->action_id,s->prepared.action_id,
              sizeof(s->prepared.action_id))!=0 ||
       p->sequence!=s->prepared.sequence ||
       s->revision!=s->prepared.revision) return fail(EINVAL);
    op=find_op(s,p->operation_id);
    if(s->pending_kind==FCJ4_RECORD_BEGIN) {
        if(op || s->operation_count>=FCJ4_MAX_OPERATIONS) return fail(EINVAL);
        op=&s->operations[s->operation_count++];
        *op=s->pending_evidence;
        op->begin.operation_id=op->id;
        op->begin.source_path=op->source_path;
        op->begin.destination_path=op->destination_path;
        op->begin.destination_basename=op->destination_basename;
    } else {
        if(!op) return fail(EINVAL);
        if(s->pending_kind==FCJ4_RECORD_SEALED) {
            op->sealed=s->pending_evidence.sealed;
            memcpy(op->stage_component,s->pending_evidence.stage_component,
                   sizeof(op->stage_component));
            memcpy(op->stage_digest,s->pending_evidence.stage_digest,
                   sizeof(op->stage_digest));
            op->sealed.operation_id=op->id;
            op->sealed.stage_component=op->stage_component;
            op->sealed.sha256=op->stage_digest;
            op->has_sealed=1;
        }
        else if(s->pending_kind==FCJ4_RECORD_MOVE) {
            op->move_state=s->pending_move;
            op->move_sequence=p->sequence;
        } else if(s->pending_kind==FCJ4_RECORD_ACTION_DEF) {
            size_t i=op->action_count++;
            op->actions[i]=s->pending_evidence.actions[0];
            op->actions[i].record.operation_id=op->id;
            op->actions[i].record.action_id=op->actions[i].id;
            op->actions[i].record.source_component=op->actions[i].source_component;
            op->actions[i].record.target_component=
                op->actions[i].record.target_role==FCJ4_TARGET_QUARANTINE
                    ? op->actions[i].target_component : NULL;
            op->actions[i].record.expected_sha256=op->actions[i].digest;
        } else if(s->pending_kind==FCJ4_RECORD_ACTION) {
            size_t i=find_action(op,p->action_id);
            if(i==SIZE_MAX) return fail(EINVAL);
            op->actions[i].state=s->pending_action;
            op->actions[i].sequence=p->sequence;
        }
    }
    s->revision++;
    s->pending=0;
    memset(&s->prepared,0,sizeof(s->prepared));
    memset(s->pending_bytes,0,sizeof(s->pending_bytes));
    s->pending_length=0U;
    return 0;
}

int fcj4_writer_abort_prepare(Fcj4WriterState *s,const Fcj4PreparedRecord *p)
{
    if(!s || !p || !s->pending || p->token!=s->prepared.token ||
       p->revision!=s->prepared.revision ||
       memcmp(p->operation_id,s->prepared.operation_id,
              sizeof(s->prepared.operation_id))!=0 ||
       memcmp(p->action_id,s->prepared.action_id,
              sizeof(s->prepared.action_id))!=0 ||
       p->kind!=s->prepared.kind ||
       p->sequence!=s->prepared.sequence ||
       s->revision!=s->prepared.revision)
        return fail(EINVAL);
    s->pending=0;
    memset(&s->prepared,0,sizeof(s->prepared));
    memset(s->pending_bytes,0,sizeof(s->pending_bytes));
    memset(&s->pending_evidence,0,sizeof(s->pending_evidence));
    s->pending_length=0U;
    return 0;
}

uint64_t fcj4_writer_revision(const Fcj4WriterState *s)
{
    return s ? s->revision : 0U;
}
