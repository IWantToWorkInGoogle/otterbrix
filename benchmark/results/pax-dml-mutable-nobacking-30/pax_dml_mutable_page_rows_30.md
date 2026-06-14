# Mutable PAX DML page-size matrix (30 runs)

Command shape: `benchmark_runner --disk --layout=pax --pax-mutable-dml-unsafe --shared-load`.

| scenario | page_rows | median_ms | avg_ms | fast update/delete/insert | fallback update/delete/insert | PAX fixed/generic | regular | counters |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| fixed_delete_insert | 128 | 36.921 | 37.380 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_delete_insert | 256 | 37.920 | 38.322 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_delete_insert | 512 | 38.631 | 38.728 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_delete_insert | 1024 | 38.281 | 38.780 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_update | 128 | 197.006 | 198.586 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| fixed_update | 256 | 212.977 | 211.513 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| fixed_update | 512 | 222.788 | 225.883 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| fixed_update | 1024 | 214.381 | 217.315 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| string_update | 128 | 82.703 | 82.193 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
| string_update | 256 | 82.167 | 82.819 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
| string_update | 512 | 82.839 | 83.319 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
| string_update | 1024 | 84.812 | 84.493 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
