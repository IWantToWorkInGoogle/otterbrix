#include "benchmark_runner.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <stdexcept>
#include <cstdlib>

#include <components/configuration/configuration.hpp>
#include <integration/cpp/base_spaces.hpp>

#include "interpreted_benchmark.hpp"
#include "sql_benchmark.hpp"

namespace otterbrix::benchmark {

namespace {

const char* layout_name(benchmark_configuration_t::disk_layout_policy layout) {
    return layout == benchmark_configuration_t::disk_layout_policy::columnar_only ? "columnar" : "auto";
}

std::filesystem::path benchmark_state_root(const benchmark_configuration_t& config) {
    auto root = std::filesystem::temp_directory_path() / "otterbrix-benchmark-runner";
    root /= config.disk_on ? "disk" : "memory";
    root /= config.layout_policy == benchmark_configuration_t::disk_layout_policy::columnar_only ? "columnar"
                                                                                                  : "auto";
    root /= config.wal_on ? "wal_on" : "wal_off";
    return root;
}

void recreate_benchmark_state_root(const benchmark_configuration_t& config) {
    auto root = benchmark_state_root(config);
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    if (ec) {
        throw std::runtime_error("Cannot clear benchmark state directory " + root.string() + ": " + ec.message());
    }
    std::filesystem::create_directories(root, ec);
    if (ec) {
        throw std::runtime_error("Cannot create benchmark state directory " + root.string() + ": " + ec.message());
    }
}

void ensure_benchmark_state_root_exists(const benchmark_configuration_t& config) {
    auto root = benchmark_state_root(config);
    if (!std::filesystem::exists(root)) {
        throw std::runtime_error("Benchmark state directory not found: " + root.string() +
                                 ". Run once without --skip-load or use --load-only first.");
    }
}

bool env_enabled(const char* name) {
    const char* value = std::getenv(name);
    return value && value[0] != '\0' && value[0] != '0';
}

bool trace_enabled() { return env_enabled("OTTERBRIX_EXEC_TRACE_NODES"); }

bool sanitizer_enabled() {
#if defined(OTTERBRIX_ASAN_ENABLED) || defined(OTTERBRIX_UBSAN_ENABLED) || defined(OTTERBRIX_TSAN_ENABLED) ||          \
    defined(__SANITIZE_ADDRESS__)
    return true;
#else
    return false;
#endif
}

bool assertions_enabled() {
#ifdef NDEBUG
    return false;
#else
    return true;
#endif
}

void clear_run_state(benchmark_state_t& state) {
    state.failed = false;
    state.error.clear();
    state.result_metadata_valid = false;
    state.row_count = 0;
    state.column_count = 0;
    state.result_hash.clear();
}

std::string csv_escape(const std::string& value) {
    std::string result = "\"";
    for (char ch : value) {
        if (ch == '"') {
            result += "\"\"";
        } else if (ch == '\n' || ch == '\r') {
            result += ' ';
        } else {
            result += ch;
        }
    }
    result += "\"";
    return result;
}

const char* build_type() {
#ifdef OTTERBRIX_BENCHMARK_BUILD_TYPE
    return OTTERBRIX_BENCHMARK_BUILD_TYPE;
#else
    return "unknown";
#endif
}

void print_benchmark_methodology_warnings(const benchmark_configuration_t& config) {
    if (trace_enabled()) {
        std::cerr << "WARNING: OTTERBRIX_EXEC_TRACE_NODES is enabled; latency results include diagnostic overhead.\n";
    }
    if (sanitizer_enabled()) {
        std::cerr << "WARNING: benchmark runner was built with sanitizers; latency results are not comparable.\n";
    }
    if (assertions_enabled()) {
        std::cerr << "WARNING: assertions are enabled; use a Release/RelWithDebInfo non-sanitized build for latency "
                     "comparison.\n";
    }
    std::string build{build_type()};
    if (!build.empty() && build != "Release" && build != "RelWithDebInfo") {
        std::cerr << "WARNING: build type is '" << build
                  << "'; use Release/RelWithDebInfo for latency comparison.\n";
    }
    if (config.group_pattern == "ssb" && config.name_pattern.empty() && config.config_file.empty()) {
        std::cerr << "WARNING: --group=ssb without a benchmark name filter/config includes diagnostic SQL files. "
                     "Use pattern 'ssb/q[1-4]-' for the official 13-query SSB set.\n";
    }
    if (config.disk_on && !config.skip_load && !config.load_only) {
        std::cerr << "WARNING: disk latency run without --skip-load includes load/setup work. Prefer --load-only first, "
                     "then --skip-load for query latency.\n";
    }
}

class benchmark_instance_t final : public base_otterbrix_t {
public:
    explicit benchmark_instance_t(const benchmark_configuration_t& config)
        : base_otterbrix_t(make_config(config)) {}

private:
    static configuration::config make_config(const benchmark_configuration_t& config) {
        auto root = benchmark_state_root(config);
        auto cfg = configuration::config::create_config(root);
        cfg.main_path = root;
        cfg.log.path = root / "log";
        cfg.disk.path = root / "disk";
        cfg.wal.path = root / "wal";
        cfg.log.level = log_t::level::off;
        cfg.disk.on = config.disk_on;
        cfg.disk.layout_policy =
            config.layout_policy == benchmark_configuration_t::disk_layout_policy::columnar_only
                ? configuration::disk_layout_policy::columnar_only
                : configuration::disk_layout_policy::auto_select;
        cfg.wal.on = config.wal_on;
        return cfg;
    }
};

} // namespace

void benchmark_runner_t::register_benchmark(std::unique_ptr<benchmark_t> bench) {
    benchmarks_.push_back(std::move(bench));
}

void benchmark_runner_t::load_benchmarks_from_directory(const std::filesystem::path& dir) {
    if (!std::filesystem::exists(dir)) {
        std::cerr << "Benchmark directory not found: " << dir.string() << "\n";
        return;
    }

    std::vector<std::filesystem::path> paths;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension();
            if (ext == ".benchmark" || ext == ".sql") {
                // Skip _setup.sql files — they are loaded by sql_benchmark_t, not run as benchmarks
                if (entry.path().filename() == "_setup.sql") {
                    continue;
                }
                paths.push_back(entry.path());
            }
        }
    }
    std::sort(paths.begin(), paths.end());

    for (const auto& path : paths) {
        try {
            if (path.extension() == ".sql") {
                load_sql_file(path, dir);
            } else {
                benchmarks_.push_back(std::make_unique<interpreted_benchmark_t>(path));
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading " << path.string() << ": " << e.what() << "\n";
        }
    }
}

