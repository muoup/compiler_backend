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
    declare_instruction_gen(get_array_ptr);

    std::optional<backend::codegen::instruction_return> gen_arithmetic_select(
            backend::codegen::function_context &context,
            const ir::block::select &,
            const backend::codegen::v_operands &operands
    );
}
