#define _POSIX_C_SOURCE 200809L

#include "journal/fcj4_parser.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char id[33];
    unsigned int kind;
    unsigned int role;
    uint64_t parent_dev, parent_ino;
    uint64_t entry_dev, entry_ino, uid, mode, size;
    char digest[65];
    char source_component[NAME_MAX];
    char target_component[NAME_MAX];
    Fcj4ActionState state;
    uint64_t sequence;
} Fcj4Action;

typedef struct {
    char id[33];
    char source_path[PATH_MAX];
    char destination_path[PATH_MAX];
    char source_basename[NAME_MAX];
    char destination_basename[NAME_MAX];
    char stage_component[NAME_MAX];
    uint64_t source_parent_dev, source_parent_ino;
    uint64_t source_dev, source_ino, source_uid, source_gid, source_mode, source_size;
    int64_t source_mtime_sec, source_ctime_sec;
    uint64_t source_mtime_nsec, source_ctime_nsec;
    uint64_t destination_parent_dev, destination_parent_ino;
    uint64_t quarantine_parent_dev, quarantine_parent_ino;
    uint64_t stage_dev, stage_ino, stage_uid, stage_mode, stage_size;
    char stage_digest[65];
    int has_begin, has_sealed;
    Fcj4MoveState move_state;
    uint64_t move_sequence;
    Fcj4Action actions[FCJ4_MAX_ACTIONS_PER_OPERATION];
    size_t action_count;
} Fcj4Operation;

static int fail(int error)
{
    errno = error;
    return -1;
}

static int parse_u64(const char *s, uint64_t *out)
{
    uint64_t n = 0U;
    if (!s || !s[0] || !out || (s[0] == '0' && s[1] != '\0'))
        return -1;
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
        unsigned int d;
        if (*p < '0' || *p > '9') return -1;
        d = (unsigned int)(*p - '0');
        if (n > (UINT64_MAX - d) / 10U) return -1;
        n = n * 10U + d;
    }
    *out = n;
    return 0;
}

static int parse_i64(const char *s, int64_t *out)
{
    uint64_t magnitude = 0U;
    int negative = 0;
    if (!s || !s[0] || !out) return -1;
    if (s[0] == '-') {
        negative = 1;
        ++s;
        if (!s[0] || strcmp(s, "0") == 0) return -1;
    }
    if (parse_u64(s, &magnitude) != 0) return -1;
    if (negative) {
        if (magnitude > (uint64_t)INT64_MAX + 1U) return -1;
        *out = magnitude == (uint64_t)INT64_MAX + 1U
            ? INT64_MIN : -(int64_t)magnitude;
    } else {
        if (magnitude > (uint64_t)INT64_MAX) return -1;
        *out = (int64_t)magnitude;
    }
    return 0;
}

static int is_hex(const char *s, size_t n)
{
    if (!s || strlen(s) != n) return 0;
    for (size_t i = 0; i < n; ++i)
        if (!((s[i] >= '0' && s[i] <= '9') ||
              (s[i] >= 'a' && s[i] <= 'f'))) return 0;
    return 1;
}

static int hex_value(unsigned char c)
{
    if (c >= '0' && c <= '9') return (int)(c - '0');
    if (c >= 'a' && c <= 'f') return (int)(c - 'a') + 10;
    return -1;
}

static int decode_hex(const char *hex, uint64_t length, char *out, size_t cap)
{
    size_t n;
    if (!hex || !out || length == 0U || length >= cap ||
        length > (uint64_t)(SIZE_MAX / 2U)) return -1;
    n = (size_t)length;
    if (strlen(hex) != n * 2U) return -1;
    for (size_t i = 0; i < n; ++i) {
        int hi = hex_value((unsigned char)hex[i * 2U]);
        int lo = hex_value((unsigned char)hex[i * 2U + 1U]);
        unsigned char c;
        if (hi < 0 || lo < 0) return -1;
        c = (unsigned char)((hi << 4) | lo);
        if (c == 0U || c < 0x20U || c == 0x7fU) return -1;
        out[i] = (char)c;
    }
    out[n] = '\0';
    return 0;
}

static int valid_component(const char *s)
{
    if (!s || !s[0] || strcmp(s, ".") == 0 || strcmp(s, "..") == 0 ||
        strlen(s) >= NAME_MAX) return 0;
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p)
        if (*p == '/' || *p < 0x20U || *p == 0x7fU) return 0;
    return 1;
}

