/* Private runtime loader experiment; not a production plugin loader. */
#define _POSIX_C_SOURCE 200809L
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

typedef int (*route_fn)(const char *);
_Static_assert(sizeof(route_fn) == sizeof(void *), "unsupported dlsym representation");

static int absent(void *handle, const char *name)
{
    (void)dlerror();
    void *symbol = dlsym(handle, name);
    const char *error = dlerror();
    return symbol == NULL && error != NULL;
}

static int exercise(const char *path)
{
    void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return 0;
    }

    int ok = 0;
    (void)dlerror();
    void *symbol = dlsym(handle, "fc_shared_reference_route");
    const char *error = dlerror();
    if (error || !symbol)
        goto cleanup;

    /* POSIX dlsym function address, copied without an ISO C pointer cast. */
    route_fn route;
    memcpy(&route, &symbol, sizeof(route));
    if (route("fixture/a.jpg") != 1 || route("a.txt") != 0 ||
        route(NULL) != -1 || route("") != -1)
        goto cleanup;

    const char *hidden[] = {
        "classifier_init", "classify", "hashmap_init",
        "hashmap_put", "hashmap_get", "fc_missing_reference_symbol"
    };
    for (size_t i = 0; i < sizeof(hidden) / sizeof(hidden[0]); ++i)
        if (!absent(handle, hidden[i]))
            goto cleanup;

    ok = 1;
cleanup:
    /* Neither the function pointer nor a borrowed loader error survives close. */
    if (dlclose(handle) != 0)
        ok = 0;
    return ok;
}

int main(int argc, char **argv)
{
    if (argc != 2 || argv[1][0] != '/')
        return 1;
    for (int i = 0; i < 16; ++i) {
        if (!exercise(argv[1])) {
            fprintf(stderr, "runtime loader cycle %d failed\n", i);
            return 1;
        }
    }
    puts("shared_classifier_runtime=PASS cycles=16 routes_per_cycle=4 rejected_symbols_per_cycle=6");
    return 0;
}
