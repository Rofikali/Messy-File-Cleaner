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

// API
void classifier_init(Classifier *c, HashMap *map, const char *def);
// filename is a nonnull POSIX path; only the final component is classified.
// A leading dot alone or a trailing dot is not an extension.
const char *classify(Classifier *c, const char *filename);

#endif