static int valid_absolute_path(const char *s)
{
    const char *p;
    if (!s || s[0] != '/' || !s[1] || strlen(s) >= PATH_MAX) return 0;
    p = s + 1;
    while (*p) {
        const char *start = p;
        while (*p && *p != '/') {
            unsigned char c = (unsigned char)*p;
            if (c < 0x20U || c == 0x7fU) return 0;
            ++p;
        }
        if (p == start || (size_t)(p - start) == 1U && start[0] == '.' ||
            (size_t)(p - start) == 2U && start[0] == '.' && start[1] == '.')
            return 0;
        if (*p == '/') {
            ++p;
            if (!*p) return 0;
        }
    }
    return 1;
}

static const char *basename_of(const char *path)
{
    const char *p = strrchr(path, '/');
    return p ? p + 1 : path;
}

static int split_fields(char *line, char **fields, size_t capacity,
                        size_t *count)
{
    size_t n = 1U;
    fields[0] = line;
    for (char *p = line; *p; ++p) {
        if (*p == '|') {
            if (n >= capacity) return -1;
            *p = '\0';
            fields[n++] = p + 1;
        }
    }
    *count = n;
    return 0;
}

static int get_u64(char **f, size_t i, uint64_t *v)
{
    return parse_u64(f[i], v);
}

static int get_i64(char **f, size_t i, int64_t *v)
{
    return parse_i64(f[i], v);
}

static Fcj4Operation *find_operation(Fcj4Replay *r, const char *id)
{
    Fcj4Operation *ops = (Fcj4Operation *)r->operations;
    for (size_t i = 0; i < r->count; ++i)
        if (strcmp(ops[i].id, id) == 0) return &ops[i];
    return NULL;
}

static int parse_begin(char **f, size_t n, Fcj4Replay *r)
{
    uint64_t v[18];
    int64_t t[2];
    Fcj4Operation *op;
    static const size_t uidx[] = {
        4,6,7,8,9,11,12,13,14,16,17,19,21,22,23,25,26
    };
    if (n != 27U || strcmp(f[0],"FCJ4") || strcmp(f[1],"BEGIN") ||
        strcmp(f[2],"1") || !is_hex(f[3],32U)) return -1;
    if (r->count >= FCJ4_MAX_OPERATIONS) return -2;
    if (find_operation(r, f[3])) return -1;
    for (size_t i=0;i<sizeof(uidx)/sizeof(uidx[0]);++i)
        if (parse_u64(f[uidx[i]], &v[i]) != 0) return -1;
    if (v[0] == 0U || v[0] >= PATH_MAX || v[1] == 0U ||
        v[1] >= PATH_MAX || v[9] == 0U || v[9] >= NAME_MAX ||
        v[14] == 0U || v[14] >= NAME_MAX) return -1;
    if (get_i64(f,15,&t[0]) || get_u64(f,16,&v[9]) ||
        get_i64(f,17,&t[1]) || get_u64(f,18,&v[10])) return -1;
    if (v[9] > 999999999U || v[10] > 999999999U) return -1;

    if (r->count == 0U) {
        op = (Fcj4Operation *)calloc(FCJ4_MAX_OPERATIONS, sizeof(Fcj4Operation));
        if (!op) return -3;
        r->operations = op;
    }
    op = &((Fcj4Operation *)r->operations)[r->count];
    memset(op, 0, sizeof(*op));
    if (decode_hex(f[5], v[0], op->source_path, sizeof(op->source_path)) ||
        decode_hex(f[20], v[11], op->destination_path, sizeof(op->destination_path)) ||
        decode_hex(f[24], v[14], op->destination_basename, sizeof(op->destination_basename)) ||
        !valid_absolute_path(op->source_path) ||
        !valid_absolute_path(op->destination_path) ||
        !valid_component(op->destination_basename) ||
        strcmp(op->destination_basename, basename_of(op->destination_path)) != 0 ||
        strcmp(f[10],"REGULAR") != 0) return -1;

    memcpy(op->id,f[3],33U);
    op->source_parent_dev=v[2]; op->source_parent_ino=v[3];
    op->source_dev=v[4]; op->source_ino=v[5];
    op->source_uid=v[6]; op->source_gid=v[7]; op->source_mode=v[8];
    op->source_size=v[9]; op->source_mtime_sec=t[0]; op->source_mtime_nsec=v[9+1U];
    op->source_ctime_sec=t[1]; op->source_ctime_nsec=v[10];
    op->destination_parent_dev=v[12]; op->destination_parent_ino=v[13];
    op->quarantine_parent_dev=v[15]; op->quarantine_parent_ino=v[16];
    op->has_begin=1;
    r->count++;
    return 0;
}

