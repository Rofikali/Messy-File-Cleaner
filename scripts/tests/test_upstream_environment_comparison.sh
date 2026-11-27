#!/bin/sh
set -eu
export LC_ALL=C
[ "$#" -eq 4 ] || exit 2
source=$1
cmake=$2
compiler=$3
linker=$4
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
compare="$source/scripts/tests/compare_upstream_environment.sh"
sh "$source/scripts/tests/report_upstream_environment.sh" "$cmake" "$compiler" "$linker" > "$work/live"
sh "$compare" "$work/live" "$work/live" > "$work/result"
grep -Fx 'upstream_environment_comparison=COMPLETE changed=0 fields=6' "$work/result"

cat > "$work/base" <<'EOF'
upstream_environment_kernel=6.12.fixture
upstream_environment_machine=x86_64
upstream_environment_libc=glibc 2.40
upstream_environment_cmake=cmake version 3.31
upstream_environment_compiler=fixture compiler
upstream_environment_configured_linker=fixture linker
upstream_environment_report=COMPLETE fields=6
EOF
sed 's/6.12.fixture/6.13.fixture/; s/glibc 2.40/glibc 2.41/' "$work/base" > "$work/changed"
sh "$compare" "$work/base" "$work/changed" > "$work/result"
printf '%s\n' 'upstream_environment_changed=kernel' 'upstream_environment_changed=libc' \
    'upstream_environment_comparison=COMPLETE changed=2 fields=6' > "$work/expected"
cmp "$work/expected" "$work/result"

# Valid reports are insensitive to field order, not to value changes.
awk 'NR<=6 {line[NR]=$0} NR==7 {for(i=6;i>=1;i--) print line[i]; print}' "$work/base" > "$work/reordered"
sh "$compare" "$work/base" "$work/reordered" > "$work/result"
grep -Fx 'upstream_environment_comparison=COMPLETE changed=0 fields=6' "$work/result"

reject() {
    if sh "$compare" "$work/base" "$1" > "$work/result" 2> "$work/error"; then
        echo "malformed candidate unexpectedly accepted" >&2
        exit 1
    else
        status=$?
    fi
    [ "$status" -eq 2 ]
    grep -Fx 'invalid candidate environment report' "$work/error"
    [ ! -s "$work/result" ]
}
sed '/upstream_environment_libc=/d' "$work/base" > "$work/missing"
reject "$work/missing"
sed '/upstream_environment_kernel=/p' "$work/base" > "$work/duplicate"
reject "$work/duplicate"
sed 's/upstream_environment_machine=/upstream_environment_unknown=/' "$work/base" > "$work/unknown"
reject "$work/unknown"
sed 's/^upstream_environment_libc=.*/upstream_environment_libc=/' "$work/base" > "$work/empty"
reject "$work/empty"
sed '/upstream_environment_report=/d' "$work/base" > "$work/incomplete"
reject "$work/incomplete"
cp "$work/base" "$work/trailing"
printf '%s\n' 'unexpected trailing data' >> "$work/trailing"
reject "$work/trailing"
printf 'upstream_environment_comparison_tests=PASS positive=3 negative=6\n'
