# PAX Synthetic Layout Comparison, 30 Runs

Configuration: Release `benchmark_runner`, `--disk`, `--runs=30`, fresh `--load-only` state per profile/layout, then `--skip-load`. Delta is `(auto_median / columnar_median - 1) * 100`; negative means `auto` is faster or smaller.

## Profile Rollup

| profile | query rows | auto wins | columnar wins | ties | median query delta % | checkpoint delta % | query state delta % | checkpoint state delta % |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| fixed_analytic | 8 | 2 | 5 | 1 | +2.13 | -10.87 | +0.00 | -3.73 |
| string_heavy | 8 | 3 | 4 | 1 | +1.14 | -7.16 | +0.00 | -3.56 |
| mixed_schema | 8 | 2 | 4 | 2 | +1.14 | -5.73 | +0.00 | -4.07 |
| ssb_flat | 13 | 11 | 1 | 1 | -3.98 | -5.05 | +0.00 | +0.73 |
| tpch_pax | 4 | 1 | 1 | 2 | +0.89 | -9.45 | +0.00 | -4.40 |

## Detailed Latency

| profile | kind | name | runs | auto median ms | columnar median ms | delta % | winner | row count | col count | hash match |
| --- | --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| fixed_analytic | query | pax/fixed_analytic/queries/q1 | 30 | 0.333 | 0.276 | +20.65 | columnar | 32 | 4 | yes |
| fixed_analytic | query | pax/fixed_analytic/queries/q2 | 30 | 0.199 | 0.272 | -26.84 | auto | 32 | 1 | yes |
| fixed_analytic | query | pax/fixed_analytic/queries/q3 | 30 | 0.317 | 0.307 | +3.26 | columnar | 16 | 2 | yes |
| fixed_analytic | query | pax/fixed_analytic/queries/q4 | 30 | 0.299 | 0.296 | +1.01 | columnar | 1 | 1 | yes |
| fixed_analytic | query | pax/fixed_analytic/queries/q5 | 30 | 0.343 | 0.313 | +9.58 | columnar | 5 | 2 | yes |
| fixed_analytic | query | pax/fixed_analytic/queries/q6 | 30 | 0.329 | 0.279 | +17.92 | columnar | 11 | 1 | yes |
| fixed_analytic | query | pax/fixed_analytic/queries/q7 | 30 | 0.322 | 0.348 | -7.47 | auto | 1 | 2 | yes |
| fixed_analytic | query | pax/fixed_analytic/queries/q8 | 30 | 0.354 | 0.355 | -0.28 | tie | 8 | 2 | yes |
| fixed_analytic | checkpoint | pax/fixed_analytic_checkpoint/checkpoint | 30 | 81.099 | 90.993 | -10.87 | auto | 0 | 0 | yes |
| mixed_schema | query | pax/mixed_schema/queries/q1 | 30 | 0.323 | 0.230 | +40.43 | columnar | 32 | 5 | yes |
| mixed_schema | query | pax/mixed_schema/queries/q2 | 30 | 0.313 | 0.331 | -5.44 | auto | 32 | 1 | yes |
| mixed_schema | query | pax/mixed_schema/queries/q3 | 30 | 0.344 | 0.342 | +0.58 | tie | 16 | 2 | yes |
| mixed_schema | query | pax/mixed_schema/queries/q4 | 30 | 0.346 | 0.340 | +1.76 | columnar | 8 | 1 | yes |
| mixed_schema | query | pax/mixed_schema/queries/q5 | 30 | 0.370 | 0.373 | -0.80 | tie | 13 | 2 | yes |
| mixed_schema | query | pax/mixed_schema/queries/q6 | 30 | 0.351 | 0.337 | +4.15 | columnar | 1 | 1 | yes |
| mixed_schema | query | pax/mixed_schema/queries/q7 | 30 | 0.241 | 0.327 | -26.30 | auto | 1 | 2 | yes |
| mixed_schema | query | pax/mixed_schema/queries/q8 | 30 | 0.361 | 0.355 | +1.69 | columnar | 7 | 2 | yes |
| mixed_schema | checkpoint | pax/mixed_schema_checkpoint/checkpoint | 30 | 126.119 | 133.783 | -5.73 | auto | 0 | 0 | yes |
| string_heavy | query | pax/string_heavy/queries/q1 | 30 | 0.339 | 0.327 | +3.67 | columnar | 32 | 5 | yes |
| string_heavy | query | pax/string_heavy/queries/q2 | 30 | 0.297 | 0.319 | -6.90 | auto | 32 | 1 | yes |
| string_heavy | query | pax/string_heavy/queries/q3 | 30 | 0.338 | 0.349 | -3.15 | auto | 8 | 2 | yes |
| string_heavy | query | pax/string_heavy/queries/q4 | 30 | 0.325 | 0.333 | -2.40 | auto | 4 | 1 | yes |
| string_heavy | query | pax/string_heavy/queries/q5 | 30 | 0.380 | 0.354 | +7.34 | columnar | 15 | 2 | yes |
| string_heavy | query | pax/string_heavy/queries/q6 | 30 | 0.346 | 0.344 | +0.58 | tie | 4 | 1 | yes |
| string_heavy | query | pax/string_heavy/queries/q7 | 30 | 0.300 | 0.295 | +1.69 | columnar | 1 | 1 | yes |
| string_heavy | query | pax/string_heavy/queries/q8 | 30 | 0.336 | 0.320 | +5.00 | columnar | 3 | 2 | yes |
| string_heavy | checkpoint | pax/string_heavy_checkpoint/checkpoint | 30 | 83.375 | 89.809 | -7.16 | auto | 0 | 0 | yes |
| ssb_flat | query | ssb_flat/q1-1 | 30 | 48.479 | 53.178 | -8.84 | auto | 1 | 1 | yes |
| ssb_flat | query | ssb_flat/q1-2 | 30 | 60.808 | 76.648 | -20.67 | auto | 1 | 1 | yes |
| ssb_flat | query | ssb_flat/q1-3 | 30 | 71.721 | 74.787 | -4.10 | auto | 1 | 1 | yes |
| ssb_flat | query | ssb_flat/q2-1 | 30 | 27.703 | 33.537 | -17.40 | auto | 214 | 3 | yes |
| ssb_flat | query | ssb_flat/q2-2 | 30 | 38.277 | 39.415 | -2.89 | auto | 24 | 3 | yes |
| ssb_flat | query | ssb_flat/q2-3 | 30 | 25.263 | 27.630 | -8.57 | auto | 5 | 3 | yes |
| ssb_flat | query | ssb_flat/q3-1 | 30 | 50.878 | 51.969 | -2.10 | auto | 60 | 4 | yes |
| ssb_flat | query | ssb_flat/q3-2 | 30 | 49.200 | 49.755 | -1.12 | auto | 0 | 0 | yes |
| ssb_flat | query | ssb_flat/q3-3 | 30 | 69.833 | 72.728 | -3.98 | auto | 0 | 0 | yes |
| ssb_flat | query | ssb_flat/q3-4 | 30 | 59.185 | 58.970 | +0.36 | tie | 0 | 0 | yes |
| ssb_flat | query | ssb_flat/q4-1 | 30 | 53.439 | 54.048 | -1.13 | auto | 28 | 3 | yes |
| ssb_flat | query | ssb_flat/q4-2 | 30 | 78.850 | 76.779 | +2.70 | columnar | 77 | 4 | yes |
| ssb_flat | query | ssb_flat/q4-3 | 30 | 61.819 | 65.718 | -5.93 | auto | 8 | 4 | yes |
| ssb_flat | checkpoint | ssb_flat_checkpoint/checkpoint | 30 | 91.734 | 96.610 | -5.05 | auto | 0 | 0 | yes |
| tpch_pax | query | tpch_pax/q1 | 30 | 83.467 | 82.333 | +1.38 | columnar | 4 | 10 | yes |
| tpch_pax | query | tpch_pax/q12 | 30 | 86.639 | 85.877 | +0.89 | tie | 2 | 3 | yes |
| tpch_pax | query | tpch_pax/q14 | 30 | 54.911 | 57.466 | -4.45 | auto | 1 | 1 | yes |
| tpch_pax | query | tpch_pax/q6 | 30 | 62.603 | 62.050 | +0.89 | tie | 1 | 1 | yes |
| tpch_pax | checkpoint | tpch_pax_checkpoint/checkpoint | 30 | 96.510 | 106.579 | -9.45 | auto | 0 | 0 | yes |

