#!/bin/sh
# Scoped Linux/glibc Release ELF checks; not a public ABI commitment.
set -eu
if [ "$#" -ne 3 ]; then
    echo "usage: $0 SHARED_OBJECT CONSUMER COMPILER" >&2
    exit 2
fi
library=$1
consumer=$2
compiler=$3
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
export LC_ALL=C

check_exports() {
    nm -D -g --defined-only -P "$1" > "$work/raw" || return 1
    awk 'NF == 0 { next } NF == 4 && $2 == "T" { print $1; next } { bad=1 } END { exit bad }' \
        "$work/raw" > "$work/exports" || return 1
    printf '%s\n' fc_shared_reference_route > "$work/expected"
    cmp -s "$work/exports" "$work/expected"
}
check_exports "$library" || { echo "unexpected dynamic export set" >&2; exit 1; }

readelf -hW "$library" > "$work/header"
grep -Eq 'Type:[[:space:]]+DYN([[:space:]]|$)' "$work/header"
readelf -dW "$library" > "$work/dynamic"
check_soname() {
    awk '$2 == "(SONAME)" {print $NF}' "$1" > "$work/soname"
    printf '%s\n' '[libfc_classifier_shared_reference.so.0]' > "$work/expected"
    cmp -s "$work/soname" "$work/expected"
}
check_soname "$work/dynamic" || { echo "unexpected private SONAME" >&2; exit 1; }
if grep -Eq '\((RPATH|RUNPATH|TEXTREL)\)|FLAGS.*TEXTREL' "$work/dynamic"; then
    echo "unexpected library search path or text relocation" >&2
    exit 1
fi
awk '$2 == "(NEEDED)" {print $NF}' "$work/dynamic" > "$work/needed"
printf '%s\n' '[libc.so.6]' > "$work/expected"
cmp "$work/needed" "$work/expected"

readelf -dW "$consumer" > "$work/consumer"
awk '$2 == "(NEEDED)" {print $NF}' "$work/consumer" | sort > "$work/needed"
printf '%s\n' '[libc.so.6]' '[libfc_classifier_shared_reference.so.0]' > "$work/expected"
sort -o "$work/expected" "$work/expected"
cmp "$work/needed" "$work/expected"
awk '$2 == "(RUNPATH)" {print $NF}' "$work/consumer" > "$work/runpath"
printf '%s\n' '[$ORIGIN]' > "$work/expected"
cmp "$work/runpath" "$work/expected"
if grep -F '(RPATH)' "$work/consumer"; then
    echo "legacy consumer RPATH is not permitted" >&2
    exit 1
fi

# Disposable negative controls: extra export and wrong SONAME.
printf '%s\n' 'int fc_shared_reference_route(const char *s) { return s != 0; }' \
    'int fc_unexpected_export(void) { return 0; }' > "$work/extra.c"
"$compiler" -std=c17 -fPIC -shared -Wl,-soname,libfc_classifier_shared_reference.so.0 \
    "$work/extra.c" -o "$work/extra.so"
if check_exports "$work/extra.so"; then
    echo "extra-export control unexpectedly passed" >&2
    exit 1
fi
printf '%s\n' 'int fc_shared_reference_route(const char *s) { return s != 0; }' > "$work/wrong.c"
"$compiler" -std=c17 -fPIC -shared -Wl,-soname,libwrong.so.0 \
    "$work/wrong.c" -o "$work/wrong.so"
check_exports "$work/wrong.so"
readelf -dW "$work/wrong.so" > "$work/wrong-dynamic"
if check_soname "$work/wrong-dynamic"; then
    echo "wrong-SONAME control unexpectedly passed" >&2
    exit 1
fi
printf 'shared_classifier_elf=PASS exports=1 controls=2\n'
