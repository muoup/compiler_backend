#pragma once

#include <ostream>

namespace ir::block {
    struct block_instruction;
}

namespace backend::output {
    void attach_variable_drop(std::ostream &ostream, const ir::block::block_instruction &block_instruction);
}