## State Sizes

| profile | group | phase | auto bytes | columnar bytes | delta % | winner |
| --- | --- | --- | ---: | ---: | ---: | --- |
| fixed_analytic | pax/fixed_analytic | query_loaded | 21491824 | 21491824 | +0.00 | tie |
| fixed_analytic | pax/fixed_analytic_checkpoint | checkpoint_after_run | 135262320 | 140505200 | -3.73 | auto |
| fixed_analytic | pax/fixed_analytic_checkpoint | checkpoint_loaded | 17821808 | 17821808 | +0.00 | tie |
| mixed_schema | pax/mixed_schema | query_loaded | 22016112 | 22016112 | +0.00 | tie |
| mixed_schema | pax/mixed_schema_checkpoint | checkpoint_after_run | 135786608 | 141553776 | -4.07 | auto |
| mixed_schema | pax/mixed_schema_checkpoint | checkpoint_loaded | 18346096 | 18346096 | +0.00 | tie |
| ssb_flat | ssb_flat | query_loaded | 23056496 | 23056496 | +0.00 | tie |
| ssb_flat | ssb_flat_checkpoint | checkpoint_after_run | 145215600 | 144167024 | +0.73 | tie |
| ssb_flat | ssb_flat_checkpoint | checkpoint_loaded | 19386480 | 19386480 | +0.00 | tie |
| string_heavy | pax/string_heavy | query_loaded | 21229680 | 21229680 | +0.00 | tie |
| string_heavy | pax/string_heavy_checkpoint | checkpoint_after_run | 135000176 | 139980912 | -3.56 | auto |
| string_heavy | pax/string_heavy_checkpoint | checkpoint_loaded | 17559664 | 17559664 | +0.00 | tie |
| tpch_pax | tpch_pax | query_loaded | 23875712 | 23875712 | +0.00 | tie |
| tpch_pax | tpch_pax_checkpoint | checkpoint_after_run | 153899136 | 160977024 | -4.40 | auto |
| tpch_pax | tpch_pax_checkpoint | checkpoint_loaded | 19681408 | 19681408 | +0.00 | tie |

Notes: `fixed_analytic`, `string_heavy`, and `mixed_schema` query profiles contain only 32 rows, so sub-millisecond query latency should be interpreted as smoke/microbenchmark behavior. `ssb_flat` and `tpch_pax` are larger generated analytical profiles and are more informative for latency trends.

Fresh-load storage probe for `pax/fixed_analytic` after a single setup checkpoint currently shows no size difference between `auto` and `columnar`: both state roots are `21491824` bytes, both have `14` `table.otbx` files, and total `table.otbx` apparent size is `20357120` bytes. This means older fixed-profile storage-size claims should not be reused without rerunning the measurement.
