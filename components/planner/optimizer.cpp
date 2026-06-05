#include "optimizer.hpp"

#include "optimizer/rules/column_pruning.hpp"
#include "optimizer/rules/constant_folding.hpp"
#include "optimizer/rules/join_predicate_pushdown.hpp"

namespace components::planner {

    logical_plan::node_ptr optimize(std::pmr::memory_resource* resource,
                                    logical_plan::node_ptr node,
                                    logical_plan::parameter_node_t* parameters) {
        if (!node) {
            return nullptr;
        }

        // Constant folding: resolve arithmetic on parameters at plan time.
        if (parameters) {
            optimizer::fold_constants(resource, node, parameters);
        }

        return node;
    }

    logical_plan::node_ptr post_validate_optimize(std::pmr::memory_resource* /*resource*/,
                                                  logical_plan::node_ptr node) {
        if (!node) {
            return nullptr;
        }

        optimizer::push_down_join_predicates(node);
        // optimizer::prune_columns(node) is intentionally not enabled here yet:
        // scan projection currently compacts output chunks while expressions keep
        // storage-schema paths. Enabling it can make predicate scans read invalid
        // columns. Keep join predicate pushdown live; re-enable pruning together
        // with expression path remapping or sparse-chunk propagation.

        return node;
    }

} // namespace components::planner