void benchmark_runner_t::load_single_benchmark(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        std::cerr << "Benchmark file not found: " << path.string() << "\n";
        return;
    }
    try {
        if (path.extension() == ".sql") {
            load_sql_file(path, path.parent_path());
        } else {
            benchmarks_.push_back(std::make_unique<interpreted_benchmark_t>(path));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading " << path.string() << ": " << e.what() << "\n";
    }
}

void benchmark_runner_t::load_sql_file(const std::filesystem::path& path,
                                       const std::filesystem::path& base_dir) {
    auto benchmarks = sql_benchmark_t::load_from_file(path, base_dir);
    for (auto& b : benchmarks) {
        benchmarks_.push_back(std::move(b));
    }
}

void benchmark_runner_t::generate_config_file(const std::filesystem::path& path) const {
    std::ofstream out(path);
    if (!out.is_open()) {
        std::cerr << "Error: cannot open " << path.string() << " for writing\n";
        return;
    }

    out << "# Generated benchmark configuration\n";
    out << "# Comment lines with # to disable benchmarks\n";

    // Group benchmarks by group()
    std::map<std::string, std::vector<std::string>> groups;
    for (const auto& b : benchmarks_) {
        groups[b->group()].push_back(b->name());
    }

    for (const auto& [group, names] : groups) {
        out << "\n# === " << group << " === (" << names.size() << " benchmarks)\n";
        for (const auto& name : names) {
            out << name << "\n";
        }
    }

    std::cout << "Generated config: " << path.string() << " (" << benchmarks_.size() << " benchmarks)\n";
}

void benchmark_runner_t::apply_config_file(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "Error: cannot open config file " << path.string() << "\n";
        return;
    }

    std::set<std::string> enabled;
    std::string line;
    while (std::getline(in, line)) {
        // Trim leading/trailing whitespace
        auto start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        auto end = line.find_last_not_of(" \t");
        line = line.substr(start, end - start + 1);

        if (line.empty() || line[0] == '#') continue;
        enabled.insert(line);
    }

    auto total = benchmarks_.size();
    benchmarks_.erase(
        std::remove_if(benchmarks_.begin(), benchmarks_.end(),
                       [&enabled](const std::unique_ptr<benchmark_t>& b) {
                           return enabled.find(b->name()) == enabled.end();
                       }),
        benchmarks_.end());

    auto kept = benchmarks_.size();
    std::cout << "Config: " << kept << " enabled, " << (total - kept) << " disabled out of " << total << " total\n";
}

