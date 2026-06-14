# Mutable PAX DML page-size matrix (30 runs)

Command shape: `benchmark_runner --disk --layout=pax --pax-mutable-dml-unsafe --shared-load`.

| scenario | page_rows | median_ms | avg_ms | fast update/delete/insert | fallback update/delete/insert | PAX fixed/generic | regular | counters |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| fixed_delete_insert | 128 | 42.960 | 44.042 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_delete_insert | 256 | 42.381 | 43.839 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_delete_insert | 512 | 44.559 | 45.019 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_delete_insert | 1024 | 45.683 | 48.160 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_update | 128 | 55.294 | 60.170 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| fixed_update | 256 | 52.769 | 53.630 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| fixed_update | 512 | 54.168 | 55.182 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| fixed_update | 1024 | 57.446 | 60.792 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| string_update | 128 | 40.664 | 42.406 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
| string_update | 256 | 41.531 | 41.770 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
| string_update | 512 | 40.645 | 42.038 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
| string_update | 1024 | 42.178 | 42.210 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
