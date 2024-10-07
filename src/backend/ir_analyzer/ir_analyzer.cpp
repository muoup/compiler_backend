#include "ir_analyzer.hpp"
#include "node_metadata.hpp"
#include "../../ir/nodes.hpp"
#include "scope_analyzer.hpp"

void backend::analyze_ir(ir::root &root) {
    add_empty_metadata(root);

    for (auto &node : root.functions) {
        backend::analyze_variable_lifetimes(node);
    }
}

void backend::add_empty_metadata(ir::root &root) {
    for (auto &function : root.functions) {
        function.metadata = std::make_unique<function_metadata>(function);

        for (auto &block : function.blocks) {
            for (auto &instruction : block.instructions) {
                instruction.metadata = std::make_unique<instruction_metadata>(instruction);
            }
        }
    }
}

backend::instruction_metadata::instruction_metadata(const ir::block::block_instruction &instruction)
    : instruction(instruction) {}

backend::function_metadata::function_metadata(const ir::global::function &function)
    : function(function) {}