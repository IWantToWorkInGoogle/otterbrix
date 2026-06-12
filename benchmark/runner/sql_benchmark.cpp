#include "sql_benchmark.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#include <core/date/date_parse.hpp>

namespace otterbrix::benchmark {

namespace {

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string to_lower_ascii(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (char ch : text) {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return result;
}

std::string format_sql_error(const components::cursor::cursor_t_ptr& cursor) {
    const auto err = cursor->get_error();
    std::string msg = "SQL error: code=";
    msg += std::to_string(static_cast<int>(err.type));
    msg += " what=";
    msg += std::string_view(err.what);
    return msg;
}

std::string compact_sql_for_error(std::string sql) {
    for (char& ch : sql) {
        if (ch == '\n' || ch == '\r' || ch == '\t') {
            ch = ' ';
        }
    }
    constexpr std::size_t max_len = 220;
    if (sql.size() > max_len) {
        sql.resize(max_len);
        sql += "...";
    }
    return sql;
}

std::string strip_comments_and_directives(const std::string& raw) {
    std::string result;
    result.reserve(raw.size());

    size_t i = 0;
    while (i < raw.size()) {
        // Block comments: /* ... */
        if (i + 1 < raw.size() && raw[i] == '/' && raw[i + 1] == '*') {
            auto end = raw.find("*/", i + 2);
            if (end == std::string::npos) {
                break; // unterminated block comment, skip rest
            }
            i = end + 2;
            continue;
        }

        // Line comments: -- ...
        if (i + 1 < raw.size() && raw[i] == '-' && raw[i + 1] == '-') {
            while (i < raw.size() && raw[i] != '\n') {
                ++i;
            }
            continue;
        }

        // TPC-H directives: lines starting with :
        if (raw[i] == ':' && (i == 0 || raw[i - 1] == '\n')) {
            while (i < raw.size() && raw[i] != '\n') {
                ++i;
            }
            continue;
        }

        result += raw[i];
        ++i;
    }

    return result;
}

std::vector<std::string> split_queries(const std::string& sql) {
    std::vector<std::string> queries;
    std::string current;

    for (char ch : sql) {
        if (ch == ';') {
            auto stmt = trim(current);
            if (!stmt.empty()) {
                queries.push_back(std::move(stmt));
            }
            current.clear();
        } else {
            current += ch;
        }
    }

    auto stmt = trim(current);
    if (!stmt.empty()) {
        queries.push_back(std::move(stmt));
    }

    return queries;
}

std::string make_relative_name(const std::filesystem::path& path, const std::filesystem::path& base_dir) {
    auto rel = std::filesystem::relative(path, base_dir);
    auto name = rel.string();
    // Remove extension
    auto dot = name.rfind('.');
    if (dot != std::string::npos) {
        name = name.substr(0, dot);
    }
    return name;
}

std::string make_group(const std::filesystem::path& path, const std::filesystem::path& base_dir) {
    auto rel = std::filesystem::relative(path, base_dir);
    if (rel.has_parent_path()) {
        return rel.parent_path().string();
    }
    return "sql";
}

std::optional<uint64_t> parse_expected_rows(const std::string& raw_sql) {
    std::istringstream iss(raw_sql);
    std::string line;
    while (std::getline(iss, line)) {
        auto trimmed = trim(line);
        if (!trimmed.starts_with("-- @expected_rows ")) {
            continue;
        }
        auto value_str = trim(trimmed.substr(18)); // strlen("-- @expected_rows ")
        if (value_str.empty()) {
            return std::nullopt;
        }
        try {
            return static_cast<uint64_t>(std::stoull(value_str));
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

// --- TPC-H template parameter handling ---

std::vector<sql_parameter_t> tpch_parameters_for_query(const std::string& base_name) {
    auto slash = base_name.find_last_of("/\\");
    auto query = slash == std::string::npos ? base_name : base_name.substr(slash + 1);

    if (query == "q1") return {{":1", "90"}};
    if (query == "q2") return {{":1", "15"}, {":2", "BRASS"}, {":3", "EUROPE"}};
    if (query == "q3") return {{":1", "BUILDING"}, {":2", "1995-03-15"}};
    if (query == "q4") return {{":1", "1993-07-01"}};
    if (query == "q5") return {{":1", "ASIA"}, {":2", "1994-01-01"}};
    if (query == "q6") return {{":1", "1994-01-01"}, {":2", "0.06"}, {":3", "24"}};
    if (query == "q7") return {{":1", "FRANCE"}, {":2", "GERMANY"}};
    if (query == "q8") return {{":1", "BRAZIL"}, {":2", "AMERICA"}, {":3", "ECONOMY ANODIZED STEEL"}};
    if (query == "q9") return {{":1", "green"}};
    if (query == "q10") return {{":1", "1993-10-01"}};
    if (query == "q11") return {{":1", "GERMANY"}, {":2", "0.0001"}};
    if (query == "q12") return {{":1", "MAIL"}, {":2", "SHIP"}, {":3", "1994-01-01"}};
    if (query == "q13") return {{":1", "special"}, {":2", "requests"}};
    if (query == "q14") return {{":1", "1995-09-01"}};
    if (query == "q15") return {{":1", "1996-01-01"}, {":s", "0"}};
    if (query == "q16") {
        return {{":1", "Brand#45"},
                {":2", "MEDIUM POLISHED"},
                {":3", "49"},
                {":4", "14"},
                {":5", "23"},
                {":6", "45"},
                {":7", "19"},
                {":8", "3"},
                {":9", "36"},
                {":10", "9"}};
    }
    if (query == "q17") return {{":1", "Brand#23"}, {":2", "MED BOX"}};
    if (query == "q18") return {{":1", "300"}};
    if (query == "q19") {
        return {{":1", "Brand#12"}, {":2", "Brand#23"}, {":3", "Brand#34"}, {":4", "1"}, {":5", "10"}, {":6", "20"}};
    }
    if (query == "q20") return {{":1", "forest"}, {":2", "1994-01-01"}, {":3", "CANADA"}};
    if (query == "q21") return {{":1", "SAUDI ARABIA"}};
    if (query == "q22") {
        return {{":1", "13"}, {":2", "31"}, {":3", "23"}, {":4", "29"}, {":5", "30"}, {":6", "18"}, {":7", "17"}};
    }
    return {};
}

void replace_all(std::string& text, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::string::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
}

std::string resolve_tpch_parameters(std::string sql, const std::vector<sql_parameter_t>& parameters) {
    auto ordered = parameters;
    std::sort(ordered.begin(), ordered.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.name.size() > rhs.name.size();
    });
    for (const auto& parameter : ordered) {
        replace_all(sql, parameter.name, parameter.value);
    }
    return sql;
}

std::string sql_quote_literal(std::string_view value) {
    std::string result = "'";
    for (char ch : value) {
        if (ch == '\'') {
            result += "''";
        } else {
            result += ch;
        }
    }
    result += "'";
    return result;
}

std::string normalize_tpch_interval_literals(const std::string& sql) {
    static const std::regex interval_literal(
        R"(interval\s+'([0-9]+)'\s+(year|yr|month|mon|week|day|hour|minute|min|second|sec)(?:\s*\(\s*[0-9]+\s*\))?)",
        std::regex_constants::icase);

    std::string result;
    std::size_t last = 0;
    for (std::sregex_iterator it(sql.begin(), sql.end(), interval_literal), end; it != end; ++it) {
        const auto& match = *it;
        result.append(sql, last, static_cast<std::size_t>(match.position()) - last);

        auto unit = to_lower_ascii(match[2].str());
        result += "interval '";
        result += match[1].str();
        result += " ";
        result += unit;
        result += "'";

        last = static_cast<std::size_t>(match.position() + match.length());
    }
    result.append(sql, last, std::string::npos);
    return result;
}

std::string format_iso_date(std::chrono::sys_days date) {
    const auto ymd = std::chrono::year_month_day{date};
    std::ostringstream out;
    out << static_cast<int>(ymd.year()) << "-" << std::setw(2) << std::setfill('0')
        << static_cast<unsigned>(ymd.month()) << "-" << std::setw(2) << std::setfill('0')
        << static_cast<unsigned>(ymd.day());
    return out.str();
}

std::optional<std::string> evaluate_tpch_date_interval(std::string_view date_text,
                                                       std::string_view op,
                                                       std::string_view interval_text) {
    auto date = core::date::parse_date(date_text);
    auto interval = core::date::parse_interval(interval_text);
    if (!date || !interval) {
        return std::nullopt;
    }

    const int sign = op == "-" ? -1 : 1;
    auto result = core::date::to_sys_days(*date);
    if (interval->month.count()) {
        result = core::date::apply_months(result, sign * interval->month.count());
    }
    result += std::chrono::days{sign * interval->day.count()};
    return format_iso_date(result);
}

std::string rewrite_tpch_date_literals_for_string_columns(const std::string& sql) {
    static const std::regex date_interval(
        R"(date\s+'([0-9]{4}-[0-9]{2}-[0-9]{2})'\s*([+-])\s*interval\s+'([^']+)')",
        std::regex_constants::icase);
    static const std::regex date_literal(R"(date\s+'([0-9]{4}-[0-9]{2}-[0-9]{2})')",
                                         std::regex_constants::icase);

    std::string arithmetic_rewritten;
    std::size_t last = 0;
    for (std::sregex_iterator it(sql.begin(), sql.end(), date_interval), end; it != end; ++it) {
        const auto& match = *it;
        arithmetic_rewritten.append(sql, last, static_cast<std::size_t>(match.position()) - last);

        if (auto evaluated = evaluate_tpch_date_interval(match[1].str(), match[2].str(), match[3].str())) {
            arithmetic_rewritten += sql_quote_literal(*evaluated);
        } else {
            arithmetic_rewritten += match.str();
        }

        last = static_cast<std::size_t>(match.position() + match.length());
    }
    arithmetic_rewritten.append(sql, last, std::string::npos);

    std::string result;
    last = 0;
    for (std::sregex_iterator it(arithmetic_rewritten.begin(), arithmetic_rewritten.end(), date_literal), end;
         it != end;
         ++it) {
        const auto& match = *it;
        result.append(arithmetic_rewritten, last, static_cast<std::size_t>(match.position()) - last);
        result += sql_quote_literal(match[1].str());
        last = static_cast<std::size_t>(match.position() + match.length());
    }
    result.append(arithmetic_rewritten, last, std::string::npos);
    return result;
}

std::string rewrite_tpch_extract_year_for_string_columns(const std::string& sql) {
    static const std::regex extract_year(
        R"(extract\s*\(\s*year\s+from\s+([A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)?)\s*\))",
        std::regex_constants::icase);

    std::string result;
    std::size_t last = 0;
    for (std::sregex_iterator it(sql.begin(), sql.end(), extract_year), end; it != end; ++it) {
        const auto& match = *it;
        result.append(sql, last, static_cast<std::size_t>(match.position()) - last);
        result += "substring(";
        result += match[1].str();
        result += ", 1, 4)";
        last = static_cast<std::size_t>(match.position() + match.length());
    }
    result.append(sql, last, std::string::npos);
    return result;
}

std::string adapt_tpch_sql_to_supported_dialect(std::string sql) {
    sql = normalize_tpch_interval_literals(sql);
    sql = rewrite_tpch_date_literals_for_string_columns(sql);
    return rewrite_tpch_extract_year_for_string_columns(sql);
}

std::optional<std::string> find_unresolved_tpch_parameter(const std::string& sql) {
    for (size_t i = 0; i < sql.size(); ++i) {
        if (sql[i] != ':') continue;
        if (i + 1 >= sql.size()) continue;
        const auto next = sql[i + 1];
        if (next == 's') {
            return ":s";
        }
        if (!std::isdigit(static_cast<unsigned char>(next))) {
            continue;
        }
        size_t end = i + 2;
        while (end < sql.size() && std::isdigit(static_cast<unsigned char>(sql[end]))) {
            ++end;
        }
        return sql.substr(i, end - i);
    }
    return std::nullopt;
}

std::filesystem::path resolved_sql_root(const std::filesystem::path& benchmark_dir) {
    return benchmark_dir.parent_path() / "results" / "resolved_sql";
}

bool is_tpch_suite_group(const std::string& group) {
    return group == "tpch" || group == "tpch_otterbrix";
}

bool sql_has_order_by(const std::string& sql) {
    return to_lower_ascii(sql).find("order by") != std::string::npos;
}

std::string uint128_to_string(components::types::uint128_t value) {
    if (value == 0) {
        return "0";
    }
    std::string result;
    while (value > 0) {
        auto digit = static_cast<unsigned>(value % 10);
        result.push_back(static_cast<char>('0' + digit));
        value /= 10;
    }
    std::reverse(result.begin(), result.end());
    return result;
}

std::string int128_to_string(components::types::int128_t value) {
    if (value < 0) {
        return "-" + uint128_to_string(static_cast<components::types::uint128_t>(-value));
    }
    return uint128_to_string(static_cast<components::types::uint128_t>(value));
}

template<typename T>
std::string numeric_to_string(T value) {
    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(value)) return "NaN";
        if (std::isinf(value)) return value < 0 ? "-Infinity" : "Infinity";
        std::ostringstream out;
        out << std::setprecision(std::numeric_limits<T>::max_digits10) << value;
        return out.str();
    } else if constexpr (std::is_unsigned_v<T>) {
        return std::to_string(static_cast<unsigned long long>(value));
    } else if constexpr (std::is_signed_v<T>) {
        return std::to_string(static_cast<long long>(value));
    } else {
        return std::to_string(value);
    }
}

std::string normalized_value(const components::types::logical_value_t& value) {
    using components::types::logical_type;
    using components::types::physical_type;

    if (value.is_null()) {
        return "null";
    }

    const auto logical = value.type().type();
    std::string prefix = "t" + std::to_string(static_cast<int>(logical)) + ":";

    switch (logical) {
        case logical_type::BOOLEAN:
            return prefix + (value.value<bool>() ? "true" : "false");
        case logical_type::TINYINT:
            return prefix + numeric_to_string(value.value<int8_t>());
        case logical_type::SMALLINT:
            return prefix + numeric_to_string(value.value<int16_t>());
        case logical_type::INTEGER:
        case logical_type::INTEGER_LITERAL:
            return prefix + numeric_to_string(value.value<int32_t>());
        case logical_type::BIGINT:
            return prefix + numeric_to_string(value.value<int64_t>());
        case logical_type::HUGEINT:
            return prefix + int128_to_string(value.value<components::types::int128_t>());
        case logical_type::UTINYINT:
            return prefix + numeric_to_string(value.value<uint8_t>());
        case logical_type::USMALLINT:
            return prefix + numeric_to_string(value.value<uint16_t>());
        case logical_type::UINTEGER:
            return prefix + numeric_to_string(value.value<uint32_t>());
        case logical_type::UBIGINT:
            return prefix + numeric_to_string(value.value<uint64_t>());
        case logical_type::UHUGEINT:
            return prefix + uint128_to_string(value.value<components::types::uint128_t>());
        case logical_type::FLOAT:
            return prefix + numeric_to_string(value.value<float>());
        case logical_type::DOUBLE:
            return prefix + numeric_to_string(value.value<double>());
        case logical_type::STRING_LITERAL:
        case logical_type::BLOB: {
            auto text = std::string(value.value<std::string_view>());
            return prefix + std::to_string(text.size()) + ":" + text;
        }
        case logical_type::DATE:
            return prefix + numeric_to_string(value.value<core::date::date_t>().value.count());
        case logical_type::TIME:
            return prefix + numeric_to_string(value.value<core::date::time_t>().value.count());
        case logical_type::TIME_TZ: {
            auto timetz = value.value<core::date::timetz_t>();
            return prefix + numeric_to_string(timetz.time.count()) + "," + numeric_to_string(timetz.zone.count());
        }
        case logical_type::TIMESTAMP:
            return prefix + numeric_to_string(value.value<core::date::timestamp_t>().value.count());
        case logical_type::TIMESTAMP_TZ:
            return prefix + numeric_to_string(value.value<core::date::timestamptz_t>().value.count());
        case logical_type::INTERVAL: {
            auto interval = value.value<core::date::interval_t>();
            return prefix + numeric_to_string(interval.time.count()) + "," + numeric_to_string(interval.day.count()) +
                   "," + numeric_to_string(interval.month.count());
        }
        case logical_type::DECIMAL: {
            const auto* decimal =
                static_cast<const components::types::decimal_logical_type_extension*>(value.type().extension());
            std::string decimal_prefix = prefix + std::to_string(static_cast<unsigned>(decimal->width())) + "," +
                                         std::to_string(static_cast<unsigned>(decimal->scale())) + ":";
            switch (decimal->stored_as()) {
                case physical_type::INT16:
                    return decimal_prefix + numeric_to_string(value.value<int16_t>());
                case physical_type::INT32:
                    return decimal_prefix + numeric_to_string(value.value<int32_t>());
                case physical_type::INT64:
                    return decimal_prefix + numeric_to_string(value.value<int64_t>());
                case physical_type::INT128:
                    return decimal_prefix + int128_to_string(value.value<components::types::int128_t>());
                default:
                    return decimal_prefix + "unsupported";
            }
        }
        case logical_type::LIST:
        case logical_type::ARRAY:
        case logical_type::STRUCT:
        case logical_type::MAP:
        case logical_type::UNION:
        case logical_type::VARIANT: {
            std::string result = prefix + "[";
            const auto& children = value.children();
            for (size_t i = 0; i < children.size(); ++i) {
                if (i > 0) result += ",";
                result += normalized_value(children[i]);
            }
            result += "]";
            return result;
        }
        default:
            return prefix + "unsupported";
    }
}

void fnv1a_update(uint64_t& hash, std::string_view text) {
    constexpr uint64_t prime = 1099511628211ULL;
    for (char raw : text) {
        auto ch = static_cast<unsigned char>(raw);
        hash ^= ch;
        hash *= prime;
    }
}

std::string hex_hash(uint64_t hash) {
    std::ostringstream out;
    out << "fnv64:" << std::hex << std::setw(16) << std::setfill('0') << hash;
    return out.str();
}

std::string normalized_row(const components::cursor::cursor_t_ptr& cursor, uint64_t row_idx) {
    const auto& chunk = cursor->chunk_data();
    std::string result;
    for (uint64_t col = 0; col < chunk.column_count(); ++col) {
        if (col > 0) {
            result += "\x1f";
        }
        result += normalized_value(chunk.value(col, row_idx));
    }
    return result;
}

std::string result_fingerprint(const components::cursor::cursor_t_ptr& cursor, bool order_sensitive) {
    constexpr uint64_t offset = 14695981039346656037ULL;
    uint64_t hash = offset;

    const auto row_count = cursor->size();
    const auto column_count = cursor->chunk_data().column_count();

    fnv1a_update(hash, "otterbrix-result-v1");
    fnv1a_update(hash, "rows=" + std::to_string(row_count));
    fnv1a_update(hash, "cols=" + std::to_string(column_count));

    std::vector<std::string> rows;
    rows.reserve(row_count);
    for (uint64_t row = 0; row < row_count; ++row) {
        rows.push_back(normalized_row(cursor, row));
    }
    if (!order_sensitive) {
        std::sort(rows.begin(), rows.end());
    }

    for (const auto& row : rows) {
        fnv1a_update(hash, "row=");
        fnv1a_update(hash, row);
    }
    return hex_hash(hash);
}

void record_cursor_result(benchmark_state_t& state,
                          const components::cursor::cursor_t_ptr& cursor,
                          bool order_sensitive,
                          bool allow_empty_shape) {
    const auto row_count = cursor->size();
    const auto column_count = cursor->chunk_data().column_count();
    if (!allow_empty_shape && row_count == 0 && column_count == 0) {
        return;
    }

    state.result_metadata_valid = true;
    state.row_count = static_cast<uint64_t>(row_count);
    state.column_count = static_cast<uint64_t>(column_count);
    state.result_hash = result_fingerprint(cursor, order_sensitive);
}

// --- Setup file parsing ---

struct setup_data_t {
    std::string sql;
    std::vector<sql_csv_entry_t> csv_entries;
    std::vector<sql_setup_step_t> steps;
    std::string database;
};

setup_data_t parse_setup_file(const std::filesystem::path& path) {
    setup_data_t data;

    std::ifstream file(path);
    if (!file.is_open()) {
        return data;
    }

    std::string line;
    std::string sql_lines;

    auto flush_sql_step = [&]() {
        auto cleaned = strip_comments_and_directives(sql_lines);
        cleaned = trim(cleaned);
        if (!cleaned.empty()) {
            sql_setup_step_t step;
            step.kind = sql_setup_step_t::kind_t::sql;
            step.sql = std::move(cleaned);
            data.steps.push_back(std::move(step));
        }
        sql_lines.clear();
    };

    while (std::getline(file, line)) {
        auto trimmed = trim(line);

        // Parse @database directive
        if (trimmed.starts_with("-- @database ")) {
            data.database = trim(trimmed.substr(13)); // strlen("-- @database ")
            continue;
        }

        // Parse @load_csv directives from comments
        if (trimmed.starts_with("-- @load_csv ")) {
            flush_sql_step();
            auto args = trimmed.substr(13); // strlen("-- @load_csv ")
            std::istringstream iss(args);
            sql_csv_entry_t entry;
            iss >> entry.path >> entry.table;
            std::string delim;
            if (iss >> delim && !delim.empty()) {
                entry.delimiter = delim[0];
            }
            if (!entry.path.empty() && !entry.table.empty()) {
                data.csv_entries.push_back(entry);

                sql_setup_step_t step;
                step.kind = sql_setup_step_t::kind_t::csv;
                step.csv = std::move(entry);
                data.steps.push_back(std::move(step));
            }
            continue;
        }

        sql_lines += line + "\n";
    }
    flush_sql_step();

    // Strip comments from the SQL portion
    for (const auto& step : data.steps) {
        if (step.kind != sql_setup_step_t::kind_t::sql || step.sql.empty()) {
            continue;
        }
        if (!data.sql.empty()) {
            data.sql += "\n";
        }
        data.sql += step.sql;
    }

    return data;
}

// --- CSV helpers (same logic as interpreted_benchmark.cpp) ---

std::vector<std::string> split_csv_line(const std::string& line, char delimiter) {
    std::vector<std::string> fields;
    std::string field;
    for (char ch : line) {
        if (ch == delimiter) {
            fields.push_back(trim(field));
            field.clear();
        } else {
            field += ch;
        }
    }
    auto trimmed = trim(field);
    if (!trimmed.empty() || !fields.empty()) {
        fields.push_back(trimmed);
    }
    // Remove trailing empty field (TPC-H .tbl files end with delimiter)
    if (!fields.empty() && fields.back().empty()) {
        fields.pop_back();
    }
    return fields;
}

std::string escape_sql_string(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (char ch : s) {
        if (ch == '\'') {
            result += "''";
        } else {
            result += ch;
        }
    }
    return result;
}

bool is_numeric(const std::string& s) {
    if (s.empty()) return false;
    size_t start = 0;
    if (s[0] == '-' || s[0] == '+') start = 1;
    if (start >= s.size()) return false;
    bool has_dot = false;
    for (size_t i = start; i < s.size(); ++i) {
        if (s[i] == '.') {
            if (has_dot) return false;
            has_dot = true;
        } else if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
            return false;
        }
    }
    return true;
}

} // namespace

sql_benchmark_t::sql_benchmark_t(std::string name,
                                 std::string group,
                                 std::string sql,
                                 std::string setup_sql,
                                 std::vector<sql_csv_entry_t> csv_entries,
                                 std::vector<sql_setup_step_t> setup_steps,
                                 std::filesystem::path benchmark_dir,
                                 std::string database,
                                 std::optional<uint64_t> expected_rows,
                                 std::vector<sql_parameter_t> parameters,
                                 bool logical_multi_statement)
    : name_(std::move(name))
    , group_(std::move(group))
    , sql_(std::move(sql))
    , setup_sql_(std::move(setup_sql))
    , csv_entries_(std::move(csv_entries))
    , setup_steps_(std::move(setup_steps))
    , benchmark_dir_(std::move(benchmark_dir))
    , database_(std::move(database))
    , expected_rows_(expected_rows)
    , parameters_(std::move(parameters))
    , logical_multi_statement_(logical_multi_statement) {}

std::string sql_benchmark_t::name() const { return name_; }
std::string sql_benchmark_t::group() const { return group_; }
std::string sql_benchmark_t::description() const { return "SQL: " + name_; }
std::string sql_benchmark_t::query() const { return sql_; }

void sql_benchmark_t::execute_sql_block(benchmark_state_t& state, const std::string& sql) {
    std::string current;
    const bool order_sensitive = sql_has_order_by(sql);

    for (char ch : sql) {
        if (ch == ';') {
            auto stmt = trim(current);
            if (!stmt.empty()) {
                auto cursor = state.dispatcher->execute_sql(state.session, stmt);
                if (cursor->is_error()) {
                    state.error = format_sql_error(cursor) + " while executing: " + compact_sql_for_error(stmt);
                    state.failed = true;
                    return;
                }
                record_cursor_result(state, cursor, order_sensitive, false);
            }
            current.clear();
        } else {
            current += ch;
        }
    }

    auto stmt = trim(current);
    if (!stmt.empty()) {
        auto cursor = state.dispatcher->execute_sql(state.session, stmt);
        if (cursor->is_error()) {
            state.error = format_sql_error(cursor) + " while executing: " + compact_sql_for_error(stmt);
            state.failed = true;
            return;
        }
        record_cursor_result(state, cursor, order_sensitive, false);
    }
}

void sql_benchmark_t::load_csv_file(benchmark_state_t& state, const sql_csv_entry_t& entry) {
    auto load_start = std::chrono::high_resolution_clock::now();
    auto csv_path = std::filesystem::path(entry.path);
    if (!csv_path.is_absolute()) {
        csv_path = benchmark_dir_ / csv_path;
    }

    std::ifstream file(csv_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open CSV file: " + csv_path.string());
    }

    // Read header line to get column names
    std::string header_line;
    if (!std::getline(file, header_line)) {
        throw std::runtime_error("CSV file is empty: " + csv_path.string());
    }
    auto columns = split_csv_line(header_line, entry.delimiter);
    if (columns.empty()) {
        throw std::runtime_error("CSV file has no columns: " + csv_path.string());
    }

    // Build column list string
    std::string col_list;
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) col_list += ", ";
        col_list += columns[i];
    }

    // Read data lines and batch INSERT
    constexpr size_t batch_size = 100;
    std::vector<std::string> value_tuples;
    uint64_t row_num = 0;
    std::string line;

    auto qualified_table = database_.empty() ? entry.table : database_ + "." + entry.table;

    auto flush_batch = [&]() {
        if (value_tuples.empty()) return;
        std::string sql = "INSERT INTO " + qualified_table + " (" + col_list + ") VALUES ";
        for (size_t i = 0; i < value_tuples.size(); ++i) {
            if (i > 0) sql += ", ";
            sql += value_tuples[i];
        }
        auto cursor = state.dispatcher->execute_sql(state.session, sql);
        if (cursor->is_error()) {
            std::string msg = "CSV load SQL error for " + entry.table + ": ";
            msg += std::string_view(cursor->get_error().what);
            state.error = std::move(msg);
            state.failed = true;
            return;
        }
        value_tuples.clear();
    };

    while (std::getline(file, line)) {
        auto trimmed_line = trim(line);
        if (trimmed_line.empty()) continue;

        auto fields = split_csv_line(trimmed_line, entry.delimiter);
        ++row_num;

        std::string tuple = "(";
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i > 0) tuple += ", ";
            if (i < fields.size() && !fields[i].empty()) {
                if (is_numeric(fields[i])) {
                    tuple += fields[i];
                } else {
                    tuple += "'" + escape_sql_string(fields[i]) + "'";
                }
            } else {
                tuple += "NULL";
            }
        }
        tuple += ")";
        value_tuples.push_back(std::move(tuple));

        if (value_tuples.size() >= batch_size) {
            flush_batch();
        }
    }
    flush_batch();

    auto load_end = std::chrono::high_resolution_clock::now();
    auto load_ms = std::chrono::duration<double, std::milli>(load_end - load_start).count();
    std::cout << "  Loaded " << row_num << " rows from " << csv_path.filename().string() << " into " << entry.table
              << " in " << std::fixed << std::setprecision(3) << load_ms << " ms\n";
}

