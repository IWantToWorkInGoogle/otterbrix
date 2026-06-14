# Mutable PAX DML vs source-column baseline, 30 runs

Mutable PAX uses `--layout=pax --pax-mutable-dml-unsafe --shared-load` and the best median over `pax_page_rows=128/256/512/1024`. Baselines were rerun in the same result directory without unsafe mode at `pax_page_rows=256`. Speedup is `(baseline_median / mutable_median - 1) * 100`.

| scenario | mutable best page_rows | mutable median ms | PAX baseline ms | COLUMNAR baseline ms | vs PAX | vs COLUMNAR | fast u/d/i | fallback u/d/i | regular scan | counters |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| fixed_delete_insert | 256 | 42.381 | 48.032 | 47.620 | 13.33% | 12.36% | 0/30/30 | 0/0/0 | 0 | OK |
| fixed_update | 256 | 52.769 | 285.617 | 289.197 | 441.26% | 448.04% | 60/0/0 | 0/0/0 | 0 | OK |
| string_update | 512 | 40.645 | 149.404 | 153.205 | 267.58% | 276.93% | 60/0/0 | 0/0/0 | 0 | OK |
