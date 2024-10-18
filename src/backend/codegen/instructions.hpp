#pragma once

#include <string>
#include <optional>

#include "valuegen.hpp"
#include "../../ir/nodes.hpp"

namespace ir::block {
    struct allocate;
}

namespace backend::codegen {
    struct function_context;

    struct instruction_return {
        std::unique_ptr<vptr> return_dest;
        bool valid = true;
    };

    instruction_return gen_allocate(
            backend::codegen::function_context &context,
            const ir::block::allocate &allocate,
            std::vector<const vptr*> &virtual_operands
    );
    instruction_return gen_store(
            backend::codegen::function_context &context,
            const ir::block::store &store,
            std::vector<const vptr*> &virtual_operands
    );
    instruction_return gen_load(
            backend::codegen::function_context &context,
            const ir::block::load &load,
            std::vector<const vptr*> &virtual_operands
    );
    instruction_return gen_icmp(
            backend::codegen::function_context &context,
            const ir::block::icmp &icmp,
            std::vector<const vptr*> &virtual_operands
    );
    instruction_return gen_branch(
            backend::codegen::function_context &context,
            const ir::block::branch &branch,
            std::vector<const vptr*> &virtual_operands
    );
    instruction_return gen_return(
            backend::codegen::function_context &context,
            const ir::block::ret &ret,
            std::vector<const vptr*> &virtual_operands
    );
    instruction_return gen_arithmetic(
            backend::codegen::function_context &context,
            const ir::block::arithmetic &arithmetic,
            std::vector<const vptr*> &virtual_operands
    );
    instruction_return gen_call(
            backend::codegen::function_context &context,
            const ir::block::call &call,
            std::vector<const vptr*> &virtual_operands
    );
}
