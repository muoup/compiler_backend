#include "scope_analyzer.hpp"
#include "node_metadata.hpp"

#include <string>
#include <unordered_map>

void backend::analyze_variable_lifetimes(ir::global::function &function) {
    using value_variant = std::variant<ir::literal, ir::variable>;

    std::unordered_map<std::string, const ir::block::block_instruction*> lifetime_map {};

    const auto document_lifetime = [&] (const value_variant &value_variant, const ir::block::block_instruction &instruction) {
        if (std::holds_alternative<ir::variable>(value_variant)) {
            const auto &variable = std::get<ir::variable>(value_variant);
            lifetime_map[variable.name] = &instruction;
        }
    };

    // First Pass - Document the last instruction where a variable is referenced
    for (const auto &block : function.blocks) {
        for (const auto &instruction : block.instructions) {
            if (instruction.assigned_to.has_value())
                lifetime_map[instruction.assigned_to->name] = &instruction;

            for (const auto &operand : instruction.operands)
                document_lifetime(operand.val, instruction);
        }
    }

    // Second Pass - Assign this information to the metadata
    for (auto &block : function.blocks) {
        for (auto &instruction : block.instructions) {
            auto &metadata = instruction.metadata;

            const auto detect_dropped = [&](const ir::value &val) {
                if (!std::holds_alternative<ir::variable>(val.val))
                    return false;

                const auto &variable = std::get<ir::variable>(val.val);
                return lifetime_map[variable.name] == &instruction;
            };

            for (const auto &operand : instruction.operands)
                metadata->dropped_data.emplace_back(detect_dropped(operand));
        }
    }
}