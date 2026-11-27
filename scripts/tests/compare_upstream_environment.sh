#!/bin/sh
# Compare complete raw reports. Changes are review signals, never CVE verdicts.
set -eu
export LC_ALL=C
if [ "$#" -ne 2 ]; then
    echo "usage: $0 BASELINE_REPORT CANDIDATE_REPORT" >&2
    exit 2
fi
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
normalize() {
    awk '
        BEGIN {
            split("kernel machine libc cmake compiler configured_linker", keys, " ")
            for (i=1; i<=6; i++) allowed["upstream_environment_" keys[i]]=1
        }
        {
            if (complete) { bad=1; next }
            if ($0 == "upstream_environment_report=COMPLETE fields=6") {
                complete=1
                next
            }
            eq=index($0,"=")
            key=substr($0,1,eq-1)
            value=substr($0,eq+1)
            if (!eq || !(key in allowed) || key in seen ||
                value !~ /[^[:space:]]/ || value ~ /[[:cntrl:]]/) {
                bad=1
                next
            }
            seen[key]=value
        }
        END {
            if (bad || !complete) exit 1
            for (i=1; i<=6; i++) {
                key="upstream_environment_" keys[i]
                if (!(key in seen)) exit 1
            }
            for (i=1; i<=6; i++) {
                key="upstream_environment_" keys[i]
                print key "=" seen[key]
            }
        }
    ' "$1"
}
if ! normalize "$1" > "$work/base"; then
    echo "invalid baseline environment report" >&2
    exit 2
fi
if ! normalize "$2" > "$work/candidate"; then
    echo "invalid candidate environment report" >&2
    exit 2
fi
# Compare by ordered key, keeping raw values out of diagnostic output.
awk '
    NR==FNR { old[FNR]=$0; next }
    {
        if ($0 != old[FNR]) {
            key=substr($0,1,index($0,"=")-1)
            sub(/^upstream_environment_/,"",key)
            printf "upstream_environment_changed=%s\n", key
            changed++
        }
    }
    END {
        printf "upstream_environment_comparison=COMPLETE changed=%d fields=6\n", changed
    }
' "$work/base" "$work/candidate"
