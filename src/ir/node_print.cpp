#include "nodes.hpp"

void ir::block::block_instruction::print(std::ostream &ostream) const {
    ostream << "    ";

    if (assigned_to) {
        assigned_to->print(ostream);
        ostream << " = ";
    }

    inst->print(ostream);

    for (int i = 0; i < operands.size(); i++) {
        ostream << " ";
        ostream << operands[i];

        if (i != operands.size() - 1) ostream << ",";
    }
}