#include <string.h>
#include <stdint.h>

#include "utils/hashmap.h"

static unsigned long hash_str(const char *str)
{
    unsigned long hash = 5381UL;
    const unsigned char *cursor = (const unsigned char *)str;

    while (*cursor != '\0')
    {
        hash = ((hash << 5) + hash) + (unsigned long)*cursor;
        cursor++;
    }

    return hash;
}

static int valid_map(const HashMap *map)
{
    return map != NULL &&
           map->entries != NULL &&
           map->capacity != 0 &&
           (map->capacity & (map->capacity - 1)) == 0;
}

void hashmap_init(HashMap *map, HashEntry *buffer, size_t capacity)
{
    if (map == NULL)
        return;

    map->entries = NULL;
    map->capacity = 0;
    map->size = 0;

    if (buffer == NULL || capacity == 0 ||
        (capacity & (capacity - 1)) != 0)
        return;

    map->entries = buffer;
    map->capacity = capacity;

    for (size_t i = 0; i < capacity; i++)
    {
        map->entries[i].key[0] = '\0';
        map->entries[i].value[0] = '\0';
        map->entries[i].used = 0;
    }
}

int hashmap_put(HashMap *map, const char *key, const char *value)
{
    if (!valid_map(map) || key == NULL || value == NULL)
        return -1;

    size_t key_len = strlen(key);
    size_t value_len = strlen(value);
    if (key_len == 0 || key_len >= MAX_KEY_LEN || value_len >= MAX_VAL_LEN)
        return -1;

    size_t idx = (size_t)(hash_str(key) & (unsigned long)(map->capacity - 1));

    for (size_t i = 0; i < map->capacity; i++)
    {
        HashEntry *entry = &map->entries[idx];

        if (!entry->used)
        {
            memcpy(entry->key, key, key_len + 1);
            memcpy(entry->value, value, value_len + 1);
            entry->used = 1;
            map->size++;
            return 0;
        }

        if (strcmp(entry->key, key) == 0)
        {
            memcpy(entry->value, value, value_len + 1);
            return 0;
        }

        idx = (idx + 1) & (map->capacity - 1);
    }

    return -1;
}

const char *hashmap_get(HashMap *map, const char *key)
{
    if (!valid_map(map) || key == NULL || key[0] == '\0' ||
        strlen(key) >= MAX_KEY_LEN)
        return NULL;

    size_t idx = (size_t)(hash_str(key) & (unsigned long)(map->capacity - 1));

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
