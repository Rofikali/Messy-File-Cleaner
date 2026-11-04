#!/usr/bin/env bash
set -euo pipefail

BIN="$1"
CONFIG="$2"
ROOT="$(mktemp -d)"
cleanup() {
  if [[ -n "${WATCH_PID:-}" ]]; then
    kill -TERM "$WATCH_PID" 2>/dev/null || true
    wait "$WATCH_PID" 2>/dev/null || true
  fi
  rm -rf "$ROOT"
}
trap cleanup EXIT

mkdir -p "$ROOT/input" "$ROOT/journal" "$ROOT/watch"
cd "$ROOT"

# Dry-run must not modify source files or publish categorized output.
printf 'dry run fixture\n' > input/dry.jpg
"$BIN" "$ROOT/input" --config "$CONFIG" --dry-run --once >dry.stdout 2>dry.stderr
test -f input/dry.jpg
test ! -e Jpeg/dry.jpg
grep -q 'dry_run_moves=1' dry.stderr
rm input/dry.jpg

# Real batch mode must move a fixture and persist a journal record.
printf 'move fixture\n' > input/move.png
"$BIN" "$ROOT/input" --config "$CONFIG" --once >move.stdout 2>move.stderr
test ! -e input/move.png
test -f Png/move.png
test -s journal/move.log
grep -q 'files_moved=1' move.stderr

# A missing root must fail visibly instead of reporting a successful batch.
if "$BIN" "$ROOT/missing" --config "$CONFIG" --dry-run --once >missing.stdout 2>missing.stderr; then
  echo "Expected missing-root scan to fail" >&2
  exit 1
fi
grep -q 'Directory scan failed' missing.stderr

# Watch mode must terminate cooperatively on SIGTERM with conventional status.
"$BIN" "$ROOT/watch" --config "$CONFIG" --dry-run --watch >watch.stdout 2>watch.stderr &
WATCH_PID=$!
sleep 0.3
kill -TERM "$WATCH_PID"
set +e
wait "$WATCH_PID"
WATCH_STATUS=$?
set -e
WATCH_PID=
test "$WATCH_STATUS" -eq 143
grep -q 'signals_received=1' watch.stderr

echo "CLI end-to-end tests passed"
