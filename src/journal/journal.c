#define _POSIX_C_SOURCE 200809L

#include "journal/journal.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define JOURNAL_MAX_RECOVERY_RECORDS 1024
#define JOURNAL_LINE_MAX 12288

typedef struct
{
    char src[PATH_MAX];
    char dest[PATH_MAX];
    int final;
} RecoveryRecord;

static long long now_ns(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
        return 0;
    return ((long long)ts.tv_sec * 1000000000LL) + ts.tv_nsec;
}

static int ensure_parent_dir(const char *path)
{
    char dir[PATH_MAX];
    const char *slash = strrchr(path, '/');

    if (!slash)
        return 0;

    size_t len = (size_t)(slash - path);
    if (len == 0 || len >= sizeof(dir))
        return -1;

    memcpy(dir, path, len);
    dir[len] = '\0';

    if (mkdir(dir, 0755) == 0 || errno == EEXIST)
        return 0;

    return -1;
}

static int append_record(FILE *fp,
                         const char *state,
                         const char *src,
                         const char *dest,
                         int error_code)
{
    if (fprintf(fp, "%lld|%s|%d|%s|%s\n",
                now_ns(), state, error_code, src, dest) < 0)
        return -1;

    if (fflush(fp) != 0)
        return -1;

    return fsync(fileno(fp));
}

static int parse_record(char *line,
                        char **state,
                        char **src,
                        char **dest)
{
    char *save = NULL;
    char *timestamp = strtok_r(line, "|", &save);
    char *parsed_state = strtok_r(NULL, "|", &save);
    char *error_code = strtok_r(NULL, "|", &save);
    char *parsed_src = strtok_r(NULL, "|", &save);
    char *parsed_dest = strtok_r(NULL, "\n", &save);

    (void)timestamp;
    (void)error_code;

    if (!parsed_state || !parsed_src || !parsed_dest)
        return -1;

    *state = parsed_state;
    *src = parsed_src;
    *dest = parsed_dest;
    return 0;
}

static RecoveryRecord *find_or_add_record(RecoveryRecord *records,
                                          int *count,
                                          const char *src,
                                          const char *dest)
{
    for (int i = 0; i < *count; i++)
    {
        if (strcmp(records[i].src, src) == 0 &&
            strcmp(records[i].dest, dest) == 0)
            return &records[i];
    }

    if (*count >= JOURNAL_MAX_RECOVERY_RECORDS)
        return NULL;

    RecoveryRecord *record = &records[*count];
    snprintf(record->src, sizeof(record->src), "%s", src);
    snprintf(record->dest, sizeof(record->dest), "%s", dest);
    record->final = 0;
    (*count)++;
    return record;
}

int journal_recover(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
    {
        if (errno == ENOENT)
            return 0;
        return -1;
    }

    RecoveryRecord *records = calloc(JOURNAL_MAX_RECOVERY_RECORDS, sizeof(*records));
    if (!records)
    {
        fclose(fp);
        return -1;
    }

    int count = 0;
    char line[JOURNAL_LINE_MAX];

    while (fgets(line, sizeof(line), fp))
    {
        char *state = NULL;
        char *src = NULL;
        char *dest = NULL;

        if (parse_record(line, &state, &src, &dest) != 0)
            continue;

        RecoveryRecord *record = find_or_add_record(records, &count, src, dest);
        if (!record)
            continue;

        if (strcmp(state, "COMPLETED") == 0 ||
            strcmp(state, "FAILED") == 0 ||
            strcmp(state, "RECOVERED") == 0)
            record->final = 1;
    }

    fclose(fp);

    MoveJournal journal;
    int journal_ready = journal_open(&journal, path) == 0;

    for (int i = 0; i < count; i++)
    {
        if (records[i].final)
            continue;

        char temp_path[PATH_MAX];
        int written = snprintf(temp_path, sizeof(temp_path), "%s.tmp", records[i].dest);
        if (written < 0 || (size_t)written >= sizeof(temp_path))
            continue;

        if (access(records[i].src, F_OK) == 0 && access(temp_path, F_OK) == 0)
            unlink(temp_path);

        if (journal_ready)
            append_record(journal.fp, "RECOVERED", records[i].src, records[i].dest, 0);
    }

    if (journal_ready)
        journal_close(&journal);

    free(records);
    return 0;
}

int journal_open(MoveJournal *journal, const char *path)
{
    memset(journal, 0, sizeof(*journal));

    if (ensure_parent_dir(path) != 0)
        return -1;

    journal->fp = fopen(path, "a");
    if (!journal->fp)
        return -1;

    if (pthread_mutex_init(&journal->lock, NULL) != 0)
    {
        fclose(journal->fp);
        journal->fp = NULL;
        return -1;
    }

    snprintf(journal->path, sizeof(journal->path), "%s", path);
    journal->enabled = 1;
    return 0;
}

void journal_close(MoveJournal *journal)
{
    if (!journal || !journal->enabled)
        return;

    pthread_mutex_lock(&journal->lock);
    fflush(journal->fp);
    fsync(fileno(journal->fp));
    fclose(journal->fp);
    journal->fp = NULL;
    journal->enabled = 0;
    pthread_mutex_unlock(&journal->lock);
    pthread_mutex_destroy(&journal->lock);
}

static int journal_record(MoveJournal *journal,
                          const char *state,
                          const char *src,
                          const char *dest,
                          int error_code)
{
    if (!journal || !journal->enabled)
        return 0;

    pthread_mutex_lock(&journal->lock);
    int rc = append_record(journal->fp, state, src, dest, error_code);
    pthread_mutex_unlock(&journal->lock);
    return rc;
}

int journal_record_started(MoveJournal *journal, const char *src, const char *dest)
{
    return journal_record(journal, "STARTED", src, dest, 0);
}

int journal_record_completed(MoveJournal *journal, const char *src, const char *dest)
{
    return journal_record(journal, "COMPLETED", src, dest, 0);
}

int journal_record_failed(MoveJournal *journal, const char *src, const char *dest, int error_code)
{
    return journal_record(journal, "FAILED", src, dest, error_code);
}
