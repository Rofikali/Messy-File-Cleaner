#ifndef FC_ROOT_PATH_H
#define FC_ROOT_PATH_H

#include <errno.h>
#include <stdint.h>
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

/* Join nonempty lexical strings, adding a separator only when needed.
 * Returns an owned copy or NULL/errno; caller validates component policy.
 * Does not resolve or validate filesystem entries. */
static inline char *fc_join_path(const char *parent, const char *name)
{
    if (!parent || !parent[0] || !name || !name[0])
    {
        errno = EINVAL;
        return NULL;
    }

    size_t parent_len = strlen(parent);
    size_t name_len = strlen(name);
    size_t separator = parent[parent_len - 1] != '/' ? 1U : 0U;
    if (parent_len > SIZE_MAX - separator - 1 ||
        name_len > SIZE_MAX - parent_len - separator - 1)
    {
        errno = EOVERFLOW;
        return NULL;
    }

    char *joined = malloc(parent_len + separator + name_len + 1);
    if (!joined)
        return NULL;
    memcpy(joined, parent, parent_len);
    if (separator)
        joined[parent_len] = '/';
    memcpy(joined + parent_len + separator, name, name_len + 1);
    return joined;
}

#endif
