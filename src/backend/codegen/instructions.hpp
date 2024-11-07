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
            function_context &context,
            const ir::block::literal &literal,
            const v_operands &operands
    );
    instruction_return gen_allocate(
            function_context &context,
            const ir::block::allocate &allocate,
            const v_operands &operands
    );
    instruction_return gen_store(
            function_context &context,
            const ir::block::store &store,
            const v_operands &operands
    );
    instruction_return gen_load(
            function_context &context,
            const ir::block::load &load,
            const v_operands &operands
    );
    instruction_return gen_icmp(
            function_context &context,
            const ir::block::icmp &icmp,
            const v_operands &operands
    );
    instruction_return gen_branch(
            function_context &context,
            const ir::block::branch &branch,
            const v_operands &operands
    );
    instruction_return gen_jmp(
            function_context &context,
            const ir::block::jmp &jmp,
            const v_operands &operands
    );
    instruction_return gen_return(
            function_context &context,
            const ir::block::ret &ret,
            const v_operands &operands
    );
    instruction_return gen_arithmetic(
            function_context &context,
            const ir::block::arithmetic &arithmetic,
            const v_operands &operands
    );
    instruction_return gen_call(
            function_context &context,
            const ir::block::call &call,
            const v_operands &operands
    );
    instruction_return gen_phi(
            function_context &context,
            const ir::block::phi &phi,
            const v_operands &operands
    );
    instruction_return gen_select(
            function_context &context,
            const ir::block::select &select,
            const v_operands &operands
    );
    instruction_return gen_arithmetic_select(
            function_context &context,
            const ir::block::select &select,
            const v_operands &virtual_operands
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
}
