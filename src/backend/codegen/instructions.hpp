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

    template <typename T>
    instruction_return gen_instruction(
            function_context &context,
            const T &instruction,
            const v_operands &operands
    ) {
        throw std::runtime_error("Undefined instruction generation function");
    }

#define declare_instruction_gen(instruction_type) template<> \
    instruction_return gen_instruction<ir::block::instruction_type>(\
            function_context &context, \
            const ir::block::instruction_type &inst,\
            const v_operands &operands\
    )

    declare_instruction_gen(literal);

    declare_instruction_gen(allocate);

    declare_instruction_gen(store);

    declare_instruction_gen(load);

    declare_instruction_gen(icmp);

    declare_instruction_gen(branch);

    declare_instruction_gen(jmp);

    declare_instruction_gen(ret);

    declare_instruction_gen(arithmetic);

    declare_instruction_gen(call);

    declare_instruction_gen(phi);

    declare_instruction_gen(select);

    declare_instruction_gen(zext);

    declare_instruction_gen(sext);

    instruction_return gen_literal(
            function_context &context,
            const ir::block::literal &inst,
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

    instruction_return gen_arithmetic_select(
            function_context &context,
            const ir::block::select &select,
            const v_operands &virtual_operands
    );
}
