#!/bin/sh
# Compile each internal header first, twice, in an otherwise empty C TU.
# No linking, execution, public API claim, or modification of source files.
set -eu
export LC_ALL=C
root=$(cd "$(dirname "$0")/../.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 1' HUP INT TERM
(cd "$root/include" && find . -type f -name '*.h') > "$tmp/unsorted"
sort "$tmp/unsorted" > "$tmp/headers"
[ -s "$tmp/headers" ]
headers=$(wc -l < "$tmp/headers" | tr -d ' ')
cases=0
for compiler in gcc clang; do
    "$compiler" --version
    for standard in c11 c17 c2x; do
        for mode in normal testing; do
            set --
            if [ "$mode" = testing ]; then
                set -- -DFC_TESTING -DFC_SCANNER_TESTING -DFC_WATCHER_TESTING
            fi
            while IFS= read -r header; do
                printf '#include "%s"\n#include "%s"\ntypedef int fc_header_probe;\n' \
                    "$header" "$header" > "$tmp/probe.c"
                if ! "$compiler" -std="$standard" -Wall -Wextra -Wpedantic \
                    -Werror=implicit-function-declaration -Werror=incompatible-pointer-types \
                    -I "$root/include" "$@" -fsyntax-only "$tmp/probe.c" \
                    > "$tmp/diagnostics" 2>&1; then
                    printf 'internal_header=FAIL compiler=%s standard=%s mode=%s header=%s\n' \
                        "$compiler" "$standard" "$mode" "$header" >&2
                    cat "$tmp/diagnostics" >&2
                    exit 1
                fi
                if [ -s "$tmp/diagnostics" ]; then
                    printf 'internal_header_diagnostics compiler=%s standard=%s mode=%s header=%s\n' \
                        "$compiler" "$standard" "$mode" "$header"
                    cat "$tmp/diagnostics"
                fi
                cases=$((cases + 1))
            done < "$tmp/headers"
            printf 'internal_header_group=PASS compiler=%s standard=%s mode=%s headers=%s\n' \
                "$compiler" "$standard" "$mode" "$headers"
        done
    done
done
[ "$cases" -eq "$((headers * 12))" ]
printf 'internal_header_compat=PASS headers=%s compilers=2 standards=3 modes=2 cases=%s\n' \
    "$headers" "$cases"
