// ./include/classifier/classifier.h
// Classifier/classifier.h

#ifndef CLASSIFIER_H
#define CLASSIFIER_H

#include "utils/hashmap.h"

typedef struct
{
    HashMap *map;
    const char *default_folder;
} Classifier;

// 2. Declare the global variable SECOND
extern Classifier classifier;

// API: a NULL initialization context is a no-op.
void classifier_init(Classifier *c, HashMap *map, const char *def);
// Returns NULL for a NULL context or NULL/empty filename.
// Otherwise only the final component of a POSIX path is classified.
// A leading dot alone or a trailing dot is not an extension.
const char *classify(Classifier *c, const char *filename);

#endif