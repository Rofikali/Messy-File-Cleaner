#!/bin/sh
# Read-only maintenance evidence, not a vulnerability or capability verdict.
set -eu
export LC_ALL=C
if [ "$#" -ne 3 ]; then
    echo "usage: $0 CMAKE COMPILER LINKER" >&2
    exit 2
fi
kernel=$(uname -s)
release=$(uname -r)
machine=$(uname -m)
libc=$(getconf GNU_LIBC_VERSION)
[ "$kernel" = Linux ]
[ -n "$release" ] && [ -n "$machine" ]
case "$libc" in
    glibc\ *) ;;
    *) echo "unsupported libc identity" >&2; exit 1 ;;
esac
# Capture command status before extracting the first line.
cmake_version=$("$1" --version)
compiler_version=$("$2" --version)
linker_version=$("$3" --version)
first_line() {
    printf '%s\n' "$1" | awk 'NR == 1 { if (length($0) == 0) exit 1; print; found=1 } END { if (!found) exit 1 }'
}
cmake_id=$(first_line "$cmake_version")
compiler_id=$(first_line "$compiler_version")
linker_id=$(first_line "$linker_version")
printf 'upstream_environment_kernel=%s\n' "$release"
printf 'upstream_environment_machine=%s\n' "$machine"
printf 'upstream_environment_libc=%s\n' "$libc"
printf 'upstream_environment_cmake=%s\n' "$cmake_id"
printf 'upstream_environment_compiler=%s\n' "$compiler_id"
printf 'upstream_environment_configured_linker=%s\n' "$linker_id"
printf 'upstream_environment_report=COMPLETE fields=6\n'
