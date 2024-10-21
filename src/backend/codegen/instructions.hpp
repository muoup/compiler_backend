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

    using v_operands = std::vector<std::string>;

    struct instruction_return {
        std::unique_ptr<vptr> return_dest;
        bool valid = true;
    };

    instruction_return gen_allocate(
            backend::codegen::function_context &context,
            const ir::block::allocate &allocate,
            const v_operands &virtual_operands
    );
    instruction_return gen_store(
            backend::codegen::function_context &context,
            const ir::block::store &store,
            const v_operands &virtual_operands
    );
    instruction_return gen_load(
            backend::codegen::function_context &context,
            const ir::block::load &load,
            const v_operands &virtual_operands
    );
    instruction_return gen_icmp(
            backend::codegen::function_context &context,
            const ir::block::icmp &icmp,
            const v_operands &virtual_operands
    );
    instruction_return gen_branch(
            backend::codegen::function_context &context,
            const ir::block::branch &branch,
            const v_operands &virtual_operands
    );
    instruction_return gen_return(
            backend::codegen::function_context &context,
            const ir::block::ret &ret,
            const v_operands &virtual_operands
    );
    instruction_return gen_arithmetic(
            backend::codegen::function_context &context,
            const ir::block::arithmetic &arithmetic,
            const v_operands &virtual_operands
    );
    instruction_return gen_call(
            backend::codegen::function_context &context,
            const ir::block::call &call,
            const v_operands &virtual_operands
    );
}
