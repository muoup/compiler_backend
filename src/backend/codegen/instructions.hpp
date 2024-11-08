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

    using v_operands = std::vector<ir::value>;

    struct instruction_return {
        std::unique_ptr<vptr> return_dest;
    };

    instruction_return gen_literal(
            backend::codegen::function_context &context,
            const ir::block::literal &literal,
            const v_operands &operands
    );
    instruction_return gen_allocate(
            backend::codegen::function_context &context,
            const ir::block::allocate &allocate,
            const v_operands &operands
    );
    instruction_return gen_store(
            backend::codegen::function_context &context,
            const ir::block::store &store,
            const v_operands &operands
    );
    instruction_return gen_load(
            backend::codegen::function_context &context,
            const ir::block::load &load,
            const v_operands &operands
    );
    instruction_return gen_icmp(
            backend::codegen::function_context &context,
            const ir::block::icmp &icmp,
            const v_operands &operands
    );
    instruction_return gen_branch(
            backend::codegen::function_context &context,
            const ir::block::branch &branch,
            const v_operands &operands
    );
    instruction_return gen_jmp(
            backend::codegen::function_context &context,
            const ir::block::jmp &jmp,
            const v_operands &operands
    );
    instruction_return gen_return(
            backend::codegen::function_context &context,
            const ir::block::ret &ret,
            const v_operands &operands
    );
    instruction_return gen_arithmetic(
            backend::codegen::function_context &context,
            const ir::block::arithmetic &arithmetic,
            const v_operands &operands
    );
    instruction_return gen_call(
            backend::codegen::function_context &context,
            const ir::block::call &call,
            const v_operands &operands
    );
    instruction_return gen_phi(
            backend::codegen::function_context &context,
            const ir::block::phi &phi,
            const v_operands &operands
    );
    instruction_return gen_select(
            backend::codegen::function_context &context,
            const ir::block::select &select,
            const v_operands &operands
    );
    instruction_return gen_arithmetic_select(
            backend::codegen::function_context &context,
            const ir::block::select &select,
            const backend::codegen::v_operands &operands
    );
    instruction_return gen_zext(
            backend::codegen::function_context &context,
            const ir::block::zext &zext,
            const v_operands &operands
    );
    instruction_return gen_sext(
            backend::codegen::function_context &context,
            const ir::block::sext &sext,
            const v_operands &operands
    );

    const char* jmp_inst(ir::block::icmp_type type);
    const char* arithmetic_command(ir::block::arithmetic_type type);
}
