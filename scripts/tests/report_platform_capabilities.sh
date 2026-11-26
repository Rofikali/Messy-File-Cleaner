#!/bin/sh
# Report actual CTest outcomes for the existing capability-sensitive tests.
# A skipped test is NOT_VERIFIED, never evidence that the feature works.
set -eu
export LC_ALL=C
[ "$#" -eq 2 ] || exit 2
build=$1
environment=$2
case "$environment" in
    container_release|hosted_debug) ;;
    *) exit 2 ;;
esac
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 1' HUP INT TERM
passed=0
skipped=0
for name in linux_uring_capability linux_io_reference linux_path_reference fcj3_writer_unit otmpfile_capability; do
    if ! ctest --test-dir "$build" -R "^$name$" --no-tests=error \
        --verbose --output-on-failure > "$tmp/result" 2>&1; then
        cat "$tmp/result" >&2
        printf 'platform_capability=FAIL environment=%s test=%s\n' "$environment" "$name" >&2
        exit 1
    fi
    cat "$tmp/result"
    outcome=$(awk -v name="$name" '
        $1 == "1/1" && $2 == "Test" && $4 == name {
            if ($0 ~ /\*\*\*Skipped[[:space:]]/) { status="NOT_VERIFIED"; count++ }
            else if ($0 ~ /[[:space:]]Passed[[:space:]]/) { status="PASS"; count++ }
        }
        END { if (count != 1) exit 1; print status }
    ' "$tmp/result")
    case "$outcome" in
        PASS) passed=$((passed + 1)) ;;
        NOT_VERIFIED) skipped=$((skipped + 1)) ;;
        *) exit 1 ;;
    esac
    printf 'platform_capability=%s environment=%s test=%s\n' "$outcome" "$environment" "$name"
done
[ "$((passed + skipped))" -eq 5 ]
printf 'platform_capability_report=COMPLETE environment=%s passed=%s skipped=%s failed=0\n' \
    "$environment" "$passed" "$skipped"
