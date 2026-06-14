# Mutable PAX DML vs baseline, 30 runs

Mutable PAX uses `--layout=pax --pax-mutable-dml-unsafe --shared-load`; baseline PAX and columnar use `--shared-load` without the unsafe flag. Speedup is `(baseline_median / mutable_median - 1) * 100`.

| scenario | mutable best page_rows | mutable median ms | PAX baseline ms | columnar baseline ms | vs PAX | vs columnar | mutable counters | regular scan |
|---|---:|---:|---:|---:|---:|---:|---|---:|
| fixed_update | 128 | 196.015 | 295.383 | 294.527 | +50.69% | +50.26% | u/d/i=60/0/0, fb=0/0/0 | 0 |
| fixed_delete_insert | 128 | 36.465 | 41.312 | 49.137 | +13.29% | +34.75% | u/d/i=0/30/30, fb=0/0/0 | 0 |
| string_update | 128 | 79.113 | 98.576 | 107.538 | +24.60% | +35.93% | u/d/i=60/0/0, fb=0/0/0 | 0 |
