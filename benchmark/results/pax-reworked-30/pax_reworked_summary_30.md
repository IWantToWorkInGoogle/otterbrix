# benchmark/pax reworked 30-run summary

Method: `benchmark_runner --disk --shared-load --runs=30`, one profile per process, Release build. `auto` is the PAX/auto layout policy; `columnar` is `--layout=columnar`. Each profile loads CSV once, runner performs one checkpoint after load, then runs one warmup and 30 timed query executions.

The `effect` column is conservative: it reports a winner only when the median direction matches the mean difference and the mean difference is larger than the combined 95% CI. Otherwise it is marked as parity/noise or unstable/outlier.

## Query Latency

| profile | query | scenario | auto median ms | columnar median ms | median speedup | auto CI95 | columnar CI95 | effect | verified |
|---|---:|---|---:|---:|---:|---:|---:|---|---|
| fixed_analytic | q1 | full aggregate over revenue/cost/count | 38.920 | 41.200 | +5.53% | 0.498 | 0.506 | PAX(auto) faster | OK |
| fixed_analytic | q2 | date-range revenue projection | 111.723 | 114.648 | +2.55% | 1.148 | 1.413 | PAX(auto) faster | OK |
| fixed_analytic | q3 | segment filter + region group by | 100.030 | 100.867 | +0.83% | 6.324 | 0.655 | unstable/outlier | OK |
| fixed_analytic | q4 | metric equality aggregate | 74.618 | 76.685 | +2.70% | 1.715 | 5.255 | PAX(auto) faster | OK |
| fixed_analytic | q5 | metric range revenue/cost aggregate | 133.052 | 133.581 | +0.40% | 1.704 | 1.479 | parity/noise | OK |
| fixed_analytic | q6 | non-projected filler predicate + discount | 143.226 | 142.517 | -0.50% | 3.106 | 1.095 | parity/noise | OK |
| fixed_analytic | q7 | id point projection | 87.127 | 90.006 | +3.20% | 1.975 | 1.735 | parity/noise | OK |
| fixed_analytic | q8 | id range aggregate | 109.171 | 108.873 | -0.27% | 1.383 | 1.462 | parity/noise | OK |
| mixed_schema | q1 | full aggregate over amount/count | 8.803 | 8.521 | -3.31% | 0.397 | 0.489 | parity/noise | OK |
| mixed_schema | q2 | flag filter amount sum | 36.793 | 36.358 | -1.20% | 1.426 | 0.455 | parity/noise | OK |
| mixed_schema | q3 | category filter + region group by | 55.528 | 54.454 | -1.97% | 0.632 | 0.588 | parity/noise | OK |
| mixed_schema | q4 | amount equality payload projection | 25.635 | 24.334 | -5.35% | 1.274 | 0.759 | parity/noise | OK |
| mixed_schema | q5 | amount range aggregate + avg score | 57.386 | 58.402 | +1.74% | 1.534 | 0.902 | parity/noise | OK |
| mixed_schema | q6 | filler + flag selective count | 50.228 | 49.264 | -1.96% | 0.566 | 0.285 | columnar faster | OK |
| mixed_schema | q7 | id point mixed projection | 12.899 | 12.271 | -5.12% | 0.290 | 0.223 | columnar faster | OK |
| mixed_schema | q8 | id range category group by | 67.132 | 67.779 | +0.95% | 0.410 | 1.202 | parity/noise | OK |
| string_heavy | q1 | count all rows | 18.585 | 16.945 | -9.68% | 1.199 | 0.552 | columnar faster | OK |
| string_heavy | q2 | region equality count | 34.351 | 34.168 | -0.54% | 2.899 | 0.317 | columnar faster | OK |
| string_heavy | q3 | region filter + city group by | 38.400 | 38.705 | +0.79% | 0.433 | 0.361 | PAX(auto) faster | OK |
| string_heavy | q4 | visits point string projection | 3.725 | 3.746 | +0.56% | 0.151 | 0.147 | parity/noise | OK |
| string_heavy | q5 | visits range count/sum | 39.934 | 38.618 | -3.41% | 2.596 | 0.349 | columnar faster | OK |
| string_heavy | q6 | city + visits range ordered notes | 91.666 | 92.931 | +1.36% | 0.853 | 1.299 | parity/noise | OK |
| string_heavy | q7 | visits point name projection | 3.775 | 3.854 | +2.05% | 0.372 | 0.156 | unstable/outlier | OK |
| string_heavy | q8 | tag filter + region group by | 37.383 | 36.702 | -1.86% | 0.398 | 2.446 | parity/noise | OK |

## Storage Snapshot

Sizes are apparent bytes after `--load-only`; `main_table_bytes` is the largest `table.otbx` under the benchmark state root. The current reworked profiles do not show a storage-size delta between auto and columnar.

| profile | layout | state_root_bytes | main_table_bytes |
|---|---|---:|---:|
| fixed_analytic | auto | 36950128 | 21770240 |
| fixed_analytic | columnar | 36950128 | 21770240 |
| mixed_schema | auto | 276287600 | 261107712 |
| mixed_schema | columnar | 276287600 | 261107712 |
| string_heavy | auto | 83611760 | 68431872 |
| string_heavy | columnar | 83611760 | 68431872 |

## PAX Win Examples

- `fixed_analytic q1`: full aggregate over revenue/cost/count, median 38.920 ms vs 41.200 ms (+5.53%).
- `fixed_analytic q2`: date-range revenue projection, median 111.723 ms vs 114.648 ms (+2.55%).
- `fixed_analytic q4`: metric equality aggregate, median 74.618 ms vs 76.685 ms (+2.70%).
- `string_heavy q3`: region filter + city group by, median 38.400 ms vs 38.705 ms (+0.79%).
