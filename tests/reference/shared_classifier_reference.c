#include "shared_classifier_reference.h"
#include "classifier/classifier.h"
#include <string.h>

/* The actual classifier/hashmap symbols remain hidden in this DSO. */
__attribute__((visibility("default")))
int fc_shared_reference_route(const char *filename)
{
    HashEntry entries[8];
    HashMap map;
    Classifier context;
    if (!filename || !*filename)
        return -1;
    hashmap_init(&map, entries, 8);
    if (hashmap_put(&map, ".jpg", "photos") != 0)
        return -1;
    classifier_init(&context, &map, "other");
    const char *result = classify(&context, filename);
    if (!result)
        return -1;
    return strcmp(result, "photos") == 0 ? 1 : 0;
}
