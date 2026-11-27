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
"$cmake_cmd" --build "$build" --target help > "$work/test-targets"
has_target() {
    grep -Eq "(^|[[:space:]])$2([[:space:]:]|$)" "$1"
}
has_target "$work/test-targets" fc_classifier_reference
has_target "$work/test-targets" static_classifier_consumer

# Check the production-only configuration without building extra artifacts.
"$cmake_cmd" -S "$source" -B "$work/off" -G "$generator" \
    "-DCMAKE_C_COMPILER=$compiler" "-DFC_C_STANDARD=$standard" \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
"$cmake_cmd" --build "$work/off" --target help > "$work/off-targets"
has_target "$work/off-targets" file-cleaner
if has_target "$work/off-targets" fc_classifier_reference ||
   has_target "$work/off-targets" static_classifier_consumer; then
    echo "private target leaked into production-only configuration" >&2
    exit 1
fi

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
