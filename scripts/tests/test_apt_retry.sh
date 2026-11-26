#!/bin/sh
# Exercise the actual Dockerfile RUN body with a private PATH of stubs.
# No real APT, list removal, network access or backoff sleep is performed.
set -eu
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 1' HUP INT TERM
mkdir "$tmp/bin"
awk '
  /^RUN set -eu; / { if (++found != 1) exit 2; active=1; sub(/^RUN /,"") }
  active { print; if (substr($0,length($0),1) != "\\") active=0 }
  END { if (found != 1 || active) exit 2 }
' Dockerfile > "$tmp/run"
[ -s "$tmp/run" ]
/bin/sh -n "$tmp/run"
cat > "$tmp/bin/stub" <<'STUB'
#!/bin/sh
set -eu
case "$0" in
    */apt-get)
        if [ "$#" -eq 3 ] && [ "$1" = -o ] &&
           [ "$2" = APT::Update::Error-Mode=any ] && [ "$3" = update ]; then
            n=0
            if [ -f "$STATE/updates" ]; then read -r n < "$STATE/updates"; fi
            n=$((n + 1))
            printf '%s\n' "$n" > "$STATE/updates"
            printf 'update\n' >> "$STATE/log"
            [ "$n" -ge "$SUCCESS_AT" ] || exit 100
            : > "$STATE/verified"
        elif [ "$#" -eq 6 ] && [ "$1" = install ] && [ "$2" = -y ] &&
             [ "$3" = --no-install-recommends ] && [ "$4" = build-essential ] &&
             [ "$5" = cmake ] && [ "$6" = ninja-build ]; then
            printf 'install\n' >> "$STATE/log"
            [ -f "$STATE/verified" ] || exit 93
            [ "$INSTALL_FAIL" -eq 0 ] || exit 100
        else
            printf 'unexpected-apt\n' >> "$STATE/log"
            exit 94
        fi ;;
    */rm)
        [ "$#" -ge 2 ] && [ "$1" = -rf ] || exit 95
        shift
        for arg do
            case "$arg" in /var/lib/apt/lists/*) ;; *) exit 95 ;; esac
        done
        n=0
        if [ -f "$STATE/clears" ]; then read -r n < "$STATE/clears"; fi
        n=$((n + 1))
        printf '%s\n' "$n" > "$STATE/clears"
        printf 'clear\n' >> "$STATE/log"
        [ "$n" -ne "$CLEAR_FAIL_AT" ] || exit 91
        # State models whether the current refresh succeeded. Do not use rm.
        if [ -f "$STATE/verified" ]; then : > "$STATE/cleared_after_verified"; fi
        ;;
    */sleep)
        [ "$#" -eq 1 ] || exit 96
        case "$1" in 2|4) printf 'sleep %s\n' "$1" >> "$STATE/log";; *) exit 96;; esac ;;
    *) exit 97 ;;
esac
STUB
chmod +x "$tmp/bin/stub"
for name in apt-get rm sleep; do ln -s stub "$tmp/bin/$name"; done
run_case() {
    name=$1; success=$2; install_fail=$3; clear_fail=$4; expected_status=$5
    expected=$6
    state="$tmp/$name"
    mkdir "$state"
    : > "$state/log"
    status=0
    # timeout and env resolve before the restricted child PATH is applied.
    timeout 10 env PATH="$tmp/bin" STATE="$state" SUCCESS_AT="$success" \
        INSTALL_FAIL="$install_fail" CLEAR_FAIL_AT="$clear_fail" \
        /bin/sh "$tmp/run" > "$state/output" 2>&1 || status=$?
    actual=$(cat "$state/log")
    if [ "$status" -ne "$expected_status" ] || [ "$actual" != "$expected" ]; then
        printf 'APT oracle %s: status %s expected %s\n%s\n' \
            "$name" "$status" "$expected_status" "$actual" >&2
        cat "$state/output" >&2
        exit 1
    fi
    printf 'apt_retry_case=%s PASS\n' "$name"
}
run_case first 1 0 0 0 'clear
update
install
clear'
run_case second 2 0 0 0 'clear
update
sleep 2
clear
update
install
clear'
run_case third 3 0 0 0 'clear
update
sleep 2
clear
update
sleep 4
clear
update
install
clear'
run_case exhausted 4 0 0 1 'clear
update
sleep 2
clear
update
sleep 4
clear
update'
run_case install_failure 1 1 0 100 'clear
update
install'
run_case initial_cleanup_failure 1 0 1 91 'clear'
run_case final_cleanup_failure 1 0 2 91 'clear
update
install
clear'
printf 'apt_retry_oracle=PASS cases=7 success=3 exhaustion=1 install_failure=1 cleanup_failure=2\n'
