#!/usr/bin/env bash
# Cold-cache PAX scan latency harness.
#
# True cold I/O requires both a fresh process (so the otterbrix buffer pool starts empty)
# and an evicted OS page cache. Each iteration therefore: evicts the state-root files from
# the page cache (posix_fadvise, see evict_cache.py) and runs `benchmark_runner --skip-load
# --runs=1` in a fresh process. The single timing per query is one cold sample; we collect
# ITERS samples per query and report the cold median.
#
# Usage:
#   benchmark/pax/run_cold.sh <layout> <profile> [iters]
# Example:
#   benchmark/pax/run_cold.sh pax fixed_analytic 15
#
# NOTE: columnar cold restart currently has known bugs (aggregate column mapping + null cursor
# on the error path) and will not complete; use layout=pax/auto here. See benchmark/pax/README.md.
set -euo pipefail
export LC_ALL=C  # stable '.' decimal separator in awk output

REPO="$(cd "$(dirname "$0")/../.." && pwd)"
RUNNER="${RUNNER:-$REPO/build-release/benchmark/runner/benchmark_runner}"
BENCHMARKS="$REPO/benchmark"
EVICT="$REPO/benchmark/pax/evict_cache.py"

LAYOUT="${1:?layout (pax|auto|columnar)}"
PROFILE="${2:?profile (fixed_analytic|mixed_schema|string_heavy)}"
ITERS="${3:-15}"

GROUP="^pax/${PROFILE}\$"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# State root mirrors benchmark_state_root() in the runner.
STATE_ROOT="${TMPDIR:-/tmp}/otterbrix-benchmark-runner/disk/${LAYOUT}"

echo ">>> load-only ${LAYOUT}/${PROFILE}"
"$RUNNER" --disk --layout="$LAYOUT" --load-only --group="$GROUP" --benchmarks="$BENCHMARKS" >/dev/null

echo ">>> ${ITERS} cold iterations (evict + fresh --skip-load --runs=1)"
for i in $(seq 1 "$ITERS"); do
    python3 "$EVICT" "$STATE_ROOT" >/dev/null 2>&1 || true
    "$RUNNER" --disk --layout="$LAYOUT" --skip-load --runs=1 --group="$GROUP" \
        --benchmarks="$BENCHMARKS" 2>/dev/null \
        | awk '/queries\/q[0-9]/ {print $1, $(NF-1)}' >> "$TMP/samples.txt"
done

echo ">>> cold medians (${PROFILE}, ${ITERS} samples/query)"
# For each query, sort its samples and take the median.
awk '
{ q=$1; v=$2; n[q]++; vals[q,n[q]]=v }
END {
    for (q in n) {
        cnt=n[q];
        for (a=1;a<=cnt;a++){ arr[a]=vals[q,a] }
        for (a=1;a<=cnt;a++) for (b=a+1;b<=cnt;b++) if (arr[b]<arr[a]){t=arr[a];arr[a]=arr[b];arr[b]=t}
        mid=(cnt%2)?arr[(cnt+1)/2]:(arr[cnt/2]+arr[cnt/2+1])/2;
        printf "%-40s cold_median_ms=%8.3f  (n=%d)\n", q, mid, cnt;
    }
}' "$TMP/samples.txt" | sort
