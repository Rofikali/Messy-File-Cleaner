#!/bin/sh
# Review gate for the declared direct build references, not a YAML parser,
# dependency resolver, signature verifier, or comprehensive supply-chain scan.
set -eu
export LC_ALL=C
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 1' HUP INT TERM
git ls-files -- '.github/workflows/*.yml' '.github/workflows/*.yaml' \
    Dockerfile .devcontainer/Dockerfile > "$tmp/paths"
[ -s "$tmp/paths" ]
while IFS= read -r path; do
    [ -f "$path" ] && [ ! -L "$path" ] || exit 1
    awk -v path="$path" '
        {
            line=$0
            sub(/^[[:space:]]+/, "", line)
            sub(/^- +/, "", line)
            sub(/[[:space:]]+$/, "", line)
            if (line ~ /^(uses:|runs-on:|python-version:)[[:space:]]/ ||
                line ~ /^FROM[[:space:]]/)
                print path " | " line
        }
    ' "$path"
done < "$tmp/paths" > "$tmp/actual"
sort "$tmp/actual" > "$tmp/sorted"
if ! diff -u docs/security/build-inputs.inventory "$tmp/sorted"; then
    echo "Direct build references changed: review and update the inventory in this PR" >&2
    exit 1
fi
printf 'build_input_inventory=PASS references=%s files=%s\n' \
    "$(wc -l < "$tmp/sorted" | tr -d ' ')" \
    "$(wc -l < "$tmp/paths" | tr -d ' ')"
