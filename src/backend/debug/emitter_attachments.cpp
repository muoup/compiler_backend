#include "emitter_attachments.hpp"

#include "../../ir/nodes.hpp"
#include "../../debug/assert.hpp"

#include <vector>

void backend::output::attach_variable_drop(std::ostream &ostream, const ir::block::block_instruction &block_instruction) {
    std::vector<std::string> dropped_vars;

    for (size_t i = 0; i < block_instruction.operands.size(); i++) {
        if (!block_instruction.metadata->dropped_data[i]) continue;

        const auto &operand = block_instruction.operands[i];
        dropped_vars.emplace_back(operand.get_name());
    }

    if (dropped_vars.empty())
        return;

    ostream << "\t\t\t\t\t; Dropped: ";

    for (size_t i = 0; i < dropped_vars.size(); i++) {
        ostream << "%" << dropped_vars[i];

        if (i + 1 < dropped_vars.size())
            ostream << ", ";
    }
}