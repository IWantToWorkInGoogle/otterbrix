PAX benchmark suite for `benchmark_runner`.

This suite is intended to compare the DISK layout policies on deterministic
synthetic tables:

- `pax/fixed_analytic`: wide fixed-width analytical table, 150000 rows
- `pax/mixed_schema`: fixed-width columns plus string payloads, 100000 rows
- `pax/string_heavy`: string-dominant schema, 80000 rows
- `pax/dml/fixed_update`: update and restore a hot fixed-width range
- `pax/dml/fixed_delete_insert`: delete a fixed-width range and insert it back
- `pax/dml/string_update`: update and restore string payload columns
- `*_checkpoint`: dirty checkpoint fixtures; not used by the current query
  latency summary

Generate fixtures before running the suite:

```bash
python3 benchmark/pax/generate_data.py
```

Recommended query-latency command shape:

```bash
build-release/benchmark/runner/benchmark_runner \
  --disk --layout=auto --shared-load --runs=30 \
  --group='^pax/fixed_analytic$' \
  --benchmarks=/home/usrname/otterbrix/benchmark \
  --out=benchmark/results/pax-reworked-30/fixed_analytic_auto_30.csv
```

Run one profile per process. `--shared-load` loads the selected group once,
performs one checkpoint after load, then runs one warmup and 30 timed query
executions. This avoids measuring CSV import and DDL time in every query.
The benchmark runner supports `--layout=auto`, `--layout=pax`, and
`--layout=columnar`.

Do not use a single multi-profile `--shared-load` command for this suite yet:
after a large profile load, creating the next profile in the same process can
hang in the current runner/storage path.

`--skip-load` (cold restart in a fresh process) works for `--layout=pax` and
`--layout=auto`. It crashes for `--layout=columnar`: this is a pre-existing
cascade of bugs in the shared columnar restore / buffer-manager / aggregate
paths, not in PAX. See `COLD_RESTART_NOTES.md` for the root cause. For cold-cache
latency measurement use `run_cold.sh` (PAX/auto only), which evicts the page
cache (`evict_cache.py`) and runs one fresh process per sample.

## Query Scenarios

Each `queries.sql` has eight statements:

1. full aggregate
2. selective/range aggregate
3. filtered group by
4. equality filter
5. range aggregate
6. selective filter using a non-projected predicate column
7. point projection
8. range group/aggregate

Indexes are intentionally not created in these profiles. The goal is to compare
the persisted scan/layout behavior, not index build or lookup behavior.

## DML Scenarios

The DML profiles exercise the mutation path for persisted layouts:

1. `pax/dml/fixed_update`: two `UPDATE` statements flip a 5000-row fixed-width
   range and restore it in the same timed run.
2. `pax/dml/fixed_delete_insert`: deletes 1000 fixed-width rows and inserts the
   same range back from a seed table.
3. `pax/dml/string_update`: updates 4000 string rows and restores the string
   predicate column.

These interpreted benchmarks use a separate `verify` SQL block. Timed `run`
statements mutate state, while `verify` checks a post-run invariant such as
"the restored range has no dirty flag rows" or "the deleted range exists again".

Recommended DML command shape:

```bash
build-release/benchmark/runner/benchmark_runner \
  --disk --layout=pax --shared-load --runs=30 \
  --group='^pax/dml/fixed_update$' \
  --benchmarks=/home/usrname/otterbrix/benchmark \
  --out=benchmark/results/pax-dml-30/fixed_update_pax_30.csv
```

Run one DML group and one layout per process. Parallel runner processes can
conflict on the shared temporary disk benchmark state directory. `--shared-load`
checkpoints the loaded base table before timed mutations; a plain run measures
mutation over the uncheckpointed append state instead of immutable persisted PAX
pages.

DML always runs through the transactional MVCC path (`UPDATE = delete + full-row
append`, deletes via the version manager). The historical benchmark-only
in-place "mutable PAX" DML path and its `pax_mutable_*` CSV counters have been
removed; only the scan-path counters remain in the CSV:

- `pax_scan_regular` should be `0` when the DML predicate/projection scan stays
  on the PAX fast path.
