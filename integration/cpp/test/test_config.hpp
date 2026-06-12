#pragma once

#include <integration/cpp/base_spaces.hpp>
#include <services/dispatcher/dispatcher.hpp>
#include <services/disk/manager_disk.hpp>
#include <services/wal/manager_wal_replicate.hpp>
#include <services/wal/wal_sync_mode.hpp>

#include <actor-zeta/send.hpp>

#include <components/catalog/catalog_oids.hpp>

#include <chrono>
#include <thread>
#include <utility>

inline configuration::config test_create_config(const std::filesystem::path& path = std::filesystem::current_path()) {
    return configuration::config::create_config(path);
    // To change log level
    // config.log.level =log_t::level::trace;
}

inline void test_clear_directory(const configuration::config& config) {
    std::filesystem::remove_all(config.main_path);
    std::filesystem::create_directories(config.main_path);
}

class test_spaces final : public otterbrix::base_otterbrix_t {
public:
    test_spaces(const configuration::config& config)
        : otterbrix::base_otterbrix_t(config) {}

    template<typename Fn, typename... Args>
    auto disk_invoke(Fn fn, Args&&... args) {
        auto [_, future] =
            actor_zeta::otterbrix::send(manager_disk_->address(), fn, std::forward<Args>(args)...);
        while (!future.available()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return std::move(future).get();
    }

    template<typename Fn, typename... Args>
    auto wal_invoke(Fn fn, Args&&... args) {
        auto [_, future] =
            actor_zeta::otterbrix::send(manager_wal_->address(), fn, std::forward<Args>(args)...);
        while (!future.available()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return std::move(future).get();
    }

    template<typename Fn, typename... Args>
    auto dispatcher_invoke(Fn fn, Args&&... args) {
        auto [_, future] =
            actor_zeta::otterbrix::send(manager_dispatcher_->address(), fn, std::forward<Args>(args)...);
        while (!future.available()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return std::move(future).get();
    }

#if defined(DEV_MODE)
    void debug_write_committed_physical_update(components::catalog::oid_t table_oid,
                                               std::pmr::vector<int64_t> row_ids,
                                               components::vector::data_chunk_t& data) {
        auto wal_data =
            std::make_unique<components::vector::data_chunk_t>(&resource, data.types(), data.size());
        data.copy(*wal_data, 0);

        manager_disk_->direct_update_sync(table_oid, row_ids, data);
        wal_invoke(&services::wal::manager_wal_replicate_t::write_physical_update,
                   otterbrix::session_id_t(),
                   table_oid,
                   std::move(row_ids),
                   std::move(wal_data),
                   data.size(),
                   std::uint64_t{0});
        wal_invoke(&services::wal::manager_wal_replicate_t::commit_txn,
                   otterbrix::session_id_t(),
                   std::uint64_t{0},
                   services::wal::wal_sync_mode::NORMAL,
                   components::catalog::well_known_oid::main_database);
    }

    components::table::storage::row_group_layout_kind
    debug_first_row_group_layout_kind(components::catalog::oid_t table_oid) const noexcept {
        return manager_disk_->debug_first_row_group_layout_kind_sync(table_oid);
    }

    void debug_reset_first_row_group_scan_path_counts(components::catalog::oid_t table_oid) noexcept {
        manager_disk_->debug_reset_first_row_group_scan_path_counts_sync(table_oid);
    }

    components::table::row_group_scan_path_counts_t
    debug_first_row_group_scan_path_counts(components::catalog::oid_t table_oid) const noexcept {
        return manager_disk_->debug_first_row_group_scan_path_counts_sync(table_oid);
    }
#endif
};
