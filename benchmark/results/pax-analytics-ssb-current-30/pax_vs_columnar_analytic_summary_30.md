# PAX vs COLUMNAR analytic benchmark summary (30 runs)

Method: Release benchmark_runner, flags: --disk --shared-load --runs=30 --pax-page-rows=256. Medians are in ms; lower is better. Point lookup probes were excluded from the PAX synthetic analytic subset: pax/fixed_analytic/q7, pax/mixed_schema/q4,q7, pax/string_heavy/q4,q7.

| Group | Valid queries | PAX wins | COLUMNAR wins | Ties | Median delta PAX vs COLUMNAR |
|---|---:|---:|---:|---:|---:|
| pax/fixed_analytic | 7 | 7 | 0 | 0 | -12.77% |
| pax/mixed_schema | 6 | 1 | 2 | 3 | +1.16% |
| pax/string_heavy | 6 | 6 | 0 | 0 | -5.57% |

## Per Query

| Query | PAX median | COLUMNAR median | Delta | Winner | Hash match |
|---|---:|---:|---:|---|---|
| pax/fixed_analytic/queries/q1 | 68.226 | 78.210 | -12.77% | pax | yes |
| pax/fixed_analytic/queries/q2 | 154.317 | 167.394 | -7.81% | pax | yes |
| pax/fixed_analytic/queries/q3 | 51.557 | 59.917 | -13.95% | pax | yes |
| pax/fixed_analytic/queries/q4 | 43.084 | 53.024 | -18.75% | pax | yes |
| pax/fixed_analytic/queries/q5 | 178.723 | 193.211 | -7.50% | pax | yes |
| pax/fixed_analytic/queries/q6 | 195.823 | 226.655 | -13.60% | pax | yes |
| pax/fixed_analytic/queries/q8 | 156.268 | 159.905 | -2.27% | pax | yes |
| pax/mixed_schema/queries/q1 | 17.505 | 17.097 | +2.39% | columnar | yes |
| pax/mixed_schema/queries/q2 | 16.316 | 16.159 | +0.97% | tie | yes |
| pax/mixed_schema/queries/q3 | 25.170 | 23.722 | +6.10% | columnar | yes |
| pax/mixed_schema/queries/q5 | 75.792 | 75.813 | -0.03% | tie | yes |
| pax/mixed_schema/queries/q6 | 69.479 | 68.560 | +1.34% | tie | yes |
| pax/mixed_schema/queries/q8 | 90.997 | 95.534 | -4.75% | pax | yes |
| pax/string_heavy/queries/q1 | 38.220 | 40.574 | -5.80% | pax | yes |
| pax/string_heavy/queries/q2 | 11.809 | 12.476 | -5.35% | pax | yes |
| pax/string_heavy/queries/q3 | 17.740 | 24.075 | -26.31% | pax | yes |
| pax/string_heavy/queries/q5 | 51.761 | 55.835 | -7.30% | pax | yes |
| pax/string_heavy/queries/q6 | 132.749 | 135.870 | -2.30% | pax | yes |
| pax/string_heavy/queries/q8 | 15.912 | 16.541 | -3.80% | pax | yes |

## Excluded / Blocked

- pax/fixed_analytic/q7 timed out on PAX even with runs=1 and external timeout 180s; the full COLUMNAR group also timed out after q6, consistent with q7 being a general point-lookup issue.
- pax/mixed_schema/q4,q7 and pax/string_heavy/q4,q7 are point-lookup/projection probes, not analytic aggregations; they were excluded from the analytic rollup.
- Official normalized ssb is not a valid latency result in this worktree: both PAX and COLUMNAR fail with missing lo_* columns because benchmark/ssb has no setup/load file. The local SSB .tbl files are owned by nobody:nogroup with 0600 permissions, and sudo -n chmod is unavailable.
