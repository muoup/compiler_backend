#include "ir_analyzer.hpp"

#include "node_metadata.hpp"
#include "scope_analyzer.hpp"

void add_empty_metadata(ir::root &root);

void backend::md::analyze(ir::root &root) {
    add_empty_metadata(root);

    for (auto &node : root.functions) {
        backend::md::analyze_variable_lifetimes(node);
    }
}

void add_empty_metadata(ir::root &root) {
    for (auto &function : root.functions) {
        function.metadata = std::make_unique<backend::md::function_metadata>(function);

        for (auto &block : function.blocks) {
            for (auto &instruction : block.instructions) {
                instruction.metadata = std::make_unique<backend::md::instruction_metadata>(instruction);
            }
        }
    }
}
