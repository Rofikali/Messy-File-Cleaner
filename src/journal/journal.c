#define _POSIX_C_SOURCE 200809L

#include "journal/journal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <time.h>
#include <unistd.h>

#define JOURNAL_LINE_MAX (4U * PATH_MAX + 256U)
#define JOURNAL_MAX_RECOVERY_RECORDS 4096

typedef struct
{
    char src[PATH_MAX];
    char dest[PATH_MAX];
    char state[16];
} RecoveryRecord;

static int valid_state(const char *state)
{
    return strcmp(state, "STARTED") == 0 ||
           strcmp(state, "PUBLISHED") == 0 ||
           strcmp(state, "COMPLETED") == 0 ||
           strcmp(state, "FAILED") == 0 ||
           strcmp(state, "RECOVERED") == 0;
}

static long long now_ns(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
        return -1;
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

/* Open the journal's parent by walking each component from a trusted root
 * descriptor. Reject dot components and symlinks rather than resolving them. */
static int open_parent_dir(const char *path, char leaf[PATH_MAX])
{
    if (!path || !path[0] || strlen(path) >= PATH_MAX) {
        errno = EINVAL;
        return -1;
    }

    char copy[PATH_MAX];
    memcpy(copy, path, strlen(path) + 1);
    char *slash = strrchr(copy, '/');
    char *base = copy;
    const char *parent = ".";
    if (slash) {
        *slash = '\0';
        base = slash + 1;
        parent = copy[0] ? copy : "/";
    }
    if (!base[0] || strcmp(base, ".") == 0 || strcmp(base, "..") == 0 ||
        strlen(base) >= PATH_MAX) {
        errno = EINVAL;
        return -1;
    }
    memcpy(leaf, base, strlen(base) + 1);

    int fd = open(parent[0] == '/' ? "/" : ".",
                  O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0)
        return -1;

    const char *walk = parent;
    while (*walk == '/')
        walk++;
    if (strcmp(parent, ".") == 0 || strcmp(parent, "/") == 0)
        return fd;

    char *save = NULL;
    for (char *component = strtok_r((char *)walk, "/", &save);
         component; component = strtok_r(NULL, "/", &save)) {
        if (!component[0] || strcmp(component, ".") == 0 ||
            strcmp(component, "..") == 0) {
            close(fd);
            errno = EINVAL;
            return -1;
        }
        int next = openat(fd, component,
                          O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        if (next < 0 && errno == ENOENT) {
            if (mkdirat(fd, component, 0700) != 0 && errno != EEXIST) {
                int saved_errno = errno;
                close(fd);
                errno = saved_errno;
                return -1;
            }
            next = openat(fd, component,
                          O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        }
        if (next < 0) {
            int saved_errno = errno;
            close(fd);
            errno = saved_errno;
            return -1;
        }
        close(fd);
        fd = next;
    }
    return fd;
}

static int hex_encode(const char *input, char *output, size_t output_size)
{
    static const char digits[] = "0123456789abcdef";
    size_t length = strlen(input);
    if (length >= PATH_MAX || length > (output_size - 1) / 2)
    {
        errno = ENAMETOOLONG;
        return -1;
    }

    for (size_t i = 0; i < length; i++)
    {
        unsigned char byte = (unsigned char)input[i];
        output[i * 2] = digits[byte >> 4];
        output[i * 2 + 1] = digits[byte & 0x0f];
    }
    output[length * 2] = '\0';
    return (int)length;
}

static int hex_value(char value)
{
    if (value >= '0' && value <= '9')
        return value - '0';
    if (value >= 'a' && value <= 'f')
        return value - 'a' + 10;
    if (value >= 'A' && value <= 'F')
        return value - 'A' + 10;
    return -1;
}

static int hex_decode(const char *input, size_t input_len,
                      size_t expected_len, char output[PATH_MAX])
{
    if (expected_len >= PATH_MAX || input_len != expected_len * 2)
        return -1;

    for (size_t i = 0; i < expected_len; i++)
    {
        int high = hex_value(input[i * 2]);
        int low = hex_value(input[i * 2 + 1]);
        if (high < 0 || low < 0)
            return -1;
        char decoded = (char)((high << 4) | low);
        if (decoded == '\0')
            return -1;
        output[i] = decoded;
    }
    output[expected_len] = '\0';
    return expected_len > 0 ? 0 : -1;
}

static int parse_number(const char *value, unsigned long long *number)
{
    if (!value || !value[0])
        return -1;
    errno = 0;
    char *end = NULL;
    unsigned long long parsed = strtoull(value, &end, 10);
    if (errno != 0 || !end || *end != '\0')
        return -1;
    *number = parsed;
    return 0;
}

static int parse_record(char *line, RecoveryRecord *record, int *error_code)
{
    size_t line_len = strlen(line);
    if (line_len == 0 || line[line_len - 1] != '\n')
        return -1;
    line[--line_len] = '\0';

    char *fields[8] = {0};
    size_t field_count = 1;
    fields[0] = line;
    for (size_t i = 0; i < line_len; i++)
    {
        if (line[i] == '|')
        {
            if (field_count >= 8)
                return -1;
            line[i] = '\0';
            fields[field_count++] = &line[i + 1];
        }
    }

    unsigned long long ignored_timestamp = 0;
    unsigned long long parsed_error = 0;

    if (field_count == 8 && strcmp(fields[0], "FCJ2") == 0)
    {
        unsigned long long src_len = 0, dest_len = 0;
        if (parse_number(fields[1], &ignored_timestamp) != 0 ||
            !valid_state(fields[2]) ||
            parse_number(fields[3], &parsed_error) != 0 ||
            parsed_error > INT_MAX ||
            parse_number(fields[4], &src_len) != 0 ||
            parse_number(fields[5], &dest_len) != 0 ||
            src_len >= PATH_MAX || dest_len >= PATH_MAX)
            return -1;

        if (hex_decode(fields[6], strlen(fields[6]), (size_t)src_len, record->src) != 0 ||
            hex_decode(fields[7], strlen(fields[7]), (size_t)dest_len, record->dest) != 0)
            return -1;

        (void)snprintf(record->state, sizeof(record->state), "%s", fields[2]);
        *error_code = (int)parsed_error;
        return 0;
    }

    /* Strict legacy parser: old pipe-delimited paths containing '|' are
     * ambiguous and are rejected instead of being guessed. */
    if (field_count != 5 ||
        parse_number(fields[0], &ignored_timestamp) != 0 ||
        !valid_state(fields[1]) ||
        parse_number(fields[2], &parsed_error) != 0 ||
        parsed_error > INT_MAX ||
        !fields[3][0] || !fields[4][0] ||
        strlen(fields[3]) >= PATH_MAX || strlen(fields[4]) >= PATH_MAX)
        return -1;

    (void)snprintf(record->src, sizeof(record->src), "%s", fields[3]);
    (void)snprintf(record->dest, sizeof(record->dest), "%s", fields[4]);
    (void)snprintf(record->state, sizeof(record->state), "%s", fields[1]);
    *error_code = (int)parsed_error;
    return 0;
}

static int append_record(FILE *fp, const char *state,
                         const char *src, const char *dest, int error_code)
{
    if (!fp || !valid_state(state) || !src || !dest ||
        error_code < 0 || strlen(src) >= PATH_MAX || strlen(dest) >= PATH_MAX)
    {
        errno = EINVAL;
        return -1;
    }

    char *src_hex = malloc(PATH_MAX * 2U + 1U);
    char *dest_hex = malloc(PATH_MAX * 2U + 1U);
    if (!src_hex || !dest_hex)
    {
        free(src_hex);
        free(dest_hex);
        return -1;
    }

    int src_len = hex_encode(src, src_hex, PATH_MAX * 2U + 1U);
    int dest_len = hex_encode(dest, dest_hex, PATH_MAX * 2U + 1U);
    if (src_len < 0 || dest_len < 0)
    {
        free(src_hex);
        free(dest_hex);
        return -1;
    }

    long long timestamp = now_ns();
    if (timestamp < 0)
    {
        free(src_hex);
        free(dest_hex);
        return -1;
    }

    int rc = fprintf(fp, "FCJ2|%lld|%s|%d|%d|%d|%s|%s\n",
                     timestamp, state, error_code, src_len, dest_len,
                     src_hex, dest_hex);
    free(src_hex);
    free(dest_hex);
    if (rc < 0 || fflush(fp) != 0)
        return -1;

    int sync_rc;
    do
    {
        sync_rc = fsync(fileno(fp));
    } while (sync_rc != 0 && errno == EINTR);
    return sync_rc;
}

static RecoveryRecord *find_or_add(RecoveryRecord *records, size_t *count,
                                   const char *src, const char *dest)
{
    for (size_t i = 0; i < *count; i++)
    {
        if (strcmp(records[i].src, src) == 0 &&
            strcmp(records[i].dest, dest) == 0)
            return &records[i];
    }

    if (*count >= JOURNAL_MAX_RECOVERY_RECORDS)
    {
        errno = EOVERFLOW;
        return NULL;
    }

    RecoveryRecord *record = &records[*count];
    (void)snprintf(record->src, sizeof(record->src), "%s", src);
    (void)snprintf(record->dest, sizeof(record->dest), "%s", dest);
    record->state[0] = '\0';
    (*count)++;
    return record;
}

static int path_is_regular(const char *path, int *exists)
{
    struct stat st;
    if (lstat(path, &st) == 0)
    {
        if (!S_ISREG(st.st_mode))
        {
            errno = EINVAL;
            return -1;
        }
        *exists = 1;
        return 0;
    }

    if (errno == ENOENT)
    {
        *exists = 0;
        return 0;
    }
    return -1;
}

int journal_recover(const char *path)
{
    if (!path || !path[0])
    {
        errno = EINVAL;
        return -1;
    }

    char leaf[PATH_MAX];
    int parent_fd = open_parent_dir(path, leaf);
    if (parent_fd < 0)
        return -1;
    int fd = openat(parent_fd, leaf,
                    O_RDWR | O_APPEND | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    int open_errno = errno;
    close(parent_fd);
    if (fd < 0) {
        errno = open_errno;
        return errno == ENOENT ? 0 : -1;
    }

    int lock_rc;
    do
    {
        lock_rc = flock(fd, LOCK_EX | LOCK_NB);
    } while (lock_rc != 0 && errno == EINTR);
    if (lock_rc != 0)
    {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }

    struct stat journal_stat;
    if (fstat(fd, &journal_stat) != 0)
    {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }
    if (!S_ISREG(journal_stat.st_mode))
    {
        close(fd);
        errno = EINVAL;
        return -1;
    }

    FILE *input = fdopen(fd, "a+");
    if (!input)
    {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }

    if (fseek(input, 0, SEEK_SET) != 0)
    {
        int saved_errno = errno;
        fclose(input);
        errno = saved_errno;
        return -1;
    }

    RecoveryRecord *records = calloc(JOURNAL_MAX_RECOVERY_RECORDS, sizeof(*records));
    if (!records)
    {
        fclose(input);
        return -1;
    }

    size_t count = 0;
    char line[JOURNAL_LINE_MAX + 2U];
    int read_status = 0;

    while (fgets(line, sizeof(line), input))
    {
        size_t length = strlen(line);
        if (length == 0 || line[length - 1] != '\n')
        {
            errno = EOVERFLOW;
            read_status = -1;
            break;
        }

        RecoveryRecord parsed;
        int error_code = 0;
        if (parse_record(line, &parsed, &error_code) != 0)
        {
            errno = EBADMSG;
            read_status = -1;
            break;
        }

        RecoveryRecord *record = find_or_add(records, &count,
                                             parsed.src, parsed.dest);
        if (!record)
        {
            read_status = -1;
            break;
        }
        (void)snprintf(record->state, sizeof(record->state), "%s", parsed.state);
    }

    if (ferror(input))
        read_status = -1;

    int saved_errno = errno;
    if (read_status != 0)
    {
        fclose(input);
        free(records);
        errno = saved_errno ? saved_errno : EBADMSG;
        return -1;
    }

    if (fseek(input, 0, SEEK_END) != 0)
    {
        saved_errno = errno;
        fclose(input);
        free(records);
        errno = saved_errno;
        return -1;
    }

    MoveJournal journal;
    memset(&journal, 0, sizeof(journal));
    int mutex_error = pthread_mutex_init(&journal.lock, NULL);
    if (mutex_error != 0)
    {
        fclose(input);
        free(records);
        errno = mutex_error;
        return -1;
    }
    journal.fp = input;
    memcpy(journal.path, path, strlen(path) + 1);
    journal.enabled = 1;

    int recovery_status = 0;
    for (size_t i = 0; i < count; i++)
    {
        RecoveryRecord *record = &records[i];
        if (strcmp(record->state, "COMPLETED") == 0 ||
            strcmp(record->state, "FAILED") == 0 ||
            strcmp(record->state, "RECOVERED") == 0)
            continue;

        int source_exists = 0;
        int destination_exists = 0;
        if (path_is_regular(record->src, &source_exists) != 0 ||
            path_is_regular(record->dest, &destination_exists) != 0)
        {
            recovery_status = -1;
            break;
        }

        int recovery_error = 0;
        if (strcmp(record->state, "PUBLISHED") == 0)
        {
            if (!source_exists && !destination_exists)
            {
                errno = EUCLEAN;
                recovery_status = -1;
                break;
            }
            if (source_exists && destination_exists)
                recovery_error = EALREADY; /* Preserve both copies. */
            else if (source_exists && !destination_exists)
                recovery_error = ENOENT;   /* Preserve the source. */
        }
        else /* STARTED, including legacy records */
        {
            if (!source_exists && destination_exists)
            {
                /* Without a durable PUBLISHED record, destination ownership
                 * is ambiguous. Never claim this operation created it. */
                errno = EUCLEAN;
                recovery_status = -1;
                break;
            }
            if (!source_exists && !destination_exists)
            {
                errno = EUCLEAN;
                recovery_status = -1;
                break;
            }
            if (source_exists && destination_exists)
                recovery_error = EALREADY; /* Preserve both copies. */
        }

        if (journal_record_recovered(&journal, record->src,
                                     record->dest, recovery_error) != 0)
        {
            recovery_status = -1;
            break;
        }
    }

    saved_errno = errno;
    if (journal_close(&journal) != 0 && recovery_status == 0)
    {
        recovery_status = -1;
        saved_errno = errno;
    }
    free(records);

    if (recovery_status != 0)
        errno = saved_errno ? saved_errno : EIO;
    return recovery_status;
}

int journal_open(MoveJournal *journal, const char *path)
{
    if (!journal || !path || !path[0])
    {
        errno = EINVAL;
        return -1;
    }
    if (strlen(path) >= sizeof(journal->path))
    {
        errno = ENAMETOOLONG;
        return -1;
    }

    memset(journal, 0, sizeof(*journal));
    char leaf[PATH_MAX];
    int parent_fd = open_parent_dir(path, leaf);
    if (parent_fd < 0)
        return -1;

    int fd;
    do {
        fd = openat(parent_fd, leaf,
                    O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK,
                    0600);
    } while (fd < 0 && errno == EINTR);
    int open_errno = errno;
    close(parent_fd);
    if (fd < 0) {
        errno = open_errno;
        return -1;
    }

    int lock_rc;
    do
    {
        lock_rc = flock(fd, LOCK_EX | LOCK_NB);
    } while (lock_rc != 0 && errno == EINTR);
    if (lock_rc != 0)
    {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) != 0)
    {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }
    if (!S_ISREG(st.st_mode))
    {
        close(fd);
        errno = EINVAL;
        return -1;
    }
    if (fchmod(fd, 0600) != 0)
    {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }

    FILE *stream = fdopen(fd, "a");
    if (!stream)
    {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }

    int mutex_error = pthread_mutex_init(&journal->lock, NULL);
    if (mutex_error != 0)
    {
        fclose(stream);
        errno = mutex_error;
        return -1;
    }

    journal->fp = stream;
    memcpy(journal->path, path, strlen(path) + 1);
    journal->enabled = 1;
    return 0;
}

int journal_close(MoveJournal *journal)
{
    if (!journal || !journal->enabled)
        return 0;

    int status = 0;
    int saved_errno = 0;
    pthread_mutex_lock(&journal->lock);

    if (fflush(journal->fp) != 0)
    {
        status = -1;
        saved_errno = errno;
    }
    if (fsync(fileno(journal->fp)) != 0 && status == 0)
    {
        status = -1;
        saved_errno = errno;
    }

    FILE *stream = journal->fp;
    journal->fp = NULL;
    journal->enabled = 0;
    pthread_mutex_unlock(&journal->lock);

    if (fclose(stream) != 0 && status == 0)
    {
        status = -1;
        saved_errno = errno;
    }
    pthread_mutex_destroy(&journal->lock);

    if (status != 0)
        errno = saved_errno;
    return status;
}

static int journal_record(MoveJournal *journal, const char *state,
                          const char *src, const char *dest, int error_code)
{
    if (!journal || !journal->enabled)
        return 0;

    pthread_mutex_lock(&journal->lock);
    int rc = append_record(journal->fp, state, src, dest, error_code);
    int saved_errno = errno;
    pthread_mutex_unlock(&journal->lock);
    errno = saved_errno;
    return rc;
}

int journal_record_started(MoveJournal *journal, const char *src, const char *dest)
{
    return journal_record(journal, "STARTED", src, dest, 0);
}

int journal_record_published(MoveJournal *journal, const char *src, const char *dest)
{
    return journal_record(journal, "PUBLISHED", src, dest, 0);
}

int journal_record_completed(MoveJournal *journal, const char *src, const char *dest)
{
    return journal_record(journal, "COMPLETED", src, dest, 0);
}

int journal_record_failed(MoveJournal *journal, const char *src,
                          const char *dest, int error_code)
{
    return journal_record(journal, "FAILED", src, dest, error_code);
}

int journal_record_recovered(MoveJournal *journal, const char *src,
                             const char *dest, int error_code)
{
    return journal_record(journal, "RECOVERED", src, dest, error_code);
}
