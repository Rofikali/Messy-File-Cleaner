#ifndef FC_ROOT_PATH_H
#define FC_ROOT_PATH_H

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* Internal Linux lexical helper: return an owned copy, trimming only trailing
 * slashes while preserving a single root slash. Caller frees the result.
 * No filesystem access, dot resolution, ancestor validation or canonicalization.
 * NULL/empty input returns NULL/EINVAL; allocation failure propagates errno. */
static inline char *fc_copy_root_path(const char *path)
{
    if (!path || path[0] == '\0')
    {
        errno = EINVAL;
        return NULL;
    }

    size_t length = strlen(path);
    while (length > 1 && path[length - 1] == '/')
        --length;

    char *copy = malloc(length + 1);
    if (!copy)
        return NULL;
    memcpy(copy, path, length);
    copy[length] = '\0';
    return copy;
}

#endif