std::string sql_benchmark_t::qualify_sql(const std::string& sql) const {
    if (database_.empty()) return sql;

    std::unordered_set<std::string> table_names;
    for (const auto& entry : csv_entries_) {
        table_names.insert(to_lower_ascii(entry.table));
    }
    if (table_names.empty()) return sql;

    auto is_ident = [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    };
    auto is_clause_end = [](const std::string& word) {
        return word == "where" || word == "group" || word == "order" || word == "having" || word == "limit" ||
               word == "offset" || word == "union" || word == "except" || word == "intersect" || word == "returning";
    };
    auto starts_table_ref = [](const std::string& word) {
        return word == "from" || word == "join" || word == "into" || word == "update" || word == "table";
    };

    std::string result;
    result.reserve(sql.size() + database_.size() * 8);

    bool in_from_list = false;
    bool expect_table = false;
    for (size_t i = 0; i < sql.size();) {
        const char ch = sql[i];
        if (ch == '\'') {
            const size_t start = i++;
            while (i < sql.size()) {
                if (sql[i] == '\'' && i + 1 < sql.size() && sql[i + 1] == '\'') {
                    i += 2;
                    continue;
                }
                if (sql[i++] == '\'') {
                    break;
                }
            }
            result.append(sql, start, i - start);
            continue;
        }
        if (ch == '"') {
            const size_t start = i++;
            while (i < sql.size()) {
                if (sql[i] == '"' && i + 1 < sql.size() && sql[i + 1] == '"') {
                    i += 2;
                    continue;
                }
                if (sql[i++] == '"') {
                    break;
                }
            }
            result.append(sql, start, i - start);
            continue;
        }
        if (!is_ident(ch)) {
            if (ch == ',' && in_from_list) {
                expect_table = true;
            }
            result.push_back(ch);
            ++i;
            continue;
        }

        const size_t start = i;
        while (i < sql.size() && is_ident(sql[i])) {
            ++i;
        }
        const auto token = sql.substr(start, i - start);
        const auto lower = to_lower_ascii(token);
        const bool preceded_by_dot = !result.empty() && result.back() == '.';

        if (is_clause_end(lower)) {
            in_from_list = false;
            expect_table = false;
        } else if (starts_table_ref(lower)) {
            expect_table = true;
            in_from_list = lower == "from";
        } else if (expect_table && !preceded_by_dot && table_names.contains(lower)) {
            result += database_;
            result += ".";
            result += token;
            expect_table = false;
            continue;
        } else if (expect_table && lower != "as") {
            expect_table = false;
        }

        result += token;
    }

    return result;
}

