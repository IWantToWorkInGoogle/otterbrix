#pragma once

#include "forward.hpp"

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <components/base/collection_full_name.hpp>
#include <components/catalog/catalog_oids.hpp>
#include <components/expressions/expression.hpp>
#include <memory_resource>
#include <unordered_set>

namespace components::logical_plan {

    class node_t;
    using node_ptr = boost::intrusive_ptr<node_t>;
    using expression_ptr = expressions::expression_ptr;
    using hash_t = expressions::hash_t;

    // Deferred uncorrelated-subquery request, attached to the plan root by the
    // SQL transformer and consumed by the dispatcher BEFORE the main plan runs.
    // The subquery is executed once, standalone, and its result is substituted
    // back into the main plan as a literal (see services/dispatcher pre-pass).
    // Only UNCORRELATED subqueries are represented here — correlated ones cannot
    // run standalone and are rejected at transform time.
    struct subquery_request_t {
        enum class kind_t : uint8_t
        {
            scalar, // (SELECT <single value> ...) — result fills `result_param`
            in_list // col IN (SELECT col ...)     — results fill `placeholder` children
        };
        kind_t kind;
        // Fully-formed logical plan of the subquery body (bare aggregate, not
        // yet resolve-wrapped — the dispatcher pre-pass wraps/resolves it).
        node_ptr subquery_plan;
        // scalar: parameter id reserved in the main plan's HAVING/WHERE compare.
        core::parameter_id_t result_param{};
        // in_list: left-hand column the IN predicate tests.
        expressions::key_t in_left_key;
        // in_list: the (initially empty) union_or compare already wired into the
        // main WHERE tree; the pre-pass appends one eq(in_left_key, $value) child
        // per subquery result row (matching the literal IN-list lowering).
        expression_ptr placeholder;

        explicit subquery_request_t(std::pmr::memory_resource* resource)
            : kind(kind_t::scalar)
            , in_left_key(resource) {}
    };

    // The polymorphic free helper `cfn_of(node_t*)` was removed.
    // Generic walkers that operated on `node_t*` and need the cfn either
    // (a) inline a per-call type switch when truly needed (see
    // validate_logical_plan.cpp::local_node_cfn for the canonical example), or
    // (b) prefer `node->table_oid()` for routing in resolved-stage code.

    class node_t : public boost::intrusive_ref_counter<node_t> {
    public:
        node_t(std::pmr::memory_resource* resource, node_type type);
        virtual ~node_t() = default;

        node_type type() const;
        // Each derived node that needs a user-typed name at the parser/SQL
        // boundary owns a role-named string field (relname_, dbname_,
        // viewname_, ...) and exposes it via role-named accessors (relname(),
        // dbname(), ...). For routing in resolved-stage code (planner,
        // dispatcher, executor, operators after enrich), always use
        // `table_oid()` from the base class.
        const std::string& result_alias() const;
        const std::pmr::vector<std::pmr::string>& output_column_aliases() const;
        std::pmr::vector<std::pmr::string>& output_column_aliases();
        const std::pmr::vector<node_ptr>& children() const;
        std::pmr::vector<node_ptr>& children();
        const std::pmr::vector<expression_ptr>& expressions() const;
        std::pmr::vector<expression_ptr>& expressions();

        // Deferred uncorrelated subqueries attached to this node (only ever
        // populated on the plan root by the SQL transformer; empty otherwise).
        // The dispatcher drains these before executing the main plan.
        const std::pmr::vector<subquery_request_t>& subqueries() const;
        std::pmr::vector<subquery_request_t>& subqueries();

        void set_result_alias(const std::string& alias);
        void reserve_child(std::size_t count);
        void append_child(const node_ptr& child);
        void append_expression(const expression_ptr& expression);
        void append_expressions(const std::vector<expression_ptr>& expressions);
        void append_expressions(const std::pmr::vector<expression_ptr>& expressions);

        // Oid-only equivalent of collection_dependencies — walks the node
        // tree and collects the set of resolved table oids referenced by
        // this plan. INVALID_OID entries are filtered out (wrapper /
        // parser-window / DDL nodes that don't target a single table).
        // Used by dispatcher to populate `context_storage_t::known_oids`.
        std::unordered_set<components::catalog::oid_t> table_oid_dependencies();

        bool operator==(const node_t& rhs) const;
        bool operator!=(const node_t& rhs) const;

        hash_t hash() const;

        // Oid-keyed identity for nodes that target a specific table.
        // Stamped by enrich_logical_plan once cfn → oid is resolved. INVALID_OID
        // for nodes that do not target a table (wrappers like sequence_t,
        // db/ns DDL, query-tree internals like sort/limit).
        components::catalog::oid_t table_oid() const noexcept { return table_oid_; }
        void set_table_oid(components::catalog::oid_t oid) noexcept { table_oid_ = oid; }

        std::string to_string() const;
        std::pmr::memory_resource* resource() const noexcept;

    protected:
        const node_type type_;
        std::string result_alias_;
        std::pmr::vector<std::pmr::string> output_column_aliases_;
        std::pmr::vector<node_ptr> children_;
        std::pmr::vector<expression_ptr> expressions_;
        std::pmr::vector<subquery_request_t> subqueries_;
        // See table_oid()/set_table_oid() above. Default INVALID_OID; enrich
        // is responsible for stamping the resolved oid before plan execution.
        components::catalog::oid_t table_oid_{components::catalog::INVALID_OID};

        void table_oid_dependencies_(std::unordered_set<components::catalog::oid_t>& upper_dependencies);

    private:
        virtual hash_t hash_impl() const = 0;
        virtual std::string to_string_impl() const = 0;
    };

    struct node_hash final {
        size_t operator()(const node_ptr& node) const { return node->hash(); }
    };

    struct node_equal final {
        size_t operator()(const node_ptr& lhs, const node_ptr& rhs) const { return lhs == rhs || *lhs == *rhs; }
    };

    template<class OStream>
    OStream& operator<<(OStream& stream, const node_ptr& node) {
        stream << node->to_string();
        return stream;
    }

} // namespace components::logical_plan
