// ./src/classifier/classifier.c
#include <string.h>
#include "classifier/classifier.h"

static const char *extract_ext(const char *filename)
{
    /* Workers pass full POSIX paths; only the final component has an extension. */
    const char *slash = strrchr(filename, '/');
    const char *basename = slash ? slash + 1 : filename;
    const char *dot = strrchr(basename, '.');
    if (!dot || dot == basename || dot[1] == '\0')
        return NULL;
    return dot;
}

void classifier_init(Classifier *c, HashMap *map, const char *def)
{
    if (!c)
        return;
    c->map = map;
    c->default_folder = def;
}

const char *classify(Classifier *c, const char *filename)
{
    if (!c || !filename || filename[0] == '\0')
        return NULL;

    const char *ext = extract_ext(filename);

    if (!ext)
        return c->default_folder;

    const char *folder = hashmap_get(c->map, ext);
    if (folder)
        return folder;

    return c->default_folder;
}