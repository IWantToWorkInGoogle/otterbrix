# PAX cold-cache scan latency

Methodology: `benchmark/pax/run_cold.sh <layout> <profile> <iters>`. Each sample is a fresh
`benchmark_runner --skip-load --runs=1` process (empty buffer pool) after evicting the state-root
files from the OS page cache (`evict_cache.py`, `posix_fadvise(DONTNEED)`). Release build. Median
over the per-sample timings. PAX/auto only — columnar cold restart crashes (see
`benchmark/pax/COLD_RESTART_NOTES.md`).

This is the regime where PAX's on-disk locality matters (a fresh process must read pages from disk),
unlike the warm `--shared-load` summary which measures CPU-bound in-memory scans.

## fixed_analytic (PAX, 12 samples/query)

| q1 | q2 | q3 | q4 | q5 | q6 | q7 | q8 |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 18.5 | 77.0 | 8.0 | 5.0 | 81.0 | 90.5 | 6.0 | 68.0 |

## mixed_schema (PAX, 10 samples/query)

| q1 | q2 | q3 | q4 | q5 | q6 | q7 | q8 |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 5.957 | 4.191 | 7.998 | 2.978 | 45.840 | 39.078 | 2.768 | 52.363 |

## string_heavy (PAX, 10 samples/query)

| q1 | q2 | q3 | q4 | q5 | q6 | q7 | q8 |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 27.641 | 4.681 | 8.312 | 1.651 | 33.975 | 87.612 | 1.660 | 6.828 |

All values in milliseconds. Columnar cold baseline is unavailable (crash); PAX cold-restart
completing all eight profiles correctly is itself the robustness result for the cold regime.
