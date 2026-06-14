# Mutable PAX DML page-size matrix (30 runs)

Command shape: `benchmark_runner --disk --layout=pax --pax-mutable-dml-unsafe --shared-load`.

| scenario | page_rows | median_ms | avg_ms | fast update/delete/insert | fallback update/delete/insert | PAX fixed/generic | regular | counters |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| fixed_delete_insert | 128 | 44.037 | 48.424 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_delete_insert | 256 | 54.277 | 51.509 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_delete_insert | 512 | 44.677 | 45.036 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_delete_insert | 1024 | 45.326 | 45.028 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_update | 128 | 330.059 | 331.006 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| fixed_update | 256 | 331.032 | 335.265 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| fixed_update | 512 | 351.122 | 352.190 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| fixed_update | 1024 | 356.688 | 364.064 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| string_update | 128 | 152.017 | 151.868 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
| string_update | 256 | 152.352 | 153.253 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
| string_update | 512 | 157.682 | 162.106 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
| string_update | 1024 | 155.992 | 160.900 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
