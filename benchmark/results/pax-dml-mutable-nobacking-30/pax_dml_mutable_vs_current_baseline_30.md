# Mutable PAX DML vs current baseline, 30 runs

Mutable PAX uses `--layout=pax --pax-mutable-dml-unsafe --shared-load`; baselines were rerun in the same result directory after the no-backing-tail change. Speedup is `(baseline_median / mutable_median - 1) * 100`.

| scenario | mutable best page_rows | mutable median ms | PAX baseline ms | columnar baseline ms | vs PAX | vs columnar | mutable counters | regular scan |
|---|---:|---:|---:|---:|---:|---:|---|---:|
| fixed_update | 128 | 197.006 | 305.608 | 310.446 | +55.13% | +57.58% | u/d/i=60/0/0, fb=0/0/0 | 0 |
| fixed_delete_insert | 128 | 36.921 | 41.583 | 41.568 | +12.63% | +12.59% | u/d/i=0/30/30, fb=0/0/0 | 0 |
| string_update | 256 | 82.167 | 104.603 | 103.946 | +27.31% | +26.51% | u/d/i=60/0/0, fb=0/0/0 | 0 |
