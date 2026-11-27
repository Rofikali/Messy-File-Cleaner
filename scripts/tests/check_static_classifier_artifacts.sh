#!/bin/sh
# Private archive isolation: no production install or production-only target.
set -eu
if [ "$#" -ne 8 ]; then
    echo "usage: $0 CMAKE SOURCE BUILD CLI ARCHIVE GENERATOR COMPILER STANDARD" >&2
    exit 2
fi
cmake_cmd=$1
source=$2
build=$3
cli=$4
archive=$5
generator=$6
compiler=$7
standard=$8
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
export LC_ALL=C
unset DESTDIR

test -f "$archive"
sh "$source/scripts/tests/check_static_classifier_symbols.sh" "$archive"
# A fresh production-only build must succeed and lack both private targets.
"$cmake_cmd" -S "$source" -B "$work/off" -G "$generator" \
    "-DCMAKE_C_COMPILER=$compiler" "-DFC_C_STANDARD=$standard" \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
"$cmake_cmd" --build "$work/off" --target file-cleaner --parallel 2
for target in fc_classifier_reference static_classifier_consumer; do
    if "$cmake_cmd" --build "$work/off" --target "$target" > "$work/negative" 2>&1; then
        echo "private target leaked into production-only configuration: $target" >&2
        exit 1
    fi
    # Require the specific absence diagnosis, not an arbitrary build failure.
    if ! grep -F "unknown target '$target'" "$work/negative" &&
       ! grep -F "No rule to make target '$target'" "$work/negative"; then
        cat "$work/negative" >&2
        exit 1
    fi
done
test ! -e "$work/off/libfc_classifier_reference.a"
test ! -e "$work/off/static_classifier_consumer"

stage="$work/install"
"$cmake_cmd" --install "$build" --config Release --prefix "$stage"
check_stage() {
    find "$stage" -mindepth 1 ! -type d -print > "$work/actual" || return 1
    printf '%s\n' "$stage/bin/file-cleaner" > "$work/expected"
    cmp -s "$work/expected" "$work/actual" &&
        test -f "$stage/bin/file-cleaner" &&
        test ! -L "$stage/bin/file-cleaner" &&
        cmp -s "$cli" "$stage/bin/file-cleaner"
}
check_stage
# Negative controls: an installed archive and an altered CLI must be rejected.
cp "$archive" "$stage/unexpected.a"
if check_stage; then
    echo "archive leak control unexpectedly passed" >&2
    exit 1
fi
rm "$stage/unexpected.a"
check_stage
printf '\n' >> "$stage/bin/file-cleaner"
if check_stage; then
    echo "altered CLI control unexpectedly passed" >&2
    exit 1
fi
printf 'static_classifier_artifact_isolation=PASS controls=2\n'
