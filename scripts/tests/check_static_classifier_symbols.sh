#!/bin/sh
# Private archive definition boundary; not a public export or ABI promise.
set -eu
if [ "$#" -ne 1 ]; then
    echo "usage: check_static_classifier_symbols.sh ARCHIVE" >&2
    exit 2
fi
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
export LC_ALL=C
nm -g --defined-only -P "$1" > "$work/raw"
# GNU/LLVM POSIX nm archive member headings have a single colon-ended field.
# Every other nonblank record must be a symbol, and all must be text definitions.
if ! awk '
    NF == 0 { next }
    NF == 1 && $1 ~ /:$/ { next }
    NF == 4 && $2 == "T" { print $1; next }
    { bad = 1 }
    END { exit bad }
' "$work/raw" > "$work/unsorted"; then
    echo "unexpected archive symbol record" >&2
    exit 1
fi
sort "$work/unsorted" > "$work/actual"
printf '%s\n' classifier_init classify hashmap_get hashmap_init hashmap_put > "$work/expected"
# Do not deduplicate: duplicate definitions are also a failure.
if ! cmp -s "$work/expected" "$work/actual"; then
    echo "unexpected archive global definitions" >&2
    diff -u "$work/expected" "$work/actual" >&2 || :
    exit 1
fi
printf 'static_classifier_symbols=PASS definitions=5\n'
