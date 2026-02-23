
// cleaner/src/core/organizer.c

#include <string.h>
#include <ctype.h>

#include "cleaner/core/organizer.h"

static void to_lowercase(const char *src,
                         char *dest,
                         size_t size)
{
    size_t i;

    for (i = 0; src[i] && i < size - 1; i++)
        dest[i] = (char)tolower((unsigned char)src[i]);

    dest[i] = '\0';
}

const char *get_extension_folder(const char *filename)
{
    static char lower_ext[64];

    const char *dot = strrchr(filename, '.');

    if (!dot || dot == filename)
        return "no_extension";

    to_lowercase(dot + 1, lower_ext, sizeof(lower_ext));

    return lower_ext;
}