static int parse_sealed(char **f, size_t n, Fcj4Replay *r)
{
    uint64_t v[6];
    Fcj4Operation *op;
    if (n != 12U || strcmp(f[0],"FCJ4") || strcmp(f[1],"SEALED") ||
        strcmp(f[2],"1") || !is_hex(f[3],32U) || !is_hex(f[11],64U))
        return -1;
    op=find_operation(r,f[3]);
    if (!op || op->has_sealed) return -1;
    if (get_u64(f,4,&v[0]) || get_u64(f,6,&v[1]) ||
        get_u64(f,7,&v[2]) || get_u64(f,8,&v[3]) ||
        get_u64(f,9,&v[4]) || get_u64(f,10,&v[5])) return -1;
    if (decode_hex(f[5],v[0],op->stage_component,sizeof(op->stage_component)) ||
        !valid_component(op->stage_component) ||
        strncmp(op->stage_component,".fc-tmp-",8U)!=0 ||
        strlen(op->stage_component)<=8U || v[1]==0U || v[2]==0U ||
        v[5]!=op->source_size) return -1;
    op->stage_dev=v[1]; op->stage_ino=v[2]; op->stage_uid=v[3];
    op->stage_mode=v[4]; op->stage_size=v[5];
    memcpy(op->stage_digest,f[11],65U);
    op->has_sealed=1;
    return 0;
}

static int parse_move(char **f, size_t n, Fcj4Replay *r)
{
    uint64_t seq, err;
    Fcj4Operation *op;
    Fcj4MoveState next;
    if (n!=7U || strcmp(f[0],"FCJ4") || strcmp(f[1],"MOVE") ||
        strcmp(f[2],"1") || !is_hex(f[3],32U) ||
        get_u64(f,4,&seq) || get_u64(f,6,&err)) return -1;
    op=find_operation(r,f[3]);
    if (!op || !op->has_sealed || seq==0U || seq!=op->move_sequence+1U ||
        err>(uint64_t)INT_MAX) return -1;
    if (!strcmp(f[5],"STAGED")) next=FCJ4_MOVE_STAGED;
    else if (!strcmp(f[5],"STARTED")) next=FCJ4_MOVE_STARTED;
    else if (!strcmp(f[5],"PUBLISHED")) next=FCJ4_MOVE_PUBLISHED;
    else if (!strcmp(f[5],"COMPLETED")) next=FCJ4_MOVE_COMPLETED;
    else if (!strcmp(f[5],"FAILED")) next=FCJ4_MOVE_FAILED;
    else return -1;
    if ((next==FCJ4_MOVE_FAILED) ? err==0U : err!=0U) return -1;
    if (!((op->move_state==FCJ4_MOVE_NONE && next==FCJ4_MOVE_STAGED) ||
          (op->move_state==FCJ4_MOVE_STAGED &&
             (next==FCJ4_MOVE_STARTED || next==FCJ4_MOVE_FAILED)) ||
          (op->move_state==FCJ4_MOVE_STARTED &&
             (next==FCJ4_MOVE_PUBLISHED || next==FCJ4_MOVE_FAILED)) ||
          (op->move_state==FCJ4_MOVE_PUBLISHED && next==FCJ4_MOVE_COMPLETED)))
        return -1;
    op->move_state=next; op->move_sequence=seq;
    return 0;
}

