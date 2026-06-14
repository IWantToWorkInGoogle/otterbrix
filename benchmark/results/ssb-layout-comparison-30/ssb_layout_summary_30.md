# SSB Layout Comparison, 30 Runs

Configuration: `benchmark_runner --disk --skip-load --group=^ssb$ --runs=30`, Release build, warm cache. `auto` and `columnar` were loaded with fresh `--load-only` state before measurement. Delta is `(auto_median / columnar_median - 1) * 100`; negative means `auto` is faster.

Persisted benchmark state after the runs: AUTO `297586832` bytes, COLUMNAR `369152144` bytes, AUTO vs COLUMNAR `-19.39%`.

| name | runs | auto_median_ms | columnar_median_ms | auto_vs_columnar_median_pct | winner_by_median | auto_min_ms | columnar_min_ms | auto_avg_ms | columnar_avg_ms | row_count | column_count | result_hash_match |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ssb/checkpoint | 30 | 201.518 | 246.888 | -18.38 | auto | 148.977 | 193.607 | 201.299 | 245.935 | 0 | 0 | yes |
| ssb/q1-1 | 30 | 47.247 | 47.772 | -1.10 | auto | 34.634 | 34.800 | 50.713 | 49.251 | 1 | 0 | yes |
| ssb/q1-2 | 30 | 44.255 | 44.377 | -0.27 | tie | 34.077 | 34.988 | 43.659 | 45.077 | 1 | 0 | yes |
| ssb/q1-3 | 30 | 46.652 | 44.209 | +5.53 | columnar | 36.726 | 33.285 | 46.211 | 43.474 | 1 | 0 | yes |
| ssb/q2-1 | 30 | 119.842 | 114.343 | +4.81 | columnar | 73.014 | 76.615 | 118.002 | 109.456 | 6 | 3 | yes |
| ssb/q2-2 | 30 | 109.861 | 113.503 | -3.21 | auto | 87.964 | 91.721 | 110.078 | 116.491 | 0 | 0 | yes |
| ssb/q2-3 | 30 | 122.073 | 123.509 | -1.16 | auto | 86.205 | 100.462 | 124.354 | 125.977 | 0 | 0 | yes |
| ssb/q3-1 | 30 | 105.345 | 112.327 | -6.22 | auto | 87.369 | 88.389 | 105.750 | 110.450 | 7 | 4 | yes |
| ssb/q3-2 | 30 | 105.889 | 117.694 | -10.03 | auto | 83.475 | 93.296 | 106.360 | 126.749 | 0 | 0 | yes |
| ssb/q3-3 | 30 | 99.452 | 116.984 | -14.99 | auto | 83.791 | 98.929 | 101.582 | 117.193 | 0 | 0 | yes |
| ssb/q3-4 | 30 | 116.107 | 118.148 | -1.73 | auto | 88.585 | 93.028 | 114.941 | 119.393 | 0 | 0 | yes |
| ssb/q4-1 | 30 | 163.005 | 180.029 | -9.46 | auto | 125.129 | 121.117 | 165.321 | 176.542 | 3 | 3 | yes |
| ssb/q4-2 | 30 | 171.987 | 158.917 | +8.22 | columnar | 134.818 | 119.543 | 173.101 | 161.498 | 0 | 0 | yes |
| ssb/q4-3 | 30 | 162.543 | 163.038 | -0.30 | tie | 120.990 | 111.866 | 164.656 | 161.008 | 0 | 0 | yes |

Winner count by median: AUTO `9`, COLUMNAR `3`, tie `2`.
