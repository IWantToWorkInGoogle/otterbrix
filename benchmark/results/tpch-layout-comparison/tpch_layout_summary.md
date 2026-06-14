# TPC-H Layout Comparison

Configuration: `benchmark_runner --disk --skip-load --group=^tpch$ --runs=3`, Release build, warm cache. Both layouts were loaded with fresh `--load-only` state before the comparison. AUTO is the current auto-select policy; COLUMNAR is forced `--layout=columnar`. Positive delta means AUTO median is slower than COLUMNAR.

Raw CSV files: `tpch_auto.csv`, `tpch_columnar.csv`, `tpch_columnar_unsupported_status.csv`.

Fresh load-only state sizes measured after the final reload: AUTO `77172904` bytes, COLUMNAR `77172904` bytes. This TPC-H fixture therefore does not currently demonstrate a storage-size advantage for AUTO/PAX.

Long supported queries `q7`, `q8`, `q9`, and `q13` were not included in the paired comparison because the single-run COLUMNAR log already shows 114-383 seconds per timed run, and the runner performs warmup plus timed runs.

## Runnable Demonstration Set

| name | runs | auto_median_ms | columnar_median_ms | auto_vs_columnar_median_pct | winner_by_median | auto_min_ms | columnar_min_ms | auto_avg_ms | columnar_avg_ms | row_count | column_count | result_hash_match |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| tpch/q1 | 3 | 250.894 | 244.884 | +2.45 | columnar | 193.074 | 197.497 | 233.160 | 231.789 | 4 | 10 | yes |
| tpch/q3 | 3 | 6158.665 | 7486.538 | -17.74 | auto | 5055.691 | 4946.890 | 6115.234 | 6924.431 | 138 | 4 | yes |
| tpch/q5 | 3 | 7486.620 | 7679.087 | -2.51 | auto | 5769.374 | 5829.140 | 7164.903 | 7200.695 | 5 | 2 | yes |
| tpch/q6 | 3 | 104.988 | 109.116 | -3.78 | auto | 97.879 | 104.393 | 102.730 | 107.758 | 1 | 1 | yes |
| tpch/q10 | 3 | 19998.876 | 19441.774 | +2.87 | columnar | 17001.398 | 16717.220 | 19907.909 | 19069.910 | 399 | 8 | yes |
| tpch/q12 | 3 | 4311.563 | 4545.397 | -5.14 | auto | 4154.938 | 4495.214 | 4422.339 | 4581.950 | 2 | 3 | yes |
| tpch/q14 | 3 | 3489.480 | 2532.323 | +37.80 | columnar | 2471.716 | 2258.449 | 3487.737 | 3126.473 | 1 | 1 | yes |
| tpch/q19 | 3 | 4436.819 | 4457.854 | -0.47 | auto | 3287.374 | 3475.682 | 4436.467 | 4538.729 | 1 | 1 | yes |

## Unsupported TPC-H Status Checks

These were run as one COLUMNAR process after the transformer fixes to verify FAIL rows do not crash the runner.

| name | status | error |
| --- | --- | --- |
| tpch/q2 | FAIL | SQL error: code=29 what=Unsupported expression |
| tpch/q4 | FAIL | SQL error: code=29 what=Unsupported expression: unknown expr type in transform_a_expr |
| tpch/q11 | FAIL | SQL error: code=29 what=Unsupported subquery in HAVING operand: scalar subquery |
| tpch/q15 | FAIL | SQL error: code=29 what=Unsupported expression while executing: select  s_suppkey,  s_name,  s_address,  s_phone,  total_revenue from  tpch.supplier,  revenue0 where  s_suppkey = supplier_no  and total_revenue = (   select    max(total_revenue)   from    revenue0  ) order by  s_suppk... |
| tpch/q16 | FAIL | SQL error: code=29 what=Unsupported expr type in transform_a_expr |
| tpch/q17 | FAIL | SQL error: code=29 what=Unsupported expression |
| tpch/q18 | FAIL | SQL error: code=29 what=Unsupported expression: unknown expr type in transform_a_expr |
| tpch/q20 | FAIL | SQL error: code=29 what=Unsupported expression: unknown expr type in transform_a_expr |
| tpch/q21 | FAIL | SQL error: code=29 what=Unsupported expression: unknown expr type in transform_a_expr |
| tpch/q22 | FAIL | SQL error: code=29 what=IN expression: left side must be a column reference |

## PAX Hypotheses

| hypothesis | reason |
| --- | --- |
| Use fixed-width analytical subsets | PAX_FIXED is the strongest current path; TPC-H mixed string/numeric tables dilute the effect with columnar fallback. |
| Measure cold restart and checkpoint/write-back | Existing PAX suite suggests checkpoint/storage effects are more promising than warm read latency. |
| Instrument scan-path counters per query/table | Show whether AUTO actually reads PAX pages, prunes pages, or falls back to regular columnar scans. |
| Prefer narrow projections with non-projected predicates | PAX should benefit when tuple-local pages avoid loading unrelated payload while still using nearby predicate columns. |
| Split TPC-H tables by schema family for explicit USING PAX experiments | Numeric/date-only tables or derived materialized subsets can force PAX_FIXED without mixed root columns. |
