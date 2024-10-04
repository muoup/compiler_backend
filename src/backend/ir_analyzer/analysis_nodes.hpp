#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <variant>

namespace ir {
    struct root;

    namespace global {
        struct function;
    }

    namespace block {
        struct block_instruction;
        struct block;
    }
}

namespace backend {
    struct instruction_metadata {
        const ir::block::block_instruction &instruction;
        std::vector<bool> dropped_data;

        explicit instruction_metadata(const ir::block::block_instruction &instruction)
            : instruction(instruction) {};
    };

    struct block_marker {
        const ir::block::block &block;

        explicit block_marker(const ir::block::block &block)
            : block(block) {};
    };

    struct function_metadata {
        const ir::global::function &function;

        explicit function_metadata(const ir::global::function &function)
            : function(function) {}
    };

    struct analysis_root {
        const ir::root &root;

        std::vector<function_metadata> functions;

        explicit analysis_root(const ir::root &root)
            : root(root) {};
    };
}
