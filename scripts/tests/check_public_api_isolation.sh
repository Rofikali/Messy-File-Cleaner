#!/bin/sh
# Test-only gate: candidate API must not leak into the production artifact.
set -eu
if [ "$#" -ne 3 ]; then
    echo "usage: check_public_api_isolation.sh PRODUCTION MOCK STAGE" >&2
    exit 2
fi
production=$1
mock=$2
stage=$3
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

# Fail closed on missing/unreadable symbol tables, not just absent matches.
nm -P "$mock" > "$work/mock.symbols"
nm -P "$production" > "$work/production.symbols"
for symbol in fc_candidate_contract_version fc_candidate_create fc_candidate_preview fc_candidate_destroy fc_candidate_example; do
    if ! awk -v s="$symbol" '$1 == s && $2 == "T" { found = 1 } END { exit !found }' "$work/mock.symbols"; then
        echo "positive mock symbol missing: $symbol" >&2
        exit 1
    fi
done
# A stripped/empty symbol table is not acceptable evidence.
if ! awk '$1 == "main" && $2 == "T" { found = 1 } END { exit !found }' "$work/production.symbols"; then
    echo "production main symbol missing" >&2
    exit 1
fi
if ! awk '$1 ~ /^fc_candidate_/ { found = 1 } END { exit found }' "$work/production.symbols"; then
    echo "candidate symbol leaked into production" >&2
    exit 1
fi
# Inspect a private staged install; expect exactly the existing CLI artifact.
find "$stage" -mindepth 1 ! -type d -print > "$work/install"
printf '%s\n' "$stage/bin/file-cleaner" > "$work/expected"
if ! cmp -s "$work/install" "$work/expected" ||
   [ ! -f "$stage/bin/file-cleaner" ] ||
   [ -L "$stage/bin/file-cleaner" ] ||
   ! cmp -s "$production" "$stage/bin/file-cleaner"; then
    echo "unexpected staged install content" >&2
    exit 1
fi
printf 'public_api_isolation=PASS\n'
