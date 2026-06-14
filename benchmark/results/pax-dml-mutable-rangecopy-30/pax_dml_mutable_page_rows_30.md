# Mutable PAX DML page-size matrix (30 runs)

Command shape: `benchmark_runner --disk --layout=pax --pax-mutable-dml-unsafe --shared-load`.

| scenario | page_rows | median_ms | avg_ms | fast update/delete/insert | fallback update/delete/insert | PAX fixed/generic | regular | counters |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| fixed_delete_insert | 128 | 44.438 | 44.949 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_delete_insert | 256 | 44.629 | 46.082 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_delete_insert | 512 | 45.313 | 46.826 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_delete_insert | 1024 | 46.090 | 48.887 | 0/30/30 | 0/0/0 | 1800/477 | 0 | OK |
| fixed_update | 128 | 233.525 | 235.774 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| fixed_update | 256 | 237.999 | 241.915 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| fixed_update | 512 | 248.425 | 255.926 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| fixed_update | 1024 | 237.659 | 237.624 | 60/0/0 | 0/0/0 | 1800/0 | 0 | OK |
| string_update | 128 | 102.574 | 105.088 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
| string_update | 256 | 101.433 | 103.408 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
| string_update | 512 | 93.653 | 96.676 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
| string_update | 1024 | 98.923 | 105.311 | 60/0/0 | 0/0/0 | 0/1200 | 0 | OK |