static int parse_action_def(char **f, size_t n, Fcj4Replay *r)
{
    uint64_t v[10];
    Fcj4Operation *op;
    Fcj4Action *a;
    unsigned int kind, role;
    if (n!=20U || strcmp(f[0],"FCJ4") || strcmp(f[1],"ACTION_DEF") ||
        strcmp(f[2],"1") || !is_hex(f[3],32U) || !is_hex(f[4],32U) ||
        !is_hex(f[15],64U)) return -1;
    op=find_operation(r,f[3]);
    if (!op || !op->has_sealed || op->action_count>=FCJ4_MAX_ACTIONS_PER_OPERATION)
        return -1;
    for (size_t i=0;i<op->action_count;i++)
        if (!strcmp(op->actions[i].id,f[4])) return -1;
    if (!strcmp(f[5],"QUARANTINE_STAGE")) kind=1U;
    else if (!strcmp(f[5],"QUARANTINE_SOURCE")) kind=2U;
    else if (!strcmp(f[5],"RETIRE_QUARANTINED_SOURCE")) kind=3U;
    else if (!strcmp(f[5],"REMOVE_QUARANTINED_STAGE")) kind=4U;
    else return -1;
    if (!strcmp(f[6],"QUARANTINE")) role=1U;
    else if (!strcmp(f[6],"NONE")) role=2U;
    else return -1;
    if (get_u64(f,7,&v[0]) || get_u64(f,8,&v[1]) ||
        get_u64(f,9,&v[2]) || get_u64(f,10,&v[3]) ||
        get_u64(f,12,&v[4]) || get_u64(f,13,&v[5]) ||
        get_u64(f,14,&v[6]) || get_u64(f,16,&v[7]) ||
        get_u64(f,18,&v[8])) return -1;
    if (strcmp(f[11],"REGULAR")!=0 || v[3]==0U) return -1;
    if (decode_hex(f[17],v[7],(char[NAME_MAX]){0},NAME_MAX)!=0) return -1;
    if (v[7]==0U || v[7]>=NAME_MAX) return -1;
    a=&op->actions[op->action_count];
    memset(a,0,sizeof(*a));
    if (decode_hex(f[17],v[7],a->source_component,sizeof(a->source_component)) ||
        !valid_component(a->source_component)) return -1;
    if (role==1U) {
        if (v[8]==0U || v[8]>=NAME_MAX ||
            decode_hex(f[19],v[8],a->target_component,sizeof(a->target_component)) ||
            !valid_component(a->target_component) ||
            (kind!=1U && kind!=2U)) return -1;
    } else {
        if (kind!=3U && kind!=4U) return -1;
        if (strcmp(f[18],"0")!=0 || strcmp(f[19],"-")!=0 || v[8]!=0U)
            return -1;
    }
    if (role==1U && (kind==1U || kind==2U)) {
        if (v[0]!=op->quarantine_parent_dev || v[1]!=op->quarantine_parent_ino)
            return -1;
    } else if (kind==3U || kind==4U) {
        if (v[0]!=op->quarantine_parent_dev || v[1]!=op->quarantine_parent_ino)
            return -1;
    }
    if (kind==1U && (v[0]!=op->destination_parent_dev ||
        v[1]!=op->destination_parent_ino ||
        strcmp(a->source_component,op->stage_component)!=0)) return -1;
    if (kind==2U && (v[0]!=op->source_parent_dev ||
        v[1]!=op->source_parent_ino ||
        strcmp(a->source_component,basename_of(op->source_path))!=0)) return -1;
    if (kind==3U && strcmp(a->source_component,basename_of(op->source_path))!=0)
        return -1;
    if (kind==4U && strcmp(a->source_component,op->stage_component)!=0)
        return -1;
    memcpy(a->id,f[4],33U);
    a->kind=kind; a->role=role;
    a->parent_dev=v[0]; a->parent_ino=v[1];
    a->entry_dev=v[2]; a->entry_ino=v[3];
    a->uid=v[4]; a->mode=v[5]; a->size=v[6];
    memcpy(a->digest,f[15],65U);
    op->action_count++;
    return 0;
}

static int parse_action(char **f, size_t n, Fcj4Replay *r)
{
    uint64_t seq, err;
    Fcj4Operation *op;
    Fcj4Action *a=NULL;
    Fcj4ActionState next;
    if (n!=8U || strcmp(f[0],"FCJ4") || strcmp(f[1],"ACTION") ||
        strcmp(f[2],"1") || !is_hex(f[3],32U) || !is_hex(f[4],32U) ||
        get_u64(f,5,&seq) || get_u64(f,7,&err) || seq==0U ||
        err>(uint64_t)INT_MAX) return -1;
    op=find_operation(r,f[3]);
    if (!op || !op->has_sealed) return -1;
    for (size_t i=0;i<op->action_count;i++)
        if (!strcmp(op->actions[i].id,f[4])) { a=&op->actions[i]; break; }
    if (!a || seq!=a->sequence+1U) return -1;
    if (!strcmp(f[6],"ACTION_PREPARED")) next=FCJ4_ACTION_PREPARED;
    else if (!strcmp(f[6],"ACTION_STARTED")) next=FCJ4_ACTION_STARTED;
    else if (!strcmp(f[6],"ACTION_APPLIED")) next=FCJ4_ACTION_APPLIED;
    else if (!strcmp(f[6],"ACTION_SYNCED")) next=FCJ4_ACTION_SYNCED;
    else if (!strcmp(f[6],"ACTION_COMPLETED")) next=FCJ4_ACTION_COMPLETED;
    else if (!strcmp(f[6],"ACTION_MANUAL_REVIEW")) next=FCJ4_ACTION_MANUAL_REVIEW;
    else return -1;
    if ((next==FCJ4_ACTION_MANUAL_REVIEW) ? err==0U : err!=0U) return -1;
    if (!((a->state==FCJ4_ACTION_NONE && next==FCJ4_ACTION_PREPARED) ||
          (a->state==FCJ4_ACTION_PREPARED &&
             (next==FCJ4_ACTION_STARTED || next==FCJ4_ACTION_MANUAL_REVIEW)) ||
          (a->state==FCJ4_ACTION_STARTED &&
             (next==FCJ4_ACTION_APPLIED || next==FCJ4_ACTION_MANUAL_REVIEW)) ||
          (a->state==FCJ4_ACTION_APPLIED &&
             (next==FCJ4_ACTION_SYNCED || next==FCJ4_ACTION_MANUAL_REVIEW)) ||
          (a->state==FCJ4_ACTION_SYNCED &&
             (next==FCJ4_ACTION_COMPLETED || next==FCJ4_ACTION_MANUAL_REVIEW))))
        return -1;
    a->state=next; a->sequence=seq;
    return 0;
}

