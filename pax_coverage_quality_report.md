# PAX Test Coverage Quality Report

Generated: 2026-06-13 18:28:57 MSK

Project root: `/home/usrname/otterbrix`  
Build directory: `/home/usrname/otterbrix/build-pax-coverage`  
Artifacts: `/home/usrname/otterbrix/pax_coverage_quality_artifacts`  
CTest regex: `components::table|single_file_block_manager|metadata|checkpoint_load|transaction_manager|statistics|zonemap|parallel scan|services::disk|test_recovery`  
PAX scope regex: `pax|PAX|layout_kind|row_group_layout|layout_policy|ROW_GROUP_LAYOUTS_MAGIC`  
Required threshold: `>=80%`

## Coverage Quality Summary

| Coverage evaluation metric | Result | Required | Status |
|---|---:|---:|---|
| PAX-scope line coverage | 85,0% (2852/3356) | >=80% | OK |
| PAX-scope function coverage | 96,2% (151/157) | >=80% | OK |
| PAX-scope branch coverage | 58,4% (2117/3622) | >=80% | below threshold |
| PAX-scope branch coverage without gcov throw edges | 70,4% (2113/3000) | >=80% | below threshold |
| PAX implementation file line coverage | 83,0% (4309/5191) | >=80% | OK |
| PAX implementation file function coverage | 92,3% (288/312) | >=80% | OK |
| PAX implementation file branch coverage | 56,6% (3039/5371) | >=80% | below threshold |
| PAX implementation file branch coverage without gcov throw edges | 68,8% (3032/4406) | >=80% | below threshold |
| PAX-targeted test pass rate | 100,0% (261/261) | >=80% | below threshold |
| PAX scenario coverage proxy | 100,0% (9/9) | >=80% | below threshold |

The raw gcov branch rows include compiler-generated exception edges marked as `(throw)`.
The “without gcov throw edges” rows keep ordinary conditional branches and switch outcomes, but exclude those exception edges.

## PAX Test Inventory

| Metric | Value |
|---|---:|
| Registered PAX-targeted CTest cases | 261 |
| Run PAX-targeted CTest cases | 261 |
| Passed PAX-targeted CTest cases | 261 |
| Measured PAX source files | 6 |

## Measured Source Files

| Measured source file |
|---|
| `components/table/data_table.cpp` |
| `components/table/row_group.cpp` |
| `components/table/row_group.hpp` |
| `components/table/storage/block_manager.hpp` |
| `components/table/storage/data_pointer.cpp` |
| `services/disk/manager_disk.cpp` |

## PAX Scenario Coverage Proxy

This is not line coverage. It checks whether the registered PAX-targeted test suite names cover the main PAX format risk classes.

| PAX scenario class | Registered target tests |
|---|---:|
| PAX fixed metadata/versioning | 5 |
| PAX generic metadata/versioning | 2 |
| PAX fixed write/load | 2 |
| PAX generic string write/load | 3 |
| PAX nested generic values | 6 |
| PAX projected fast scan | 2 |
| PAX filters and null predicates | 9 |
| PAX fallback/unsupported routing | 4 |
| PAX overflow/page boundaries | 5 |

## Tool Readiness

| Tool | Status |
|---|---|
| cmake | `/usr/local/bin/cmake` |
| ctest | `/usr/local/bin/ctest` |
| gcov | `/usr/bin/gcov` |
| rg | not found |

## Artifact Files

| Artifact | Path |
|---|---|
| Build log | `/home/usrname/otterbrix/pax_coverage_quality_artifacts/build.log` |
| CTest inventory | `/home/usrname/otterbrix/pax_coverage_quality_artifacts/ctest_inventory.txt` |
| CTest run log | `/home/usrname/otterbrix/pax_coverage_quality_artifacts/ctest_run.log` |
| gcov log | `/home/usrname/otterbrix/pax_coverage_quality_artifacts/gcov.log` |
| gcov files | `/home/usrname/otterbrix/pax_coverage_quality_artifacts/gcov` |
