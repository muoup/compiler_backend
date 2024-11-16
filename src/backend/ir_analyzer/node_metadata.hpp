#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <variant>

#include "../../ir/node_prototypes.hpp"

namespace backend::md {
    struct instruction_metadata {
        const ir::block::block_instruction &instruction;
        std::vector<bool> dropped_data;

        explicit instruction_metadata(const ir::block::block_instruction &instruction)
            : instruction(instruction) {}
    };

    struct function_metadata {
        const ir::global::function &function;

        explicit function_metadata(const ir::global::function &function)
            : function(function) {}
    };
}