bool benchmark_runner_t::matches_filter(const benchmark_t& bench, const benchmark_configuration_t& config) {
    // Check group filter
    if (!config.group_pattern.empty()) {
        try {
            std::regex pattern(config.group_pattern);
            if (!std::regex_search(bench.group(), pattern)) return false;
        } catch (const std::regex_error&) {
            if (bench.group().find(config.group_pattern) == std::string::npos) return false;
        }
    }

    // Check name filter
    if (!config.name_pattern.empty()) {
        try {
            std::regex pattern(config.name_pattern);
            if (!std::regex_search(bench.name(), pattern)) return false;
        } catch (const std::regex_error&) {
            if (bench.name().find(config.name_pattern) == std::string::npos) return false;
        }
    }

    return true;
}

void benchmark_runner_t::run(const benchmark_configuration_t& config) {
    // List groups mode
    if (config.list_groups) {
        std::map<std::string, size_t> groups;
        for (auto& b : benchmarks_) {
            groups[b->group()]++;
        }
        for (const auto& [group, count] : groups) {
            std::cout << std::left << std::setw(30) << group << count << " benchmarks\n";
        }
        std::cout << "\nTotal: " << benchmarks_.size() << " benchmarks in " << groups.size() << " groups\n";
        return;
    }

    std::vector<benchmark_t*> filtered;
    for (auto& b : benchmarks_) {
        if (matches_filter(*b, config)) {
            filtered.push_back(b.get());
        }
    }

    if (filtered.empty()) {
        std::cout << "No benchmarks matched.\n";
        return;
    }

    if (config.list_only) {
        for (auto* b : filtered) {
            std::cout << std::left << std::setw(45) << b->name() << "[" << b->group() << "]\n";
        }
        std::cout << "\n" << filtered.size() << " benchmarks\n";
        return;
    }

    if (config.show_info) {
        for (auto* b : filtered) {
            std::cout << b->name() << "\n";
            std::cout << "  Group:       " << b->group() << "\n";
            std::cout << "  Description: " << b->description() << "\n";
            std::cout << "  Runs:        " << b->nruns() << "\n";
            std::cout << "\n";
        }
        return;
    }

    if (config.show_query) {
        for (auto* b : filtered) {
            std::cout << "-- " << b->name() << "\n";
            std::cout << b->query() << "\n\n";
        }
        return;
    }

    print_benchmark_methodology_warnings(config);

    // Load-only mode: create one shared instance, run load() for first benchmark per group, then exit
    if (config.load_only) {
        if (config.disk_on || config.wal_on) {
            recreate_benchmark_state_root(config);
        }
        benchmark_instance_t instance(config);
        benchmark_state_t state;
        state.dispatcher = instance.dispatcher();
        state.session = session_id_t();

        std::set<std::string> loaded_groups;
        for (auto* b : filtered) {
            if (loaded_groups.count(b->group())) {
                continue;
            }
            loaded_groups.insert(b->group());
            if (config.verbose) {
                std::cout << "Loading data for group: " << b->group() << " (via " << b->name() << ")\n";
            }
            try {
                state.failed = false;
                b->load(state);
                if (state.failed) {
                    std::cerr << "Error loading group " << b->group() << " (see stderr above)\n";
                } else {
                    std::cout << "Loaded group: " << b->group() << "\n";
                }
            } catch (const std::exception& e) {
                std::cerr << "Error loading group " << b->group() << ": " << e.what() << "\n";
            }
        }
        // Ensure disk state is durable for subsequent --skip-load runs in another process.
        if (config.disk_on) {
            auto checkpoint = state.dispatcher->execute_sql(state.session, "CHECKPOINT");
            if (checkpoint->is_error() && config.verbose) {
                std::cerr << "CHECKPOINT failed after load-only: " << checkpoint->get_error().what << "\n";
            }
        }
        std::cout << "Load-only complete. " << loaded_groups.size() << " groups loaded.\n";
        return;
    }

    std::ofstream csv_file;
    if (!config.output_file.empty()) {
        csv_file.open(config.output_file);
        if (csv_file.is_open()) {
            csv_file << "name,group,layout,disk,warm_cache,warmup,trace,build_type,nruns,min_ms,max_ms,avg_ms,"
                        "median_ms,verified,row_count,column_count,result_hash,error_message\n";
        }
    }

    report_header(std::cout);

    for (auto* b : filtered) {
        auto result = run_single(*b, config);
        report_result(result, std::cout);

        if (csv_file.is_open()) {
            csv_file << std::fixed << std::setprecision(3) << result.name << "," << result.group << ","
                     << layout_name(config.layout_policy) << "," << (config.disk_on ? "disk" : "memory") << ","
                     << "warm"
                     << "," << "true"
                     << "," << (trace_enabled() ? "true" : "false") << "," << build_type() << "," << result.nruns
                     << "," << result.min_ms() << "," << result.max_ms() << "," << result.avg_ms() << ","
                     << result.median_ms() << "," << (result.verified ? "OK" : "FAIL") << ",";
            if (result.result_metadata_valid) {
                csv_file << result.row_count << "," << result.column_count << "," << result.result_hash;
            } else {
                csv_file << ",,";
            }
            csv_file << "," << csv_escape(result.error) << "\n";
        }
    }
}

