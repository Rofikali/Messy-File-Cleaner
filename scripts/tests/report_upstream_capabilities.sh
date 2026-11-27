#!/bin/sh
# Pair observed environment identity with existing disposable probe outcomes.
# Not a version-based capability inference or production safety verdict.
set -eu
export LC_ALL=C
if [ "$#" -ne 9 ]; then
    echo "usage: $0 SOURCE CMAKE COMPILER LINKER URING IO PATH WRITER OTMPFILE" >&2
    exit 2
fi
source=$1
sh "$source/scripts/tests/report_upstream_environment.sh" "$2" "$3" "$4"
shift 4
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
passed=0
skipped=0
not_built=0
probe() {
    name=$1
    binary=$2
    limit=$3
    if [ "$binary" = "-" ]; then
        not_built=$((not_built + 1))
        printf 'upstream_capability=%s status=NOT_BUILT\n' "$name"
        return
    fi
    case "$binary" in
        /*) ;;
        *) echo "probe path must be absolute" >&2; exit 2 ;;
    esac
    test -f "$binary" && test -x "$binary" || {
        echo "configured probe executable missing: $name" >&2
        exit 1
    }
    if timeout "$limit" "$binary" > "$work/result" 2>&1; then
        status=0
    else
        status=$?
    fi
    case "$status" in
        0) passed=$((passed + 1)); outcome=PASS ;;
        77) skipped=$((skipped + 1)); outcome=NOT_VERIFIED ;;
        *) cat "$work/result" >&2
           printf 'upstream_capability=%s status=FAIL exit=%s\n' "$name" "$status" >&2
           exit 1 ;;
    esac
    printf 'upstream_capability=%s status=%s\n' "$name" "$outcome"
}
probe linux_uring_capability "$1" 10
probe linux_io_reference "$2" 15
probe linux_path_reference "$3" 15
probe fcj3_writer_unit "$4" 10
probe otmpfile_capability "$5" 10
[ "$((passed + skipped + not_built))" -eq 5 ]
printf 'upstream_capability_report=COMPLETE passed=%s skipped=%s not_built=%s fields=5\n' \
    "$passed" "$skipped" "$not_built"
