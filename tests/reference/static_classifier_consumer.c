/* Internal archive reference consumer; not an SDK or public ABI. */
#include "classifier/classifier.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int equal(const char *actual, const char *expected)
{
    return actual != NULL && strcmp(actual, expected) == 0;
}

int main(void)
{
    HashEntry left_entries[8], right_entries[8];
    HashMap left_map, right_map;
    Classifier left, right;
    hashmap_init(&left_map, left_entries, 8);
    hashmap_init(&right_map, right_entries, 8);
    if (hashmap_put(&left_map, ".jpg", "photos") != 0 ||
        hashmap_put(&right_map, ".jpg", "images") != 0)
        return 1;

    classifier_init(&left, &left_map, "left-other");
    classifier_init(&right, &right_map, "right-other");

    /* Two caller-owned contexts must remain independent. */
    if (!equal(classify(&left, "fixture/a.jpg"), "photos") ||
        !equal(classify(&right, "fixture/a.jpg"), "images") ||
        !equal(classify(&left, "dir.jpg/no-extension"), "left-other") ||
        !equal(classify(&right, ".hidden"), "right-other") ||
        classify(&left, "") != NULL || classify(NULL, "a.jpg") != NULL)
        return 1;

    if (hashmap_put(&left_map, ".jpg", "changed") != 0 ||
        !equal(classify(&left, "a.jpg"), "changed") ||
        !equal(classify(&right, "a.jpg"), "images"))
        return 1;

    /* A separate owner exercises copied input and borrowed result lifetimes. */
    HashEntry *owned = calloc(2, sizeof(*owned));
    if (!owned)
        return 1;
    HashMap small_map;
    Classifier small;
    char key[] = ".txt";
    char value[] = "notes";
    char fallback[] = "other";
    int ok = 0;
    hashmap_init(&small_map, owned, 2);
    classifier_init(&small, &small_map, fallback);
    if (hashmap_put(&small_map, key, value) != 0)
        goto cleanup;
    key[1] = 'x';
    value[0] = 'X';
    if (!equal(classify(&small, "a.txt"), "notes"))
        goto cleanup;
    if (classify(&small, "a.bin") != fallback)
        goto cleanup;
    if (hashmap_put(&small_map, ".bin", "binary") != 0)
        goto cleanup;
    if (hashmap_put(&small_map, ".new", "rejected") == 0)
        goto cleanup;
    if (small_map.size != 2 || hashmap_get(&small_map, ".new") != NULL)
        goto cleanup;
    if (!equal(classify(&small, "a.txt"), "notes") ||
        !equal(classify(&small, "a.bin"), "binary"))
        goto cleanup;
    if (hashmap_put(&small_map, ".txt", "updated") != 0 ||
        !equal(classify(&small, "a.txt"), "updated"))
        goto cleanup;
    if (!equal(classify(&left, "a.jpg"), "changed") ||
        !equal(classify(&right, "a.jpg"), "images"))
        goto cleanup;
    ok = 1;
cleanup:
    /* All borrowers cease use before the owner releases its storage. */
    free(owned);
    if (!ok)
        return 1;
    puts("static_classifier_consumer=PASS base_checks=8 ownership_scenarios=8");
    return 0;
}
