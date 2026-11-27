// ./include/utils/hashmap.h
#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>

#define MAX_KEY_LEN 16
#define MAX_VAL_LEN 64

typedef struct
{
    char key[MAX_KEY_LEN];   // ".jpg"
    char value[MAX_VAL_LEN]; // "Jpeg/"
    int used;
} HashEntry;

typedef struct
{
    HashEntry *entries;
    size_t capacity;
    size_t size;
} HashMap;

// API: caller owns initialized backing storage of at least capacity entries.
// Invalid capacity or size metadata is rejected; this cannot prove buffer extent.
void hashmap_init(HashMap *map, HashEntry *buffer, size_t capacity);
// Insertion cannot exceed declared capacity, even if an unused slot is found.
// Existing-key updates remain allowed at full capacity and accept borrowed values.
int hashmap_put(HashMap *map, const char *key, const char *value);
const char *hashmap_get(HashMap *map, const char *key);

#endif
