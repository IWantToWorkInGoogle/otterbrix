#!/usr/bin/env bash

set -uo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_BUILD_DIR="${ROOT_DIR}/build"
if [[ -d "${ROOT_DIR}/build-pax-coverage" ]]; then
    DEFAULT_BUILD_DIR="${ROOT_DIR}/build-pax-coverage"
fi

BUILD_DIR="${BUILD_DIR:-${DEFAULT_BUILD_DIR}}"
REPORT_FILE="${REPORT_FILE:-${ROOT_DIR}/pax_coverage_quality_report.md}"
ARTIFACT_DIR="${ARTIFACT_DIR:-${ROOT_DIR}/pax_coverage_quality_artifacts}"

RUN_BUILD=1
RUN_TESTS=1
RUN_COVERAGE=1
RESET_COUNTERS=1
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"
THRESHOLD_PERCENT="${THRESHOLD_PERCENT:-80}"

PAX_TEST_REGEX="${PAX_TEST_REGEX:-pax|columnar-only|layout round trip|layout defaults|projected scan}"
PAX_SCOPE_REGEX="${PAX_SCOPE_REGEX:-pax|PAX|layout_kind|row_group_layout|layout_policy|ROW_GROUP_LAYOUTS_MAGIC}"
PAX_BUILD_TARGETS="${PAX_BUILD_TARGETS:-test_table}"

