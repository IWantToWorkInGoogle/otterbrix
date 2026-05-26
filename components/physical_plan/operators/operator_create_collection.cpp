#include "operator_create_collection.hpp"

#include <components/context/context.hpp>
#include <components/configuration/configuration.hpp>
#include <services/disk/manager_disk.hpp>
#include <services/index/manager_index.hpp>

namespace components::operators {

    namespace {
        configuration::disk_layout_policy
        to_disk_layout_policy(components::logical_plan::create_collection_storage_format_t storage_format) {
            using components::logical_plan::create_collection_storage_format_t;

            switch (storage_format) {
                case create_collection_storage_format_t::disk_columnar:
                    return configuration::disk_layout_policy::columnar_only;
                case create_collection_storage_format_t::disk_pax:
                    return configuration::disk_layout_policy::pax_only;
                case create_collection_storage_format_t::disk_auto:
                case create_collection_storage_format_t::in_memory:
                default:
                    return configuration::disk_layout_policy::auto_select;
            }
        }
    } // namespace

    operator_create_collection_t::operator_create_collection_t(std::pmr::memory_resource* resource,
                                                               log_t log,
                                                               components::catalog::oid_t table_oid,
                                                               components::catalog::oid_t database_oid,
                                                               std::vector<table::column_definition_t> columns,
                                                               components::logical_plan::create_collection_storage_format_t
                                                                   storage_format,
                                                               std::vector<catalog_write_t> catalog_writes)
        : read_write_operator_t(resource, std::move(log), operator_type::create_collection)
        , table_oid_(table_oid)
        , database_oid_(database_oid)
        , columns_(std::move(columns))
        , storage_format_(storage_format)
        , catalog_writes_(std::move(catalog_writes)) {}

    void operator_create_collection_t::on_execute_impl(pipeline::context_t* /*ctx*/) { async_wait(); }

    actor_zeta::unique_future<void> operator_create_collection_t::await_async_and_resume(pipeline::context_t* ctx) {
        // Step 1: Create physical storage
        if (storage_format_ != components::logical_plan::create_collection_storage_format_t::in_memory) {
            auto [_, f] = actor_zeta::send(ctx->disk_address,
                                           &services::disk::manager_disk_t::create_storage_disk,
                                           ctx->session,
                                           table_oid_,
                                           database_oid_,
                                           std::move(columns_),
                                           to_disk_layout_policy(storage_format_));
            co_await std::move(f);
        } else if (columns_.empty()) {
            auto [_, f] = actor_zeta::send(ctx->disk_address,
                                           &services::disk::manager_disk_t::create_storage,
                                           ctx->session,
                                           table_oid_,
                                           database_oid_);
            co_await std::move(f);
        } else {
            auto [_, f] = actor_zeta::send(ctx->disk_address,
                                           &services::disk::manager_disk_t::create_storage_with_columns,
                                           ctx->session,
                                           table_oid_,
                                           database_oid_,
                                           std::move(columns_));
            co_await std::move(f);
        }

        // Step 2: Register with index manager (oid-keyed).
        if (ctx->index_address != actor_zeta::address_t::empty_address()) {
            auto [_, f] = actor_zeta::send(ctx->index_address,
                                           &services::index::manager_index_t::register_collection,
                                           ctx->session,
                                           table_oid_);
            co_await std::move(f);
        }

        // Step 3: Write pg_catalog rows (pg_class, pg_attribute, pg_depend)
        components::execution_context_t exec_ctx{ctx->session, ctx->txn, {}};
        for (auto& [tbl_oid, row] : catalog_writes_) {
            auto [_, f] = actor_zeta::send(ctx->disk_address,
                                           &services::disk::manager_disk_t::append_pg_catalog_row,
                                           exec_ctx,
                                           tbl_oid,
                                           std::move(row));
            auto rng = co_await std::move(f);
            if (rng.count > 0)
                ctx->pg_catalog_appends.push_back(std::move(rng));
        }

        mark_executed();
    }

} // namespace components::operators