void fcj4_replay_init(Fcj4Replay *replay)
{
    if (replay) memset(replay,0,sizeof(*replay));
}

void fcj4_replay_destroy(Fcj4Replay *replay)
{
    if (!replay) return;
    free(replay->operations);
    memset(replay,0,sizeof(*replay));
}

int fcj4_replay_parse(const void *data, size_t length, Fcj4Replay *out)
{
    const unsigned char *bytes=(const unsigned char *)data;
    Fcj4Replay temporary;
    size_t offset=0U;
    int status=0;
    if (!out || (length && !data)) return fail(EINVAL);
    if (out->operations || out->count || out->record_count) return fail(EINVAL);
    if (length>FCJ4_MAX_JOURNAL_BYTES) return fail(E2BIG);
    fcj4_replay_init(&temporary);
    while (offset<length) {
        const unsigned char *newline;
        size_t line_length, content_length, field_count=0U;
        char line[FCJ4_MAX_LINE_BYTES+1U];
        char *fields[24];
        Fcj4Operation *before;
        if (temporary.record_count>=FCJ4_MAX_RECORDS) { status=E2BIG; break; }
        newline=(const unsigned char *)memchr(bytes+offset,'\n',length-offset);
        if (!newline) { status=EBADMSG; break; }
        line_length=(size_t)(newline-(bytes+offset))+1U;
        if (line_length>FCJ4_MAX_LINE_BYTES) { status=E2BIG; break; }
        content_length=line_length-1U;
        if (content_length==0U || memchr(bytes+offset,'\0',content_length) ||
            memchr(bytes+offset,'\r',content_length)) { status=EBADMSG; break; }
        memcpy(line,bytes+offset,content_length);
        line[content_length]='\0';
        if (split_fields(line,fields,24U,&field_count)!=0 ||
            field_count<3U || strcmp(fields[0],"FCJ4")!=0 ||
            strcmp(fields[2],"1")!=0) { status=EBADMSG; break; }
        before=(Fcj4Operation *)temporary.operations;
        if (!strcmp(fields[1],"BEGIN"))
            status=parse_begin(fields,field_count,&temporary);
        else if (!strcmp(fields[1],"SEALED"))
            status=parse_sealed(fields,field_count,&temporary);
        else if (!strcmp(fields[1],"MOVE"))
            status=parse_move(fields,field_count,&temporary);
        else if (!strcmp(fields[1],"ACTION_DEF"))
            status=parse_action_def(fields,field_count,&temporary);
        else if (!strcmp(fields[1],"ACTION"))
            status=parse_action(fields,field_count,&temporary);
        else status=-1;
        (void)before;
        if (status!=0) { status=status==-2 ? E2BIG : status==-3 ? ENOMEM : EBADMSG; break; }
        temporary.record_count++;
        offset+=line_length;
    }
    if (status==0) {
        *out=temporary;
        return 0;
    }
    fcj4_replay_destroy(&temporary);
    return fail(status);
}

int fcj4_replay_operation(const Fcj4Replay *replay, size_t index,
                          Fcj4OperationView *out)
{
    const Fcj4Operation *ops;
    if (!replay || !out || index>=replay->count || !replay->operations)
        return -1;
    ops=(const Fcj4Operation *)replay->operations;
    memcpy(out->operation_id,ops[index].id,sizeof(out->operation_id));
    out->action_count=ops[index].action_count;
    out->move_state=ops[index].move_state;
    return 0;
}