- `pax_scan_fixed_projected` or `pax_scan_generic_projected` should be non-zero,
  depending on the table layout.

## Current 30-run Results

Full results:

- `benchmark/results/pax-reworked-30/pax_reworked_summary_30.md`
- `benchmark/results/pax-reworked-30/pax_reworked_summary_30.csv`
- raw per-layout CSV files in `benchmark/results/pax-reworked-30/`
- DML verification/performance results:
  - `benchmark/results/pax-dml-30/pax_dml_summary_30.md`
  - `benchmark/results/pax-dml-30/pax_dml_summary_30.csv`
  - `benchmark/results/pax-dml-30/pax_dml_layouts_30.csv`
- Benchmark-only mutable PAX DML results:
  - `benchmark/results/pax-dml-mutable-30/pax_dml_mutable_page_rows_30.md`
  - `benchmark/results/pax-dml-mutable-30/pax_dml_mutable_page_rows_30.csv`
  - `benchmark/results/pax-dml-mutable-30/pax_dml_mutable_vs_baseline_30.md`
  - `benchmark/results/pax-dml-mutable-30/pax_dml_mutable_vs_baseline_30.csv`
- Benchmark-only mutable PAX DML results after removing DML-time columnar tail
  backing:
  - `benchmark/results/pax-dml-mutable-nobacking-30/pax_dml_mutable_page_rows_30.md`
  - `benchmark/results/pax-dml-mutable-nobacking-30/pax_dml_mutable_page_rows_30.csv`
  - `benchmark/results/pax-dml-mutable-nobacking-30/pax_dml_mutable_vs_current_baseline_30.md`
  - `benchmark/results/pax-dml-mutable-nobacking-30/pax_dml_mutable_vs_current_baseline_30.csv`
- Benchmark-only mutable PAX DML results after limiting UPDATE output to
  source/target columns:
  - `benchmark/results/pax-dml-mutable-sourcecols-allpages-30/pax_dml_mutable_page_rows_30.md`
  - `benchmark/results/pax-dml-mutable-sourcecols-allpages-30/pax_dml_mutable_page_rows_30.csv`
  - `benchmark/results/pax-dml-mutable-sourcecols-allpages-30/pax_dml_mutable_vs_sourcecols_baseline_30.md`
  - `benchmark/results/pax-dml-mutable-sourcecols-allpages-30/pax_dml_mutable_vs_sourcecols_baseline_30.csv`

Conservative PAX wins from the current run:

- `fixed_analytic q1`: 38.920 ms vs 41.200 ms median, +5.53%
- `fixed_analytic q2`: 111.723 ms vs 114.648 ms median, +2.55%
- `fixed_analytic q4`: 74.618 ms vs 76.685 ms median, +2.70%
- `string_heavy q3`: 38.400 ms vs 38.705 ms median, +0.79%

The fixed-width analytical profile is the clearest positive case for PAX/auto.
Mixed and string-heavy profiles are mostly parity/noise or columnar-favored in
the current implementation.

Storage-size snapshot after `--load-only` does not show a size delta between
`auto` and `columnar` for these reworked profiles. Treat these benchmarks as
query-latency demonstrations, not storage-compression evidence.

The historical DML summary above was collected before the benchmark-only
mutable PAX path was enabled. Treat it as coverage of the regular production
DML path, not as the current result for the immutable-base + mutable-delta
experiment.

Current mutable PAX DML run after source/target-only UPDATE output, 30 runs,
best `pax_page_rows` per scenario:

- `fixed_update`: 52.769 ms median at 256 rows/page, +441.26% vs regular PAX
  baseline and +448.04% vs columnar baseline.
- `fixed_delete_insert`: 42.381 ms median at 256 rows/page, +13.33% vs
  regular PAX baseline and +12.36% vs columnar baseline.
- `string_update`: 40.645 ms median at 512 rows/page, +267.58% vs regular PAX
  baseline and +276.93% vs columnar baseline.

For all mutable PAX DML page-size runs, expected fast-path counters matched,
fallback counters were `0`, and `pax_scan_regular` was `0`.
