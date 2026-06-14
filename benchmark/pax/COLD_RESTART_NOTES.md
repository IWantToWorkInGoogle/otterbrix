# Cold-restart / `--skip-load` scan path notes

This documents the state of the persisted ("cold") scan path for the PAX benchmark suite, as
investigated while optimizing the PAX scan. "Cold restart" means: load + checkpoint in one process,
then reopen the persisted state in a *fresh* process (`benchmark_runner --skip-load`) and scan.

## Summary

- **PAX (`--layout=pax` / `--layout=auto`) cold restart works.** A fresh process reads the persisted
  PAX layout from disk and scans all eight query profiles correctly.
- **Columnar (`--layout=columnar`) cold restart crashes.** This is the "persisted scan crash" the
  README previously referred to. It is *not* in PAX code; it is a cascade of pre-existing bugs in the
  shared columnar restore / buffer-manager / aggregate paths.

For true cold-cache I/O measurement use `run_cold.sh` (fresh process per sample + `evict_cache.py`
page-cache eviction). It runs the PAX/auto layouts only.

## Columnar cold-restart bug cascade

Reproduce (debug build gives the asserts/line numbers):

```bash
build/benchmark/runner/benchmark_runner --disk --layout=columnar --load-only \
  --group='^pax/fixed_analytic$' --benchmarks=$PWD/benchmark
build/benchmark/runner/benchmark_runner --disk --layout=columnar --skip-load --runs=1 \
  --group='^pax/fixed_analytic$' --benchmarks=$PWD/benchmark
```

### Bug #1 — in-memory block evicted then re-pinned (buffer manager)
- Crash: `standard_buffer_manager_t::pin` at `standard_buffer_manager.cpp:233`,
  `handle->get_buffer(lock)->allocation_size()` on a null buffer.
  Stack: `column_segment_t::initialize_scan` (`column_segment.cpp:990`) → `pin`.
- Root cause: cold restore creates many transient validity segments
  (`column_data_t::initialize_column_validity` → `apend_transient_segment` →
  `register_transient_memory`). These are in-memory blocks (`block_id >= MAXIMUM_BLOCK`,
  `destroy_buffer_condition::BLOCK`). `block_handle_t::can_unload()` does not exclude them, so the
  buffer pool can unload one. `block_handle_t::load()` returns an empty handle for
  `block_id >= MAXIMUM_BLOCK` (no disk backing), and `pin` then dereferences the null buffer.

### Bug #2 — aggregate key path out of range (cold-restore chunk shape)
- Once bug #1 no longer aborts the scan, the next failure is
  `operator_func.cpp:71`: `key.path().front() < chunk.data.size()` — the aggregate references an
  absolute column index larger than the number of columns in the scan output chunk. Warm columnar is
  fine, so cold restore produces a differently shaped / column-reduced chunk. In Release the assert
  is compiled out and the function falls into its graceful-error branch (returns `false`).

### Bug #3 — null cursor on the error path (dispatcher)
- In Release, bug #2's graceful error then SIGSEGVs in
  `cursor_t::is_success()` ← `manager_dispatcher_t::execute_plan_impl`: the failed plan's result
  cursor is dereferenced without a null check.

## Why a naive fix for #1 was NOT landed

A one-line guard in `can_unload()` (return `false` for in-memory `destroy_buffer_condition::BLOCK`
blocks, mirroring upstream DuckDB `BlockHandle::CanUnload`) does stop the bug #1 SIGSEGV. **However,
it regressed the PAX cold path**: with transient blocks no longer evictable, the PAX cold scan began
hitting bug #2 (aggregate column-count mismatch) that it previously avoided, i.e. the guard changes
buffer-pool behavior globally in a way that surfaces the deeper bug #2 on the (previously working)
PAX path. Because the goal was a working, measurable PAX cold path, the guard was reverted.

A correct fix needs to address the cascade together (transient-validity-segment lifecycle on restore,
the cold-restore chunk column mapping for aggregates, and the dispatcher null-cursor guard), which is
a columnar-restore work item beyond the PAX scan scope. Tracked here for follow-up.
