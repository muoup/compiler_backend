#include "scope_analyzer.hpp"
#include "node_metadata.hpp"

#include <string>
#include <unordered_map>

void backend::md::analyze_variable_lifetimes(ir::global::function &function) {
    std::unordered_map<std::string, const ir::block::block_instruction*> lifetime_map {};

    const auto document_lifetime = [&] (const ir::value &value, const ir::block::block_instruction &instruction) {
        if (value.is_literal()) return;

        lifetime_map[value.get_name().data()] = &instruction;
    };

    // First Pass - Document the last instruction where a variable is referenced
    for (const auto &block : function.blocks) {
        for (const auto &instruction : block.instructions) {
            if (instruction.assigned_to.has_value())
                lifetime_map[instruction.assigned_to->name] = &instruction;

            for (const auto &operand : instruction.operands)
                document_lifetime(operand, instruction);
        }
    }

    // Second Pass - Assign this information to the metadata
    for (auto &block : function.blocks) {
        for (auto &instruction : block.instructions) {
            auto &metadata = instruction.metadata;

            const auto detect_dropped = [&](const ir::value &val) {
                if (!val.is_variable())
                    return false;

                return lifetime_map[val.get_name().data()] == &instruction;
            };

            for (const auto &operand : instruction.operands)
                metadata->dropped_data.emplace_back(detect_dropped(operand));
        }
    }
}