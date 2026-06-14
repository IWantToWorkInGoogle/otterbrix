# Mutable PAX DML page-size matrix (30 runs)

Command shape: `benchmark_runner --disk --layout=pax --pax-mutable-dml-unsafe --shared-load`.

| scenario | page_rows | median_ms | avg_ms | fast update/delete/insert | fallback update/delete/insert | PAX fixed/generic | regular | counters |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| fixed_delete_insert | 128 | 36.465 | 37.028 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_delete_insert | 256 | 36.923 | 37.083 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_delete_insert | 512 | 38.234 | 38.302 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_delete_insert | 1024 | 38.395 | 38.562 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_update | 128 | 196.015 | 198.209 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| fixed_update | 256 | 204.241 | 205.655 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| fixed_update | 512 | 213.947 | 218.045 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| fixed_update | 1024 | 209.407 | 212.789 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| string_update | 128 | 79.113 | 79.263 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
| string_update | 256 | 81.657 | 82.494 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
| string_update | 512 | 80.433 | 79.748 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
| string_update | 1024 | 80.920 | 83.249 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
