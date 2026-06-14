# PAX DML synthetic benchmark results, 30 runs

Methodology: each scenario was run on disk with `--shared-load --runs=30` for `auto`, explicit `pax`, and `columnar`. The load phase is shared and checkpointed once; timed runs contain only the DML workload plus post-run verification. Positive speedup means the tested layout was faster than `columnar`.

## Comparison

| Scenario | Operation | Runs | Auto median ms | PAX median ms | Columnar median ms | Auto speedup | PAX speedup | Verified | Auto verdict | PAX verdict |
|---|---|---:|---:|---:|---:|---:|---:|---|---|---|
| fixed_update | Update 5000 fixed-width rows and restore them | 30 | 305.578 | 336.158 | 313.144 | 2.416% | -7.349% | OK | parity/noise | parity/noise |
| fixed_delete_insert | Delete 1000 fixed-width rows and insert them back from seed | 30 | 64.299 | 64.979 | 63.410 | -1.402% | -2.474% | OK | parity/noise | parity/noise |
| string_update | Update 4000 string payload rows and restore them | 30 | 102.573 | 111.748 | 100.819 | -1.740% | -10.840% | OK | parity/noise | columnar faster |

## Per-layout Metrics

| Scenario | Layout | Runs | Min ms | Median ms | Avg ms | Stddev ms | CI95 ms | RSD % | Verified |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|
| fixed_update | auto | 30 | 244.042 | 305.578 | 323.172 | 70.498 | 25.227 | 21.814 | OK |
| fixed_update | pax | 30 | 237.763 | 336.158 | 350.514 | 72.888 | 26.083 | 20.795 | OK |
| fixed_update | columnar | 30 | 219.255 | 313.144 | 323.677 | 71.394 | 25.548 | 22.057 | OK |
| fixed_delete_insert | auto | 30 | 56.548 | 64.299 | 65.029 | 3.613 | 1.293 | 5.555 | OK |
| fixed_delete_insert | pax | 30 | 58.485 | 64.979 | 65.223 | 4.069 | 1.456 | 6.239 | OK |
| fixed_delete_insert | columnar | 30 | 55.909 | 63.410 | 63.640 | 4.357 | 1.559 | 6.847 | OK |
| string_update | auto | 30 | 86.792 | 102.573 | 102.788 | 8.306 | 2.972 | 8.081 | OK |
| string_update | pax | 30 | 94.255 | 111.748 | 111.555 | 8.956 | 3.205 | 8.028 | OK |
| string_update | columnar | 30 | 85.769 | 100.819 | 100.990 | 9.173 | 3.282 | 9.083 | OK |

Conservative verdict rule: a winner is reported only when the average-time delta is larger than the combined 95% CI and the median delta has the same sign. Otherwise the result is treated as parity/noise.

Raw per-layout CSV files are in the same directory. `pax_dml_layouts_30.csv` contains the long-form table.
