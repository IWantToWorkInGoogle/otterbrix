# Mutable PAX DML page-size matrix (30 runs)

Command shape: `benchmark_runner --disk --layout=pax --pax-mutable-dml-unsafe --shared-load`.

| scenario | page_rows | median_ms | avg_ms | fast update/delete/insert | fallback update/delete/insert | PAX fixed/generic | regular | counters |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| fixed_delete_insert | 128 | 44.123 | 44.260 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_update | 128 | 54.920 | 56.639 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| string_update | 128 | 53.219 | 49.294 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
