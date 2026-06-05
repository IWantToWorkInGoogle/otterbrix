#pragma once

#include <components/logical_plan/node.hpp>

namespace components::planner::optimizer {

    // Late rule: after validation, duplicate WHERE predicates that reference both
    // sides of a SQL-89 comma join into the corresponding synthesized cross join.
    // The original WHERE remains in place, so this is a semantic no-op that avoids
    // materializing the full cross product before filtering.
    void push_down_join_predicates(const logical_plan::node_ptr& root);

} // namespace components::planner::optimizer
