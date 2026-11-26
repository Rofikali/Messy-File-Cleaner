#!/bin/sh
# Isolated fixtures: no workflow execution, remote access or real repository edits.
set -eu
export LC_ALL=C
root=$(pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 1' HUP INT TERM
sha=11d5960a326750d5838078e36cf38b85af677262
other=10e90e3645eae34f1e60eeb005ba3a3d33f178e8
workflow=.github/workflows/test.yml
inventory=docs/security/build-inputs.inventory
setup() {
    dir="$tmp/$1"
    mkdir -p "$dir/.github/workflows" "$dir/.devcontainer" "$dir/docs/security"
    cd "$dir"
    git init -q
    printf 'FROM ubuntu:24.04 AS build\n' > Dockerfile
    printf 'FROM ubuntu:24.04\n' > .devcontainer/Dockerfile
    write_workflow "actions/checkout@$sha # v4"
    printf '%s\n' \
        '.devcontainer/Dockerfile | FROM ubuntu:24.04' \
        "$workflow | runs-on: ubuntu-24.04" \
        "$workflow | uses: actions/checkout@$sha # v4" \
        'Dockerfile | FROM ubuntu:24.04 AS build' > "$inventory"
    git add .
}
write_workflow() {
    printf 'jobs:\n  test:\n    runs-on: ubuntu-24.04\n    steps:\n      - uses: %s\n' "$1" > "$workflow"
}
run_case() {
    name=$1; expected=$2; diagnostic=$3
    status=0
    timeout 10 sh "$root/scripts/check_build_inputs.sh" > "$tmp/$name.log" 2>&1 || status=$?
    if [ "$status" -ne "$expected" ] || ! grep -F "$diagnostic" "$tmp/$name.log" > /dev/null; then
        printf 'build-input fixture %s: status %s expected %s\n' "$name" "$status" "$expected" >&2
        cat "$tmp/$name.log" >&2
        exit 1
    fi
    printf 'build_input_case=%s PASS\n' "$name"
}
drift='Direct build references changed'
pin='direct action must use owner/repo@full lowercase commit SHA'
setup matching
run_case matching 0 'build_input_inventory=PASS references=4 files=3'
setup changed_sha
write_workflow "actions/checkout@$other # v4"
run_case changed_sha 1 "$drift"
setup changed_runner
sed -i 's/ubuntu-24.04/ubuntu-latest/' "$workflow"
run_case changed_runner 1 "$drift"
setup added_workflow
cp "$workflow" .github/workflows/extra.yaml
git add .github/workflows/extra.yaml
run_case added_workflow 1 "$drift"
setup missing_declaration
sed -i '/uses:/d' "$workflow"
run_case missing_declaration 1 "$drift"
setup duplicate_declaration
printf '      - uses: actions/checkout@%s # v4\n' "$sha" >> "$workflow"
run_case duplicate_declaration 1 "$drift"
setup changed_base
printf 'FROM ubuntu:22.04 AS build\n' > Dockerfile
run_case changed_base 1 "$drift"
setup synchronized_tag
write_workflow 'actions/checkout@v4'
sed -i "s/@$sha # v4/@v4/" "$inventory"
run_case synchronized_tag 2 "$pin"
setup synchronized_short
write_workflow 'actions/checkout@11d5960'
sed -i "s/@$sha # v4/@11d5960/" "$inventory"
run_case synchronized_short 2 "$pin"
setup synchronized_nonhex
bad=zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz
write_workflow "actions/checkout@$bad"
sed -i "s/@$sha # v4/@$bad/" "$inventory"
run_case synchronized_nonhex 2 "$pin"
setup reviewed_update
write_workflow "actions/checkout@$other # v4"
sed -i "s/@$sha /@$other /" "$inventory"
run_case reviewed_update 0 'build_input_inventory=PASS references=4 files=3'
cd "$root"
printf 'build_input_oracle=PASS cases=11 accepted=2 drift_rejected=6 pin_rejected=3\n'
