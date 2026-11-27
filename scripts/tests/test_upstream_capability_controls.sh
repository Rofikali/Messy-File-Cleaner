#!/bin/sh
# Exercise the real reporter with disposable deterministic exit-status fixtures.
set -eu
export LC_ALL=C
[ "$#" -eq 4 ] || exit 2
source=$1
cmake=$2
compiler=$3
linker=$4
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
report="$source/scripts/tests/report_upstream_capabilities.sh"
fixture() {
    printf '#!/bin/sh\nexit %s\n' "$2" > "$work/$1"
    chmod 700 "$work/$1"
}
fixture pass 0
fixture skip 77
fixture fail 3
fixture unreachable 0
# A later probe must not execute after failure.
printf '#!/bin/sh\nprintf "executed\\n" > "%s"\n' "$work/reached" > "$work/unreachable"
run_report() {
    sh "$report" "$source" "$cmake" "$compiler" "$linker" "$@"
}
run_report "$work/pass" "$work/skip" - "$work/pass" "$work/pass" > "$work/report"
grep '^upstream_capability' "$work/report" > "$work/actual"
cat > "$work/expected" <<'EOF'
upstream_capability=linux_uring_capability status=PASS
upstream_capability=linux_io_reference status=NOT_VERIFIED
upstream_capability=linux_path_reference status=NOT_BUILT
upstream_capability=fcj3_writer_unit status=PASS
upstream_capability=otmpfile_capability status=PASS
upstream_capability_report=COMPLETE passed=3 skipped=1 not_built=1 fields=5
EOF
cmp "$work/expected" "$work/actual"

reject() {
    diagnostic=$1
    shift
    if run_report "$@" > "$work/report" 2> "$work/error"; then
        echo "invalid probe unexpectedly accepted" >&2
        exit 1
    fi
    grep -Fx "$diagnostic" "$work/error"
    if grep -q '^upstream_capability_report=COMPLETE' "$work/report"; then
        echo "failed probe emitted completion marker" >&2
        exit 1
    fi
    [ ! -e "$work/reached" ]
}
reject 'upstream_capability=linux_uring_capability status=FAIL exit=3' \
    "$work/fail" "$work/unreachable" "$work/pass" "$work/pass" "$work/pass"
reject 'configured probe executable missing: linux_uring_capability' \
    "$work/missing" "$work/unreachable" "$work/pass" "$work/pass" "$work/pass"
reject 'probe path must be absolute' \
    relative "$work/unreachable" "$work/pass" "$work/pass" "$work/pass"
# A genuine timeout: sleep is longer than the reporter's first probe deadline.
printf '#!/bin/sh\nexec sleep 30\n' > "$work/slow"
chmod 700 "$work/slow"
reject 'upstream_capability=linux_uring_capability status=FAIL exit=124' \
    "$work/slow" "$work/unreachable" "$work/pass" "$work/pass" "$work/pass"
printf 'upstream_capability_controls=PASS mixed=1 rejected=4\n'
