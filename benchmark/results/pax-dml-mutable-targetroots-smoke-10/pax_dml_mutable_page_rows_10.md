# Mutable PAX DML page-size matrix (10 runs)

Command shape: `benchmark_runner --disk --layout=pax --pax-mutable-dml-unsafe --shared-load`.

| scenario | page_rows | median_ms | avg_ms | fast update/delete/insert | fallback update/delete/insert | PAX fixed/generic | regular | counters |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| fixed_delete_insert | 256 | 42.868 | 43.518 | 0/10/10 | 0/0/0 | 600/65 | 0 | OK |
| fixed_update | 256 | 55.772 | 57.457 | 20/0/0 | 0/0/0 | 600/0 | 0 | OK |
| string_update | 256 | 41.065 | 41.054 | 20/0/0 | 0/0/0 | 0/400 | 0 | OK |
