#pragma once

#include "../../ir/nodes.hpp"

namespace backend::md {
    struct function_metadata;

    void analyze_variable_lifetimes(ir::global::function &function);
}