benchmark_result_t benchmark_runner_t::run_single(benchmark_t& bench, const benchmark_configuration_t& config) {
    benchmark_result_t result;
    result.name = bench.name();
    result.group = bench.group();

    auto nruns = config.nruns > 0 ? config.nruns : bench.nruns();
    result.nruns = nruns;

    if (config.verbose) {
        std::cout << "  Loading data for " << bench.name() << "...\n";
        if (config.disk_on || config.wal_on) {
            std::cout << "  State dir: " << benchmark_state_root(config).string()
                      << (config.skip_load ? " (reusing)\n" : " (resetting)\n");
        }
    }

    try {
        if (config.disk_on || config.wal_on) {
            if (config.skip_load) {
                ensure_benchmark_state_root_exists(config);
            } else {
                recreate_benchmark_state_root(config);
            }
        }

        benchmark_instance_t instance(config);
        benchmark_state_t state;
        state.dispatcher = instance.dispatcher();
        state.session = session_id_t();

        auto bail_on_fail = [&]() {
            result.verified = false;
            result.error = !state.error.empty() ? state.error : "see stderr";
        };

        if (!config.skip_load) {
            clear_run_state(state);
            bench.load(state);
            if (state.failed) { bail_on_fail(); return result; }
        }

        // Warmup
        if (config.verbose) {
            std::cout << "  Warmup run...\n";
        }
        clear_run_state(state);
        bench.run(state);
        if (state.failed) { bail_on_fail(); return result; }

        // Timed runs
        for (uint64_t i = 0; i < nruns; ++i) {
            clear_run_state(state);
            auto start = std::chrono::high_resolution_clock::now();
            bench.run(state);
            auto end = std::chrono::high_resolution_clock::now();
            if (state.failed) { bail_on_fail(); return result; }

            auto duration = std::chrono::duration<double, std::milli>(end - start);
            result.timings_ms.push_back(duration.count());

            if (state.result_metadata_valid) {
                if (!result.result_metadata_valid) {
                    result.result_metadata_valid = true;
                    result.row_count = state.row_count;
                    result.column_count = state.column_count;
                    result.result_hash = state.result_hash;
                } else if (result.row_count != state.row_count || result.column_count != state.column_count ||
                           result.result_hash != state.result_hash) {
                    result.verified = false;
                    result.error = "Result fingerprint mismatch across runs";
                    return result;
                }
            }

            if (config.verbose) {
                std::cout << "  Run " << (i + 1) << "/" << nruns << ": " << std::fixed << std::setprecision(3)
                          << duration.count() << " ms\n";
            }
        }

        // Verify
        auto verify_err = bench.verify(state);
        if (!verify_err.empty()) {
            result.verified = false;
            result.error = verify_err;
        }

        bench.cleanup(state);

    } catch (const std::exception& e) {
        result.error = e.what();
        result.verified = false;
    }

    return result;
}

void benchmark_runner_t::report_header(std::ostream& out) {
    out << std::left << std::setw(45) << "Benchmark" << std::right << std::setw(8) << "Runs" << std::setw(12)
        << "Min (ms)" << std::setw(12) << "Max (ms)" << std::setw(12) << "Avg (ms)" << std::setw(12) << "Median"
        << std::setw(10) << "Status"
        << "\n";
    out << std::string(111, '-') << "\n";
}

void benchmark_runner_t::report_result(const benchmark_result_t& result, std::ostream& out) {
    out << std::left << std::setw(45) << result.name << std::right << std::setw(8) << result.nruns;

    if (result.timings_ms.empty()) {
        out << std::setw(12) << "-" << std::setw(12) << "-" << std::setw(12) << "-" << std::setw(12) << "-";
    } else {
        out << std::fixed << std::setprecision(3) << std::setw(12) << result.min_ms() << std::setw(12)
            << result.max_ms() << std::setw(12) << result.avg_ms() << std::setw(12) << result.median_ms();
    }

    if (!result.error.empty()) {
        out << std::setw(10) << "FAIL";
        out << "\n  Error: " << result.error;
    } else {
        out << std::setw(10) << (result.verified ? "OK" : "FAIL");
    }
    out << "\n";
}

} // namespace otterbrix::benchmark
