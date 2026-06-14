#!/usr/bin/env python3
"""Cold-reopen vs warm round-trip correctness gate for the disk storage formats.

Invariant under test: for a given layout, a query must return the *same* result
whether the table is freshly loaded in-process (WARM) or persisted to disk,
the process restarted, and the table reopened from disk (COLD). Cold-reopen must
not change query results — anything else is a serialization/round-trip bug.

This is the suite-level counterpart to the unit tests in
components/table/test/test_checkpoint_load.cpp ("... preserves null validity ...").
It is what surfaced the columnar NULL-string round-trip bug: q6/q18/q24 of
ClickBench diverge on COLD only for the columnar layout, while PAX round-trips
correctly (cold == warm for every query).

Usage:
    roundtrip_verify.py --runner ./benchmark_runner --group '^clickbench' \
        --layout both [--pattern 'clickbench/queries/q\\d+$'] [--pax-page-rows 256]

Exit code: 0 if every comparable query round-trips on every requested layout,
1 if any divergence is found (so it can gate CI).
"""
import argparse
import csv
import os
import subprocess
import sys
import tempfile


def run(runner, cwd, args, timeout):
    """Invoke the benchmark runner; return (rc, stdout+stderr)."""
    proc = subprocess.run(
        [runner] + args,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        timeout=timeout,
    )
    return proc.returncode, proc.stdout


def load_hashes(csv_path):
    """name -> (result_hash, row_count, error_message) for rows with a usable result."""
    out = {}
    if not os.path.exists(csv_path):
        return out
    with open(csv_path, newline="") as f:
        for r in csv.DictReader(f):
            out[r["name"]] = (
                r.get("result_hash", ""),
                r.get("row_count", ""),
                (r.get("error_message", "") or "").strip(),
            )
    return out


def verify_layout(runner, cwd, group, layout, pattern, pax_page_rows, runs, timeout, outdir):
    """Run WARM and COLD for one layout, return list of divergent query names."""
    base = ["--disk", f"--layout={layout}", f"--group={group}",
            f"--pax-page-rows={pax_page_rows}", f"--runs={runs}",
            f"--timeout={timeout}"]
    pat = [pattern] if pattern else []
    warm_csv = os.path.join(outdir, f"warm_{layout}.csv")
    cold_csv = os.path.join(outdir, f"cold_{layout}.csv")

    # WARM: load each group once in-process, then time queries (no disk reopen).
    run(runner, cwd, base + ["--shared-load", f"--out={warm_csv}"] + pat, timeout * 60)
    # COLD: persist to disk, then a fresh invocation reopens from disk and scans.
    run(runner, cwd, base + ["--load-only"], timeout * 60)
    run(runner, cwd, base + ["--skip-load", f"--out={cold_csv}"] + pat, timeout * 60)

    warm = load_hashes(warm_csv)
    cold = load_hashes(cold_csv)

    diverged = []
    compared = 0
    for name in sorted(warm):
        wh, _, werr = warm[name]
        if name not in cold:
            continue
        ch, _, cerr = cold[name]
        if werr or cerr or not wh or not ch:
            continue  # query errored or produced no result in one mode — not comparable
        compared += 1
        if wh != ch:
            diverged.append((name, wh, ch))
    return diverged, compared


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--runner", default="./benchmark_runner",
                    help="path to benchmark_runner binary (default: ./benchmark_runner)")
    ap.add_argument("--cwd", default=None,
                    help="working dir for the runner (default: dir containing --runner)")
    ap.add_argument("--group", required=True, help="benchmark group regex, e.g. '^clickbench'")
    ap.add_argument("--pattern", default=None, help="optional benchmark-name regex filter")
    ap.add_argument("--layout", choices=["pax", "columnar", "both"], default="both")
    ap.add_argument("--pax-page-rows", type=int, default=256)
    ap.add_argument("--runs", type=int, default=1)
    ap.add_argument("--timeout", type=int, default=30, help="per-benchmark timeout seconds")
    args = ap.parse_args()

    runner = os.path.abspath(args.runner)
    cwd = args.cwd or os.path.dirname(runner)
    if not os.path.exists(runner):
        sys.exit(f"runner not found: {runner}")

    layouts = ["pax", "columnar"] if args.layout == "both" else [args.layout]
    any_diverged = False
    with tempfile.TemporaryDirectory(prefix="roundtrip_") as outdir:
        for layout in layouts:
            diverged, compared = verify_layout(
                runner, cwd, args.group, layout, args.pattern,
                args.pax_page_rows, args.runs, args.timeout, outdir)
            if diverged:
                any_diverged = True
                print(f"[{layout}] COLD != WARM on {len(diverged)}/{compared} comparable queries:")
                for name, wh, ch in diverged:
                    print(f"    {name}: warm={wh} cold={ch}")
            else:
                print(f"[{layout}] OK — all {compared} comparable queries round-trip (cold == warm)")

    sys.exit(1 if any_diverged else 0)


if __name__ == "__main__":
    main()