void sql_benchmark_t::write_resolved_artifacts(const std::string& executable_sql) const {
    if (!is_tpch_suite_group(group_)) {
        return;
    }
    if (resolved_artifacts_written_) {
        return;
    }

    auto root = resolved_sql_root(benchmark_dir_);
    auto sql_path = root / (name_ + ".sql");
    std::error_code ec;
    std::filesystem::create_directories(sql_path.parent_path(), ec);
    if (ec) {
        return;
    }

    {
        std::ofstream out(sql_path);
        if (out.is_open()) {
            out << executable_sql;
            if (!executable_sql.empty() && executable_sql.back() != '\n') {
                out << "\n";
            }
        }
    }

    auto params_path = root / (name_ + ".params.csv");
    std::filesystem::create_directories(params_path.parent_path(), ec);
    if (ec) {
        return;
    }
    std::ofstream params(params_path);
    if (!params.is_open()) {
        return;
    }
    params << "query,parameter,value\n";
    for (const auto& parameter : parameters_) {
        params << name_ << "," << parameter.name << "," << parameter.value << "\n";
    }
    resolved_artifacts_written_ = true;
}

void sql_benchmark_t::load(benchmark_state_t& state) {
    // Create database if specified. IF NOT EXISTS keeps multi-query benchmark
    // groups (e.g. SSB) idempotent — first sub-query creates the DB, the rest
    // reuse it without erroring on re-CREATE.
    if (!database_.empty()) {
        auto create_db = "CREATE DATABASE IF NOT EXISTS " + database_;
        auto cursor = state.dispatcher->execute_sql(state.session, create_db);
        if (cursor->is_error()) {
            std::string msg = "Cannot create database: ";
            msg += std::string_view(cursor->get_error().what);
            state.error = std::move(msg);
            state.failed = true;
            return;
        }
    }

    if (!setup_steps_.empty()) {
        for (const auto& step : setup_steps_) {
            if (step.kind == sql_setup_step_t::kind_t::sql) {
                execute_sql_block(state, qualify_sql(step.sql));
            } else {
                load_csv_file(state, step.csv);
            }
            if (state.failed) return;
        }
        return;
    }

    if (!setup_sql_.empty()) {
        execute_sql_block(state, qualify_sql(setup_sql_));
        if (state.failed) return;
    }
    for (const auto& entry : csv_entries_) {
        load_csv_file(state, entry);
        if (state.failed) return;
    }
}

