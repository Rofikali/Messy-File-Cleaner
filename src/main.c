#include <stdio.h>
#include "scanner/scanner.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    scan_directory(argv[1]);
    return 0;
}