usage() {
    cat <<'USAGE'
Usage:
  ./coverage_quality_report.sh [options]

Options:
  --build-dir DIR        CMake coverage build directory. Default: ./build-pax-coverage if it exists, else ./build
  --report FILE          Markdown report path. Default: ./pax_coverage_quality_report.md
  --artifacts DIR        Directory for logs/intermediate files. Default: ./pax_coverage_quality_artifacts
  --threshold N          Required coverage threshold in percent. Default: 80
  --test-regex REGEX     CTest regex for PAX-targeted tests. Default: pax|columnar-only|layout round trip|layout defaults|projected scan
  --scope-regex REGEX    Regex that marks PAX-specific lines/functions inside mixed source files
  --build-targets LIST   Space-separated CMake targets to build. Default: test_table
  --jobs N               Parallel jobs for build/ctest. Default: nproc
  --no-build             Do not run cmake --build
  --no-run-tests         Do not run CTest; use existing coverage counters
  --no-reset-counters    Do not remove stale .gcda counters before running tests
  --no-coverage          Skip gcov probing and render only test/scenario metrics
  -h, --help             Show this help

What this script measures:
  - PAX-targeted CTest pass rate
  - PAX scenario coverage proxy by registered test names
  - gcov line/function/branch coverage for the PAX implementation area

Notes:
  The default build target is test_table because it contains the focused PAX
  metadata/checkpoint/load/projected-scan tests. Use PAX_BUILD_TARGETS or
  --build-targets to add integration targets when that build is linkable.
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir)
            [[ $# -ge 2 ]] || { echo "Missing value for --build-dir" >&2; exit 2; }
            BUILD_DIR="$2"
            shift 2
            ;;
        --report)
            [[ $# -ge 2 ]] || { echo "Missing value for --report" >&2; exit 2; }
            REPORT_FILE="$2"
            shift 2
            ;;
        --artifacts)
            [[ $# -ge 2 ]] || { echo "Missing value for --artifacts" >&2; exit 2; }
            ARTIFACT_DIR="$2"
            shift 2
            ;;
        --threshold)
            [[ $# -ge 2 ]] || { echo "Missing value for --threshold" >&2; exit 2; }
            THRESHOLD_PERCENT="$2"
            shift 2
            ;;
        --test-regex)
            [[ $# -ge 2 ]] || { echo "Missing value for --test-regex" >&2; exit 2; }
            PAX_TEST_REGEX="$2"
            shift 2
            ;;
        --scope-regex)
            [[ $# -ge 2 ]] || { echo "Missing value for --scope-regex" >&2; exit 2; }
            PAX_SCOPE_REGEX="$2"
            shift 2
            ;;
        --build-targets)
            [[ $# -ge 2 ]] || { echo "Missing value for --build-targets" >&2; exit 2; }
            PAX_BUILD_TARGETS="$2"
            shift 2
            ;;
        --jobs)
            [[ $# -ge 2 ]] || { echo "Missing value for --jobs" >&2; exit 2; }
            JOBS="$2"
            shift 2
            ;;
        --no-build)
            RUN_BUILD=0
            shift
            ;;
        --no-run-tests)
            RUN_TESTS=0
            shift
            ;;
        --no-reset-counters)
            RESET_COUNTERS=0
            shift
            ;;
        --no-coverage)
            RUN_COVERAGE=0
            shift
            ;;
        --run-tests)
            RUN_TESTS=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

if [[ "$BUILD_DIR" != /* ]]; then
    BUILD_DIR="${ROOT_DIR}/${BUILD_DIR}"
fi

if [[ "$REPORT_FILE" != /* ]]; then
    REPORT_FILE="${ROOT_DIR}/${REPORT_FILE}"
fi

if [[ "$ARTIFACT_DIR" != /* ]]; then
    ARTIFACT_DIR="${ROOT_DIR}/${ARTIFACT_DIR}"
fi

mkdir -p "$ARTIFACT_DIR"

BUILD_LOG="${ARTIFACT_DIR}/build.log"
CTEST_INVENTORY="${ARTIFACT_DIR}/ctest_inventory.txt"
CTEST_TESTS="${ARTIFACT_DIR}/pax_ctest_tests.txt"
CTEST_RUN_LOG="${ARTIFACT_DIR}/ctest_run.log"
GCOV_DIR="${ARTIFACT_DIR}/gcov"
GCOV_LOG="${ARTIFACT_DIR}/gcov.log"
PAX_TARGETS_FILE="${ARTIFACT_DIR}/pax_source_files.txt"
PAX_SCENARIO_TABLE="${ARTIFACT_DIR}/pax_scenario_table.md"
COVERAGE_ENV="${ARTIFACT_DIR}/coverage_metrics.env"
MEASURED_FILES_TABLE="${ARTIFACT_DIR}/measured_files.md"

have() {
    command -v "$1" >/dev/null 2>&1
}

percent() {
    local numerator="$1"
    local denominator="$2"
    awk -v n="$numerator" -v d="$denominator" 'BEGIN {
        if (d == 0) {
            print "n/a";
        } else {
            printf "%.1f%%", (100.0 * n / d);
        }
    }'
}

plain_percent() {
    local numerator="$1"
    local denominator="$2"
    awk -v n="$numerator" -v d="$denominator" 'BEGIN {
        if (d == 0) {
            print "n/a";
        } else {
            printf "%.1f", (100.0 * n / d);
        }
    }'
}

status_for_percent() {
    local value="$1"
    if [[ "$value" == "n/a" ]]; then
        printf 'not measured'
        return
    fi
    awk -v v="$value" -v t="$THRESHOLD_PERCENT" 'BEGIN {
        if (v >= t) print "OK";
        else print "below threshold";
    }'
}

metric_row() {
    local label="$1"
    local value="$2"
    local detail="$3"
    local status
    status="$(status_for_percent "$value")"
    if [[ "$value" == "n/a" ]]; then
        printf '| %s | n/a | >=%s%% | %s |\n' "$label" "$THRESHOLD_PERCENT" "$status"
    else
        printf '| %s | %.1f%% %s | >=%s%% | %s |\n' "$label" "$value" "$detail" "$THRESHOLD_PERCENT" "$status"
    fi
}

tool_status() {
    local tool="$1"
    if have "$tool"; then
        printf '`%s`' "$(command -v "$tool")"
    else
        printf 'not found'
    fi
}

pax_source_files=(
    components/table/storage/data_pointer.cpp
    components/table/storage/data_pointer.hpp
    components/table/row_group.cpp
    components/table/row_group.hpp
    components/table/data_table.cpp
    components/table/storage/block_manager.hpp
    components/configuration/configuration.hpp
    services/disk/manager_disk.cpp
)

pax_gcov_entrypoints=(
    components/table/storage/data_pointer.cpp
    components/table/row_group.cpp
    components/table/data_table.cpp
    services/disk/manager_disk.cpp
)

write_pax_target_file_list() {
    : > "$PAX_TARGETS_FILE"
    for rel in "${pax_source_files[@]}"; do
        if [[ -f "${ROOT_DIR}/${rel}" ]]; then
            printf '%s\n' "${ROOT_DIR}/${rel}" >> "$PAX_TARGETS_FILE"
        fi
    done
}

build_targets() {
    : > "$BUILD_LOG"
    [[ "$RUN_BUILD" -eq 1 ]] || {
        echo "build skipped" > "$BUILD_LOG"
        return
    }
    if [[ ! -d "$BUILD_DIR" ]]; then
        echo "Build directory does not exist: $BUILD_DIR" > "$BUILD_LOG"
        return
    fi
    if ! have cmake; then
        echo "cmake not found" > "$BUILD_LOG"
        return
    fi

    read -r -a targets <<< "$PAX_BUILD_TARGETS"
    if [[ "${#targets[@]}" -eq 0 ]]; then
        echo "No build targets configured" > "$BUILD_LOG"
        return
    fi

    cmake --build "$BUILD_DIR" --target "${targets[@]}" -j "$JOBS" > "$BUILD_LOG" 2>&1 || true
}

collect_ctest_inventory() {
    : > "$CTEST_INVENTORY"
    : > "$CTEST_TESTS"
    if [[ ! -d "$BUILD_DIR" ]] || ! have ctest; then
        return
    fi

    ctest -N --test-dir "$BUILD_DIR" -R "$PAX_TEST_REGEX" > "$CTEST_INVENTORY" 2>&1 || true
    sed -nE 's/^ *Test *#[0-9]+: //p' "$CTEST_INVENTORY" > "$CTEST_TESTS"
}

reset_coverage_counters() {
    [[ "$RESET_COUNTERS" -eq 1 ]] || return
    [[ "$RUN_TESTS" -eq 1 ]] || return
    [[ -d "$BUILD_DIR" ]] || return
    find "$BUILD_DIR" -type f -name '*.gcda' -delete 2>/dev/null || true
}

run_pax_tests() {
    : > "$CTEST_RUN_LOG"
    [[ "$RUN_TESTS" -eq 1 ]] || {
        echo "ctest skipped" > "$CTEST_RUN_LOG"
        return
    }
    if [[ ! -d "$BUILD_DIR" ]]; then
        echo "Build directory does not exist: $BUILD_DIR" > "$CTEST_RUN_LOG"
        return
    fi
    if ! have ctest; then
        echo "ctest not found" > "$CTEST_RUN_LOG"
        return
    fi

    ctest --test-dir "$BUILD_DIR" \
        -R "$PAX_TEST_REGEX" \
        --output-on-failure \
        -j "$JOBS" > "$CTEST_RUN_LOG" 2>&1 || true
}

generate_gcov_reports() {
    : > "$GCOV_LOG"
    mkdir -p "$GCOV_DIR"
    find "$GCOV_DIR" -type f -name '*.gcov' -delete 2>/dev/null || true

    [[ "$RUN_COVERAGE" -eq 1 ]] || {
        echo "coverage skipped" > "$GCOV_LOG"
        return
    }
    if ! have gcov; then
        echo "gcov not found" > "$GCOV_LOG"
        return
    fi
    if [[ ! -d "$BUILD_DIR" ]]; then
        echo "Build directory does not exist: $BUILD_DIR" > "$GCOV_LOG"
        return
    fi

    local rel base found_any
    found_any=0
    for rel in "${pax_gcov_entrypoints[@]}"; do
        base="$(basename "$rel")"
        while IFS= read -r data_file; do
            [[ -n "$data_file" ]] || continue
            found_any=1
            (
                cd "$GCOV_DIR" || exit 1
                gcov -b -c -p "$data_file" >> "$GCOV_LOG" 2>&1
            ) || true
        done < <(find "$BUILD_DIR" -type f -name "${base}.gcda" 2>/dev/null | sort)
    done

    if [[ "$found_any" -eq 0 ]]; then
        echo "No .gcda files found for PAX entrypoint sources. Run tests from a --coverage build first." >> "$GCOV_LOG"
    fi
}

parse_gcov_reports() {
    cat > "$COVERAGE_ENV" <<'EOF'
file_line_covered=0
file_line_total=0
file_function_covered=0
file_function_total=0
file_branch_covered=0
file_branch_total=0
file_branch_no_throw_covered=0
file_branch_no_throw_total=0
scope_line_covered=0
scope_line_total=0
scope_function_covered=0
scope_function_total=0
scope_branch_covered=0
scope_branch_total=0
scope_branch_no_throw_covered=0
scope_branch_no_throw_total=0
measured_file_count=0
EOF

    : > "$MEASURED_FILES_TABLE"
    {
        echo "| Measured source file |"
        echo "|---|"
    } >> "$MEASURED_FILES_TABLE"

    [[ "$RUN_COVERAGE" -eq 1 ]] || return
    compgen -G "${GCOV_DIR}/*.gcov" >/dev/null || return

    awk -v targets="$PAX_TARGETS_FILE" \
        -v scope_re="$PAX_SCOPE_REGEX" \
        -v measured_table="$MEASURED_FILES_TABLE" '
        function trim(s) {
            gsub(/^[[:space:]]+|[[:space:]]+$/, "", s);
            return s;
        }
        function executable_marker(marker) {
            return marker != "-";
        }
        function covered_marker(marker) {
            return marker !~ /^[#=]+$/ && marker !~ /^0+\*?$/;
        }
        function covered_branch(line) {
            return line !~ /never executed/ && line !~ /taken 0/;
        }
        BEGIN {
            FS = ":";
            while ((getline t < targets) > 0) {
                target[t] = 1;
            }
        }
        FNR == 1 {
            include = 0;
            source = "";
            current_function_scope = 0;
            last_line_scope = 0;
        }
        /^[[:space:]]*-:[[:space:]]*0:Source:/ {
            source = $0;
            sub(/^.*Source:/, "", source);
            include = (source in target);
            current_function_scope = 0;
            last_line_scope = 0;
            if (include && !(source in measured)) {
                measured[source] = 1;
                measured_count++;
                rel = source;
                sub(/^.*\/otterbrix\//, "", rel);
                printf "| `%s` |\n", rel >> measured_table;
            }
            next;
        }
        include && /^function / {
            file_function_total++;
            if ($0 !~ /called 0/) {
                file_function_covered++;
            }
            current_function_scope = ($0 ~ scope_re);
            if (current_function_scope) {
                scope_function_total++;
                if ($0 !~ /called 0/) {
                    scope_function_covered++;
                }
            }
            last_line_scope = current_function_scope;
            next;
        }
        include && /^branch / {
            file_branch_total++;
            if (covered_branch($0)) {
                file_branch_covered++;
            }
            if ($0 !~ /\(throw\)/) {
                file_branch_no_throw_total++;
                if (covered_branch($0)) {
                    file_branch_no_throw_covered++;
                }
            }
            if (current_function_scope || last_line_scope) {
                scope_branch_total++;
                if (covered_branch($0)) {
                    scope_branch_covered++;
                }
                if ($0 !~ /\(throw\)/) {
                    scope_branch_no_throw_total++;
                    if (covered_branch($0)) {
                        scope_branch_no_throw_covered++;
                    }
                }
            }
            next;
        }
        include && /^[[:space:]]*[-#=0-9]+[*]?:[[:space:]]*[0-9]+:/ {
            marker = trim($1);
            code = $0;
            sub(/^[^:]*:[^:]*:/, "", code);
            line_scope = (current_function_scope || code ~ scope_re);
            last_line_scope = line_scope;
            if (executable_marker(marker)) {
                file_line_total++;
                if (covered_marker(marker)) {
                    file_line_covered++;
                }
                if (line_scope) {
                    scope_line_total++;
                    if (covered_marker(marker)) {
                        scope_line_covered++;
                    }
                }
            }
        }
        END {
            printf "file_line_covered=%d\n", file_line_covered;
            printf "file_line_total=%d\n", file_line_total;
            printf "file_function_covered=%d\n", file_function_covered;
            printf "file_function_total=%d\n", file_function_total;
            printf "file_branch_covered=%d\n", file_branch_covered;
            printf "file_branch_total=%d\n", file_branch_total;
            printf "file_branch_no_throw_covered=%d\n", file_branch_no_throw_covered;
            printf "file_branch_no_throw_total=%d\n", file_branch_no_throw_total;
            printf "scope_line_covered=%d\n", scope_line_covered;
            printf "scope_line_total=%d\n", scope_line_total;
            printf "scope_function_covered=%d\n", scope_function_covered;
            printf "scope_function_total=%d\n", scope_function_total;
            printf "scope_branch_covered=%d\n", scope_branch_covered;
            printf "scope_branch_total=%d\n", scope_branch_total;
            printf "scope_branch_no_throw_covered=%d\n", scope_branch_no_throw_covered;
            printf "scope_branch_no_throw_total=%d\n", scope_branch_no_throw_total;
            printf "measured_file_count=%d\n", measured_count;
        }
    ' "$GCOV_DIR"/*.gcov > "$COVERAGE_ENV"
}

make_scenario_table() {
    : > "$PAX_SCENARIO_TABLE"
    {
        echo "| PAX scenario class | Registered target tests |"
        echo "|---|---:|"
    } >> "$PAX_SCENARIO_TABLE"

    local covered=0
    local total=0
    local label pattern count
    while IFS='|' read -r label pattern; do
        [[ -n "$label" ]] || continue
        total=$((total + 1))
        count=0
        if [[ -s "$CTEST_TESTS" ]]; then
            count="$(grep -Eic "$pattern" "$CTEST_TESTS" || true)"
        fi
        [[ "$count" -gt 0 ]] && covered=$((covered + 1))
        printf '| %s | %s |\n' "$label" "$count" >> "$PAX_SCENARIO_TABLE"
    done <<'SCENARIOS'
PAX fixed metadata/versioning|pax_fixed|pax fixed v[0-9]|fixed .*v[0-9]
PAX generic metadata/versioning|pax_generic|pax_generic v[0-9]|pax_generic v2|pax_generic layout|pax_generic
PAX fixed write/load|fixed integer columns are written as pax|fixed-width scalar roots
PAX generic string write/load|string columns are written as pax generic|pax generic string
PAX nested generic values|struct column|nested struct|union column|list column|array of struct
PAX projected fast scan|projected scan uses fast path
PAX filters and null predicates|filters|null predicates|null validity
PAX fallback/unsupported routing|falls back|columnar-only|unsupported roots
PAX overflow/page boundaries|overflow strings|across pages
SCENARIOS

    scenario_covered="$covered"
    scenario_total="$total"
}

ctest_registered_count() {
    if [[ ! -s "$CTEST_TESTS" ]]; then
        echo 0
        return
    fi
    wc -l < "$CTEST_TESTS" | tr -d ' '
}

ctest_pass_counts() {
    if [[ ! -s "$CTEST_RUN_LOG" ]] || [[ "$RUN_TESTS" -ne 1 ]]; then
        echo "0 0"
        return
    fi

    local parsed failed total
    parsed="$(sed -nE 's/.* ([0-9]+) tests failed out of ([0-9]+).*/\1 \2/p' "$CTEST_RUN_LOG" | tail -n 1)"
    if [[ -z "$parsed" ]]; then
        echo "0 0"
        return
    fi
    failed="${parsed%% *}"
    total="${parsed##* }"
    echo "$((total - failed)) $total"
}

render_report() {
    # shellcheck disable=SC1090
    source "$COVERAGE_ENV"

    local registered_tests pass_counts passed_tests total_run_tests
    registered_tests="$(ctest_registered_count)"
    pass_counts="$(ctest_pass_counts)"
    passed_tests="${pass_counts%% *}"
    total_run_tests="${pass_counts##* }"

    local test_pass_pct scenario_pct
    test_pass_pct="$(plain_percent "$passed_tests" "$total_run_tests")"
    scenario_pct="$(plain_percent "$scenario_covered" "$scenario_total")"

    local scope_line_pct scope_function_pct scope_branch_pct scope_branch_no_throw_pct
    local file_line_pct file_function_pct file_branch_pct file_branch_no_throw_pct
    scope_line_pct="$(plain_percent "$scope_line_covered" "$scope_line_total")"
    scope_function_pct="$(plain_percent "$scope_function_covered" "$scope_function_total")"
    scope_branch_pct="$(plain_percent "$scope_branch_covered" "$scope_branch_total")"
    scope_branch_no_throw_pct="$(plain_percent "$scope_branch_no_throw_covered" "$scope_branch_no_throw_total")"
    file_line_pct="$(plain_percent "$file_line_covered" "$file_line_total")"
    file_function_pct="$(plain_percent "$file_function_covered" "$file_function_total")"
    file_branch_pct="$(plain_percent "$file_branch_covered" "$file_branch_total")"
    file_branch_no_throw_pct="$(plain_percent "$file_branch_no_throw_covered" "$file_branch_no_throw_total")"

    cat > "$REPORT_FILE" <<REPORT
# PAX Test Coverage Quality Report

Generated: $(date '+%Y-%m-%d %H:%M:%S %Z')

Project root: \`$ROOT_DIR\`  
Build directory: \`$BUILD_DIR\`  
Artifacts: \`$ARTIFACT_DIR\`  
CTest regex: \`$PAX_TEST_REGEX\`  
PAX scope regex: \`$PAX_SCOPE_REGEX\`  
Required threshold: \`>=${THRESHOLD_PERCENT}%\`

## Coverage Quality Summary

| Coverage evaluation metric | Result | Required | Status |
|---|---:|---:|---|
$(metric_row "PAX-scope line coverage" "$scope_line_pct" "(${scope_line_covered}/${scope_line_total})")
$(metric_row "PAX-scope function coverage" "$scope_function_pct" "(${scope_function_covered}/${scope_function_total})")
$(metric_row "PAX-scope branch coverage" "$scope_branch_pct" "(${scope_branch_covered}/${scope_branch_total})")
$(metric_row "PAX-scope branch coverage without gcov throw edges" "$scope_branch_no_throw_pct" "(${scope_branch_no_throw_covered}/${scope_branch_no_throw_total})")
$(metric_row "PAX implementation file line coverage" "$file_line_pct" "(${file_line_covered}/${file_line_total})")
$(metric_row "PAX implementation file function coverage" "$file_function_pct" "(${file_function_covered}/${file_function_total})")
$(metric_row "PAX implementation file branch coverage" "$file_branch_pct" "(${file_branch_covered}/${file_branch_total})")
$(metric_row "PAX implementation file branch coverage without gcov throw edges" "$file_branch_no_throw_pct" "(${file_branch_no_throw_covered}/${file_branch_no_throw_total})")
$(metric_row "PAX-targeted test pass rate" "$test_pass_pct" "(${passed_tests}/${total_run_tests})")
$(metric_row "PAX scenario coverage proxy" "$scenario_pct" "(${scenario_covered}/${scenario_total})")

The raw gcov branch rows include compiler-generated exception edges marked as \`(throw)\`.
The “without gcov throw edges” rows keep ordinary conditional branches and switch outcomes, but exclude those exception edges.

## PAX Test Inventory

| Metric | Value |
|---|---:|
| Registered PAX-targeted CTest cases | $registered_tests |
| Run PAX-targeted CTest cases | $total_run_tests |
| Passed PAX-targeted CTest cases | $passed_tests |
| Measured PAX source files | $measured_file_count |

## Measured Source Files

$(cat "$MEASURED_FILES_TABLE")

## PAX Scenario Coverage Proxy

This is not line coverage. It checks whether the registered PAX-targeted test suite names cover the main PAX format risk classes.

$(cat "$PAX_SCENARIO_TABLE")

## Tool Readiness

| Tool | Status |
|---|---|
| cmake | $(tool_status cmake) |
| ctest | $(tool_status ctest) |
| gcov | $(tool_status gcov) |
| rg | $(tool_status rg) |

## Artifact Files

| Artifact | Path |
|---|---|
| Build log | \`$BUILD_LOG\` |
| CTest inventory | \`$CTEST_INVENTORY\` |
| CTest run log | \`$CTEST_RUN_LOG\` |
| gcov log | \`$GCOV_LOG\` |
| gcov files | \`$GCOV_DIR\` |
REPORT
}

main() {
    write_pax_target_file_list
    build_targets
    collect_ctest_inventory
    reset_coverage_counters
    run_pax_tests
    collect_ctest_inventory
    generate_gcov_reports
    parse_gcov_reports
    make_scenario_table
    render_report

    echo "Report written to: $REPORT_FILE"
    echo "Artifacts written to: $ARTIFACT_DIR"
}

main "$@"
