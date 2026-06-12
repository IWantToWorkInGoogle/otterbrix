#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

#include <integration/cpp/wrapper_dispatcher.hpp>
#include <components/table/row_group.hpp>

namespace otterbrix {
class base_otterbrix_t;
}

namespace otterbrix::benchmark {

using components::session::session_id_t;

struct benchmark_state_t {
    base_otterbrix_t* instance = nullptr;
    wrapper_dispatcher_t* dispatcher = nullptr;
    session_id_t session;
    bool failed = false;
    std::string error;
    bool result_metadata_valid = false;
    uint64_t row_count = 0;
    uint64_t column_count = 0;
    std::string result_hash;
};

struct benchmark_result_t {
    std::string name;
    std::string group;
    uint64_t nruns = 0;
    std::vector<double> timings_ms;
    bool verified = true;
    std::string error;
    bool result_metadata_valid = false;
    uint64_t row_count = 0;
    uint64_t column_count = 0;
    std::string result_hash;
    components::table::row_group_scan_path_counts_t scan_path_counts{};

    double min_ms() const {
        if (timings_ms.empty()) return 0.0;
        return *std::min_element(timings_ms.begin(), timings_ms.end());
    }

    double max_ms() const {
        if (timings_ms.empty()) return 0.0;
        return *std::max_element(timings_ms.begin(), timings_ms.end());
    }

    double avg_ms() const {
        if (timings_ms.empty()) return 0.0;
        return std::accumulate(timings_ms.begin(), timings_ms.end(), 0.0) /
               static_cast<double>(timings_ms.size());
    }

    double median_ms() const {
        if (timings_ms.empty()) return 0.0;
        auto sorted = timings_ms;
        std::sort(sorted.begin(), sorted.end());
        auto n = sorted.size();
        if (n % 2 == 0) {
            return (sorted[n / 2 - 1] + sorted[n / 2]) / 2.0;
        }
        return sorted[n / 2];
    }

    double stddev_ms() const {
        auto n = timings_ms.size();
        if (n < 2) return 0.0;
        auto mean = avg_ms();
        double sum_sq = 0.0;
        for (auto value : timings_ms) {
            auto delta = value - mean;
            sum_sq += delta * delta;
        }
        return std::sqrt(sum_sq / static_cast<double>(n - 1));
    }

    double ci95_ms() const {
        auto n = timings_ms.size();
        if (n < 2) return 0.0;
        return 1.96 * stddev_ms() / std::sqrt(static_cast<double>(n));
    }

    double rsd_pct() const {
        if (timings_ms.empty()) return 0.0;
        auto mean = avg_ms();
        return stddev_ms() * 100.0 / mean;
    }
};

class benchmark_t {
public:
    virtual ~benchmark_t() = default;

    virtual std::string name() const = 0;
    virtual std::string group() const = 0;
    virtual std::string description() const = 0;
    virtual std::string query() const = 0;

    virtual void load(benchmark_state_t& state) = 0;
    virtual void run(benchmark_state_t& state) = 0;
    virtual void cleanup(benchmark_state_t& /*state*/) {}
    virtual std::string verify(benchmark_state_t& /*state*/) { return ""; }

    virtual uint64_t nruns() const { return 5; }
    virtual uint64_t timeout_seconds() const { return 30; }
};

} // namespace otterbrix::benchmark
