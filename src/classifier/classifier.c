#include <string.h>
#include "classifier/classifier.h"

static const char *extract_ext(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return NULL;
    return dot;
}

void classifier_init(Classifier *c, HashMap *map, const char *def)
{
    c->map = map;
    c->default_folder = def;
}

const char *classify(Classifier *c, const char *filename)
{
    const char *ext = extract_ext(filename);

    if (!ext)
        return c->default_folder;

    const char *folder = hashmap_get(c->map, ext);
    if (folder)
        return folder;

    return c->default_folder;
}