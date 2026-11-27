/* Internal archive reference consumer; not an SDK or public ABI. */
#include "classifier/classifier.h"
#include <stdio.h>
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

    puts("static_classifier_consumer=PASS checks=8");
    return 0;
}
