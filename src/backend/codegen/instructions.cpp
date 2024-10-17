#include <sstream>
#include "instructions.hpp"
#include "codegen.hpp"
#include "valuegen.hpp"
#include "../../debug/assert.hpp"

backend::codegen::instruction_return backend::codegen::gen_allocate(
    backend::codegen::function_context &context,
    const ir::block::allocate &allocate,
    std::vector<const backend::codegen::vptr*> &virtual_operands
) {
    debug::assert(virtual_operands.empty(), "Allocation size must be a multiple of 8");

    return backend::codegen::instruction_return {
        backend::codegen::stack_allocate(context, allocate.size)
    };
}

backend::codegen::instruction_return backend::codegen::gen_store(
    backend::codegen::function_context &context,
    const ir::block::store &store,
    std::vector<const vptr*> &virtual_operands
) {
    debug::assert(virtual_operands.size() == 2, "Store instruction must have 2 operands");

    const auto *src = virtual_operands[0];
    const auto *dest = virtual_operands[1];

    context.ostream << "    mov " << dest->get_address(store.size) << ", " << src->get_address(store.size) << '\n';

    return backend::codegen::instruction_return {};
}

backend::codegen::instruction_return backend::codegen::gen_load(
        backend::codegen::function_context &context,
        const ir::block::load &load,
        std::vector<const vptr*> &virtual_operands
) {
    debug::assert(virtual_operands.size() == 1, "Load instruction must have 2 operands");

    const auto *src = virtual_operands[0];
    auto reg = backend::codegen::request_register(context);

    context.ostream << "    mov " << reg->get_address(load.size) << ", " << src->get_address(load.size) << '\n';

    return {
        .return_dest = std::move(reg)
    };
}

const char* jmp_inst(ir::block::icmp_type type) {
    switch (type) {
        using enum ir::block::icmp_type;

        case eq:
            return "je";
        case neq:
            return "jne";

        case slt:
            return "jl";
        case sgt:
            return "jg";
        case sle:
            return "jle";
        case sge:
            return "jge";

        case ult:
            return "jl";
        case ugt:
            return "jg";
        case ule:
            return "jle";
        case uge:
            return "jge";

        default:
            throw std::runtime_error("no such icmp type");
    }
}

backend::codegen::instruction_return backend::codegen::gen_icmp(
        backend::codegen::function_context &context,
        const ir::block::icmp &icmp,
        std::vector<const vptr*> &virtual_operands
) {
    debug::assert(virtual_operands.size() == 2, "ICMP instruction must have 2 operands");

    const auto *lhs = virtual_operands[0];
    const auto *rhs = virtual_operands[1];
    const auto *icmp_flag = jmp_inst(icmp.type);

    context.ostream << "    cmp " << lhs->get_address(4) << ", " << rhs->get_address(4) << '\n';

    return {
            .return_dest = std::make_unique<backend::codegen::icmp_result>(icmp_flag)
    };
}

backend::codegen::instruction_return backend::codegen::gen_branch(
        backend::codegen::function_context &context,
        const ir::block::branch &branch,
        std::vector<const vptr*> &virtual_operands
) {
    debug::assert(virtual_operands.size() == 1, "Invalid Parameter Count for Branch");

    const auto *icmp_result = dynamic_cast<const backend::codegen::icmp_result*>(virtual_operands[0]);
    debug::assert(icmp_result, "Parameter of Branch is not a ICMP Result!");

    context.ostream << "    " << icmp_result->flag << " " << branch.true_branch << '\n';
    context.ostream << "    jmp " << branch.false_branch << '\n';

    return {};
}
