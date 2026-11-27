#!/bin/sh
# Private shared reference must never become a production dependency or install.
set -eu
if [ "$#" -ne 9 ]; then
    echo "usage: $0 CMAKE SOURCE BUILD CLI DSO RUNTIME GENERATOR COMPILER STANDARD" >&2
    exit 2
fi
cmake_cmd=$1
source=$2
build=$3
cli=$4
library=$5
runtime=$6
generator=$7
compiler=$8
standard=$9
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
export LC_ALL=C
unset DESTDIR

# Positive evidence: actual private DSO exists and exports its adapter.
test -f "$library"
nm -D -g --defined-only -P "$library" > "$work/exports"
grep -E '^fc_shared_reference_route T ' "$work/exports"
check_no_private_link() {
    readelf -dW "$1" > "$work/dynamic" || return 1
    if grep -E '\(NEEDED\).*libfc_classifier_shared_reference' "$work/dynamic"; then
        return 1
    fi
    nm -g -P "$1" > "$work/symbols" || return 1
    if grep -E '^fc_shared_reference_' "$work/symbols"; then
        return 1
    fi
}
check_no_private_link "$cli"
check_no_private_link "$runtime"
# Positive control for the dependency detector: the linked consumer needs the DSO.
readelf -dW "$build/shared_classifier_consumer" > "$work/linked"
grep -E '\(NEEDED\).*\[libfc_classifier_shared_reference\.so\.0\]' "$work/linked"
if check_no_private_link "$build/shared_classifier_consumer"; then
    echo "private dependency control unexpectedly passed" >&2
    exit 1
fi

"$cmake_cmd" -S "$source" -B "$work/off" -G "$generator" \
    "-DCMAKE_C_COMPILER=$compiler" "-DFC_C_STANDARD=$standard" \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
"$cmake_cmd" --build "$work/off" --target file-cleaner --parallel 2
for target in fc_classifier_shared_reference shared_classifier_consumer shared_classifier_runtime; do
    if "$cmake_cmd" --build "$work/off" --target "$target" > "$work/negative" 2>&1; then
        echo "private shared target leaked into production-only configuration: $target" >&2
        exit 1
    fi
    if ! grep -F "unknown target '$target'" "$work/negative" &&
       ! grep -F "No rule to make target '$target'" "$work/negative"; then
        cat "$work/negative" >&2
        exit 1
    fi
done
check_no_private_link "$work/off/file-cleaner"

check_stage() {
    find "$1" -mindepth 1 ! -type d -print > "$work/actual" || return 1
    printf '%s\n' "$1/bin/file-cleaner" > "$work/expected"
    cmp -s "$work/expected" "$work/actual" &&
        test -f "$1/bin/file-cleaner" &&
        test ! -L "$1/bin/file-cleaner" &&
        cmp -s "$2" "$1/bin/file-cleaner"
}
"$cmake_cmd" --install "$build" --config Release --prefix "$work/on-stage"
"$cmake_cmd" --install "$work/off" --config Release --prefix "$work/off-stage"
check_stage "$work/on-stage" "$cli"
check_stage "$work/off-stage" "$work/off/file-cleaner"
# Disposable controls for a leaked DSO and leaked SONAME symlink.
cp "$library" "$work/on-stage/unexpected.so"
if check_stage "$work/on-stage" "$cli"; then
    echo "DSO leak control unexpectedly passed" >&2
    exit 1
fi
rm "$work/on-stage/unexpected.so"
ln -s missing.so "$work/on-stage/unexpected.so.0"
if check_stage "$work/on-stage" "$cli"; then
    echo "SONAME symlink leak control unexpectedly passed" >&2
    exit 1
fi
rm "$work/on-stage/unexpected.so.0"
check_stage "$work/on-stage" "$cli"
printf 'shared_classifier_artifact_isolation=PASS absent_targets=3 negative_controls=3\n'
