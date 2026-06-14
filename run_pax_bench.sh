#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUNNER="${RUNNER:-$ROOT/build-release/benchmark/runner/benchmark_runner}"
BENCH_ROOT="${BENCH_ROOT:-$ROOT/benchmark/pax}"
OUT_DIR="${OUT_DIR:-$ROOT/benchmark/results/pax-dml-mutable-30}"
RUNS="${RUNS:-30}"
TIMEOUT="${TIMEOUT:-120}"
PAX_PAGE_ROWS_LIST="${PAX_PAGE_ROWS_LIST:-128 256 512 1024}"

if [[ ! -x "$RUNNER" ]]; then
    echo "benchmark_runner not found or not executable: $RUNNER" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"

SCENARIO_NAMES=(
    "fixed_update"
    "fixed_delete_insert"
    "string_update"
)
SCENARIO_FILES=(
    "$BENCH_ROOT/dml_fixed_update/update_toggle.benchmark"
    "$BENCH_ROOT/dml_fixed_delete_insert/delete_insert_restore.benchmark"
    "$BENCH_ROOT/dml_string_update/update_toggle.benchmark"
)

for rows in $PAX_PAGE_ROWS_LIST; do
    for i in "${!SCENARIO_NAMES[@]}"; do
        name="${SCENARIO_NAMES[$i]}"
        file="${SCENARIO_FILES[$i]}"
        out="$OUT_DIR/${name}_pax_mutable_rows_${rows}_${RUNS}.csv"

        echo "Running $name: pax_page_rows=$rows runs=$RUNS"
        "$RUNNER" \
            --disk \
            --layout=pax \
            --pax-mutable-dml-unsafe \
            --shared-load \
            --runs="$RUNS" \
            --timeout="$TIMEOUT" \
            --pax-page-rows="$rows" \
            --file="$file" \
            --out="$out"
    done
done

python3 - "$OUT_DIR" "$RUNS" <<'PY'
import csv
import pathlib
import sys

out_dir = pathlib.Path(sys.argv[1])
runs = int(sys.argv[2])

summary_csv = out_dir / f"pax_dml_mutable_page_rows_{runs}.csv"
summary_md = out_dir / f"pax_dml_mutable_page_rows_{runs}.md"

rows = []
for path in sorted(out_dir.glob(f"*_pax_mutable_rows_*_{runs}.csv")):
    if path.name.startswith("pax_dml_mutable_page_rows_"):
        continue
    with path.open(newline="") as fp:
        reader = csv.DictReader(fp)
        for row in reader:
            scenario = row["name"].split("/")[-2]
            page_rows = int(row["pax_page_rows"])
            update_fast = int(row["pax_mutable_update_fast"] or 0)
            update_fallback = int(row["pax_mutable_update_fallback"] or 0)
            delete_fast = int(row["pax_mutable_delete_fast"] or 0)
            delete_fallback = int(row["pax_mutable_delete_fallback"] or 0)
            insert_fast = int(row["pax_mutable_insert_fast"] or 0)
            insert_fallback = int(row["pax_mutable_insert_fallback"] or 0)
            pax_fixed = int(row["pax_scan_fixed_projected"] or 0)
            pax_generic = int(row["pax_scan_generic_projected"] or 0)
            regular = int(row["pax_scan_regular"] or 0)

            expected = {
                "fixed_update": (2 * runs, 0, 0),
                "string_update": (2 * runs, 0, 0),
                "fixed_delete_insert": (0, runs, runs),
            }.get(scenario)
            fast_ok = expected is not None and (update_fast, delete_fast, insert_fast) == expected
            fallback_ok = update_fallback == 0 and delete_fallback == 0 and insert_fallback == 0
            scan_ok = regular == 0 and (pax_fixed + pax_generic) > 0
            counter_status = "OK" if fast_ok and fallback_ok and scan_ok else "CHECK"

            rows.append({
                "scenario": scenario,
                "page_rows": page_rows,
                "runs": runs,
                "median_ms": row["median_ms"],
                "avg_ms": row["avg_ms"],
                "rsd_pct": row["rsd_pct"],
                "verified": row["verified"],
                "update_fast": update_fast,
                "delete_fast": delete_fast,
                "insert_fast": insert_fast,
                "update_fallback": update_fallback,
                "delete_fallback": delete_fallback,
                "insert_fallback": insert_fallback,
                "pax_fixed_projected": pax_fixed,
                "pax_generic_projected": pax_generic,
                "pax_regular": regular,
                "counter_status": counter_status,
                "error_message": row.get("error_message", ""),
            })

rows.sort(key=lambda r: (r["scenario"], r["page_rows"]))

fieldnames = [
    "scenario",
    "page_rows",
    "runs",
    "median_ms",
    "avg_ms",
    "rsd_pct",
    "verified",
    "update_fast",
    "delete_fast",
    "insert_fast",
    "update_fallback",
    "delete_fallback",
    "insert_fallback",
    "pax_fixed_projected",
    "pax_generic_projected",
    "pax_regular",
    "counter_status",
    "error_message",
]
with summary_csv.open("w", newline="") as fp:
    writer = csv.DictWriter(fp, fieldnames=fieldnames)
    writer.writeheader()
    writer.writerows(rows)

with summary_md.open("w") as fp:
    fp.write(f"# Mutable PAX DML page-size matrix ({runs} runs)\n\n")
    fp.write("Command shape: `benchmark_runner --disk --layout=pax --pax-mutable-dml-unsafe --shared-load`.\n\n")
    fp.write("| scenario | page_rows | median_ms | avg_ms | fast update/delete/insert | fallback update/delete/insert | PAX fixed/generic | regular | counters |\n")
    fp.write("|---|---:|---:|---:|---:|---:|---:|---:|---|\n")
    for row in rows:
        fast = f"{row['update_fast']}/{row['delete_fast']}/{row['insert_fast']}"
        fallback = f"{row['update_fallback']}/{row['delete_fallback']}/{row['insert_fallback']}"
        pax = f"{row['pax_fixed_projected']}/{row['pax_generic_projected']}"
        fp.write(
            f"| {row['scenario']} | {row['page_rows']} | {row['median_ms']} | {row['avg_ms']} | "
            f"{fast} | {fallback} | {pax} | {row['pax_regular']} | {row['counter_status']} |\n"
        )

print(f"Wrote {summary_csv}")
print(f"Wrote {summary_md}")
PY