void sql_benchmark_t::run(benchmark_state_t& state) {
    auto qualified = qualify_sql(sql_);
    write_resolved_artifacts(qualified);

    if (is_tpch_suite_group(group_)) {
        if (auto unresolved = find_unresolved_tpch_parameter(qualified)) {
            state.error = "Unresolved TPC-H parameter in " + name_ + ": " + *unresolved;
            state.failed = true;
            return;
        }
    }

    if (logical_multi_statement_) {
        execute_sql_block(state, qualified);
        return;
    }

    auto cursor = state.dispatcher->execute_sql(state.session, qualified);
    if (cursor->is_error()) {
        state.error = format_sql_error(cursor);
        state.failed = true;
        return;
    }
    record_cursor_result(state, cursor, sql_has_order_by(qualified), true);
    if (expected_rows_.has_value() && cursor->size() != expected_rows_.value()) {
        state.error = "Expected rows mismatch: expected " + std::to_string(expected_rows_.value()) + ", got " +
                      std::to_string(cursor->size());
        state.failed = true;
        return;
    }
}

std::vector<std::unique_ptr<sql_benchmark_t>>
sql_benchmark_t::load_from_file(const std::filesystem::path& path, const std::filesystem::path& base_dir) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open SQL file: " + path.string());
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    auto raw = ss.str();

    auto cleaned = strip_comments_and_directives(raw);
    auto queries = split_queries(cleaned);
    auto expected_rows = parse_expected_rows(raw);

    if (queries.empty()) {
        throw std::runtime_error("No SQL queries found in: " + path.string());
    }

    // Look for _setup.sql in same directory
    auto setup_path = path.parent_path() / "_setup.sql";
    setup_data_t setup;
    if (std::filesystem::exists(setup_path)) {
        setup = parse_setup_file(setup_path);
    }

    auto benchmark_dir = path.parent_path();

    std::vector<std::unique_ptr<sql_benchmark_t>> result;
    auto base_name = make_relative_name(path, base_dir);
    auto group = make_group(path, base_dir);

    if (is_tpch_suite_group(group)) {
        auto parameters = tpch_parameters_for_query(base_name);
        auto resolved = resolve_tpch_parameters(cleaned, parameters);
        resolved = adapt_tpch_sql_to_supported_dialect(std::move(resolved));
        auto resolved_queries = split_queries(resolved);
        if (resolved_queries.empty()) {
            throw std::runtime_error("No SQL queries found after TPC-H parameter substitution in: " + path.string());
        }
        if (auto unresolved = find_unresolved_tpch_parameter(resolved)) {
            throw std::runtime_error("Unresolved TPC-H parameter " + *unresolved + " in: " + path.string());
        }

        const bool logical_multi_statement = resolved_queries.size() > 1;
        auto sql = logical_multi_statement ? trim(resolved) : std::move(resolved_queries[0]);
        result.push_back(std::unique_ptr<sql_benchmark_t>(
            new sql_benchmark_t(base_name,
                                group,
                                std::move(sql),
                                setup.sql,
                                setup.csv_entries,
                                setup.steps,
                                benchmark_dir,
                                setup.database,
                                expected_rows,
                                std::move(parameters),
                                logical_multi_statement)));
        return result;
    }

    if (queries.size() == 1) {
        result.push_back(std::unique_ptr<sql_benchmark_t>(
            new sql_benchmark_t(base_name, group, std::move(queries[0]),
                                setup.sql, setup.csv_entries, setup.steps, benchmark_dir,
                                setup.database, expected_rows, {}, false)));
    } else {
        for (size_t i = 0; i < queries.size(); ++i) {
            auto name = base_name + "/q" + std::to_string(i + 1);
            result.push_back(std::unique_ptr<sql_benchmark_t>(
                new sql_benchmark_t(name, group, std::move(queries[i]),
                                    setup.sql, setup.csv_entries, setup.steps, benchmark_dir,
                                    setup.database, expected_rows, {}, false)));
        }
    }

    return result;
}

} // namespace otterbrix::benchmark
