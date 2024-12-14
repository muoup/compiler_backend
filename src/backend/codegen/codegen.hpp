#pragma once

#include <span>
#include <unordered_map>
#include <functional>

#include "registers.hpp"
#include "valuegen.hpp"

#include "asmgen/asm_nodes.hpp"
#include "../../ir/node_prototypes.hpp"
#include "../ir_analyzer/node_metadata.hpp"

namespace backend::context {
    struct instruction_return;
    struct virtual_memory;

    void generate(const ir::root& root, std::ostream& ostream);
    void gen_function(const ir::root &root, std::ostream &ostream, const ir::global::function &function,
                      std::vector<std::unique_ptr<global_pointer>> &global_strings);

    instruction_return gen_instruction(backend::context::function_context &context, const ir::block::block_instruction &instruction);
}
