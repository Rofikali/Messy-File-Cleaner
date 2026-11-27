#!/bin/sh
# Inspect the private unsanitized Release archive, not a public ABI.
set -eu
if [ "$#" -ne 1 ]; then
    echo "usage: check_static_classifier_dependencies.sh ARCHIVE" >&2
    exit 2
fi
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
export LC_ALL=C
for member in classifier.c.o hashmap.c.o; do
    ar p "$1" "$member" > "$work/member.o"
    nm -u -P "$work/member.o" > "$work/undefined"
    if ! awk -v member="$member" '
        NF == 0 { next }
        NF != 2 || $2 != "U" { bad = 1; next }
        member == "classifier.c.o" {
            if ($1 == "hashmap_get") { edge = 1; next }
            if ($1 == "strrchr") next
            bad = 1; next
        }
        member == "hashmap.c.o" {
            if ($1 ~ /^(strlen|strcmp|memcpy|memmove|memset)$/) next
            bad = 1; next
        }
        END {
            if (member == "classifier.c.o" && !edge) bad = 1
            exit bad
        }
    ' "$work/undefined"; then
        echo "unexpected archive dependency: $member" >&2
        cat "$work/undefined" >&2
        exit 1
    fi
    printf 'static_classifier_dependencies=PASS member=%s\n' "$member"
done
