// ./src/utils/hashmap.c
#include <string.h>
#include <stdio.h>
#include "utils/hashmap.h"

static unsigned long hash_str(const char *str)
{
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; // djb2
    return hash;
}

void hashmap_init(HashMap *map, HashEntry *buffer, size_t capacity)
{
    map->entries = buffer;
    map->capacity = capacity;
    map->size = 0;

    for (size_t i = 0; i < capacity; i++)
    {
        map->entries[i].used = 0;
    }
}

int hashmap_put(HashMap *map, const char *key, const char *value)
{
    unsigned long hash = hash_str(key);
    size_t idx = hash & (map->capacity - 1);

    for (size_t i = 0; i < map->capacity; i++)
    {
        HashEntry *entry = &map->entries[idx];

        if (!entry->used || strcmp(entry->key, key) == 0)
        {
            // Use MAX_KEY_LEN - 1 to leave room for \0
            strncpy(entry->key, key, MAX_KEY_LEN - 1);
            entry->key[MAX_KEY_LEN - 1] = '\0'; // Force null termination

            strncpy(entry->value, value, MAX_VAL_LEN - 1);
            entry->value[MAX_VAL_LEN - 1] = '\0'; // Force null termination

            entry->used = 1;
            map->size++;
            return 0;
        }
        idx = (idx + 1) & (map->capacity - 1);
    }
    return -1;
}

// int hashmap_put(HashMap *map, const char *key, const char *value)
// {
//     unsigned long hash = hash_str(key);
//     size_t idx = hash & (map->capacity - 1);

//     for (size_t i = 0; i < map->capacity; i++)
//     {
//         HashEntry *entry = &map->entries[idx];

//         if (!entry->used || strcmp(entry->key, key) == 0)
//         {
//             strncpy(entry->key, key, MAX_KEY_LEN);
//             strncpy(entry->value, value, MAX_VAL_LEN);
//             entry->used = 1;
//             map->size++;
//             return 0;
//         }

//         idx = (idx + 1) & (map->capacity - 1);
//     }

//     return -1; // full
// }

const char *hashmap_get(HashMap *map, const char *key)
{
    unsigned long hash = hash_str(key);
    size_t idx = hash & (map->capacity - 1);

    for (size_t i = 0; i < map->capacity; i++)
    {
        HashEntry *entry = &map->entries[idx];

        if (!entry->used)
            return NULL;

        if (strcmp(entry->key, key) == 0)
            return entry->value;

        idx = (idx + 1) & (map->capacity - 1);
    }

    return NULL;
}