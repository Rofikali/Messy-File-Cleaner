#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "scanner/scanner.h"

typedef struct { DIR *directory; char *path; } ScanFrame;

static char *join_path(const char *parent, const char *name)
{
    size_t a = strlen(parent), b = strlen(name);
    size_t sep = a > 0 && parent[a - 1] != '/';
    if (b > SIZE_MAX - a - sep - 1) { errno = EOVERFLOW; return NULL; }
    char *out = malloc(a + sep + b + 1);
    if (!out) return NULL;
    memcpy(out, parent, a);
    if (sep) out[a++] = '/';
    memcpy(out + a, name, b);
    out[a + b] = '\0';
    return out;
}

static int push(ScanFrame **stack, size_t *count, size_t *capacity, DIR *dir, char *path)
{
    if (*count == *capacity) {
        size_t next = *capacity ? *capacity * 2 : 16;
        if (next < *capacity || next > SIZE_MAX / sizeof(**stack)) { errno = EOVERFLOW; return -1; }
        ScanFrame *grown = realloc(*stack, next * sizeof(**stack));
        if (!grown) return -1;
        *stack = grown; *capacity = next;
    }
    (*stack)[*count].directory = dir;
    (*stack)[*count].path = path;
    (*count)++;
    return 0;
}

int scan_directory(const char *path, event_handler_fn handler)
{
    if (!path || !path[0] || !handler) { errno = EINVAL; return -1; }
    int fd = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return -1;
    DIR *root = fdopendir(fd);
    if (!root) { int saved = errno; close(fd); errno = saved; return -1; }
    char *root_path = strdup(path);
    if (!root_path) { int saved = errno; closedir(root); errno = saved; return -1; }

    ScanFrame *stack = NULL;
    size_t count = 0, capacity = 0;
    int status = 0;
    if (push(&stack, &count, &capacity, root, root_path) != 0) {
        int saved = errno; closedir(root); free(root_path); errno = saved; return -1;
    }

    while (count) {
        ScanFrame *frame = &stack[count - 1];
        errno = 0;
        struct dirent *entry = readdir(frame->directory);
        if (!entry) {
            if (errno) status = -1;
            if (closedir(frame->directory) != 0) status = -1;
            free(frame->path);
            count--;
            continue;
        }
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;

        struct stat st;
        if (fstatat(dirfd(frame->directory), entry->d_name, &st, AT_SYMLINK_NOFOLLOW) != 0) {
            if (errno != ENOENT) status = -1;
            continue;
        }
        if (S_ISLNK(st.st_mode) || (!S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode))) continue;

        char *child_path = join_path(frame->path, entry->d_name);
        if (!child_path) { status = -1; continue; }

        if (S_ISDIR(st.st_mode)) {
            int child_fd = openat(dirfd(frame->directory), entry->d_name,
                                  O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
            if (child_fd < 0) {
                int saved = errno; free(child_path);
                if (saved != ENOENT && saved != ELOOP) status = -1;
                continue;
            }
            DIR *child = fdopendir(child_fd);
            if (!child) {
                int saved = errno; close(child_fd); free(child_path); errno = saved; status = -1; continue;
            }
            if (push(&stack, &count, &capacity, child, child_path) != 0) {
                closedir(child); free(child_path); status = -1;
            }
        } else {
            handler(child_path);
            free(child_path);
        }
    }
    free(stack);
    return status;
}
