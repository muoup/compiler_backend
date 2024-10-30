#include "instructions.hpp"

#include <sstream>

#include "codegen.hpp"
#include "valuegen.hpp"
#include "inst_output.hpp"

#include "../../debug/assert.hpp"
#include "dataflow.hpp"

backend::codegen::instruction_return backend::codegen::gen_allocate(
    backend::codegen::function_context &context,
    const ir::block::allocate &allocate,
    const v_operands &virtual_operands
) {
    debug::assert(virtual_operands.empty(), "Allocation size must be a multiple of 8");

    return backend::codegen::instruction_return {
        backend::codegen::stack_allocate(context, allocate.size)
    };
}

backend::codegen::instruction_return backend::codegen::gen_store(
    backend::codegen::function_context &context,
    const ir::block::store &store,
    const v_operands &virtual_operands
) {
    debug::assert(virtual_operands.size() == 2, "Store instruction must have 2 operands");

    backend::codegen::emit_move(context, virtual_operands[1], virtual_operands[0], store.size);
    return backend::codegen::instruction_return {};
}

backend::codegen::instruction_return backend::codegen::gen_load(
        backend::codegen::function_context &context,
        const ir::block::load &load,
        const v_operands &virtual_operands
) {
    debug::assert(virtual_operands.size() == 1, "Load instruction must have 2 operands");

    auto dest = backend::codegen::find_memory(context, load.size);
    backend::codegen::emit_move(context, dest.get(), virtual_operands[0], load.size);

    return {
        .return_dest = std::move(dest)
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
            return "jb";
        case ugt:
            return "ja";
        case ule:
            return "jbe";
        case uge:
            return "jae";

        default:
            throw std::runtime_error("no such icmp type");
    }
}

backend::codegen::instruction_return backend::codegen::gen_icmp(
        backend::codegen::function_context &context,
        const ir::block::icmp &icmp,
        const v_operands &virtual_operands
) {
    debug::assert(virtual_operands.size() == 2, "ICMP instruction must have 2 operands");

    const auto *lhs = context.get_value(virtual_operands[0]);
    const auto *rhs = context.get_value(virtual_operands[1]);
    const auto *icmp_flag = jmp_inst(icmp.type);

    context.ostream << "    cmp     " << lhs->get_address(8) << ", " << rhs->get_address(8) << '\n';

    return {
            .return_dest = std::make_unique<backend::codegen::icmp_result>(icmp_flag)
    };
}

backend::codegen::instruction_return backend::codegen::gen_branch(
        backend::codegen::function_context &context,
        const ir::block::branch &branch,
        const v_operands &virtual_operands
) {
    debug::assert(virtual_operands.size() == 1, "Invalid Parameter Count for Branch");

    const auto *cond = context.get_value(virtual_operands[0]);
    const auto *icmp_result = dynamic_cast<const backend::codegen::icmp_result*>(cond);
    debug::assert(icmp_result, "Parameter of Branch is not a ICMP Result!");

    context.ostream << "    " << icmp_result->flag << "     ." << branch.true_branch << '\n';
    context.ostream << "    jmp     ." << branch.false_branch << '\n';

    return {};
}

backend::codegen::instruction_return backend::codegen::gen_return(
        backend::codegen::function_context &context,
        const ir::block::ret &,
        const v_operands &virtual_operands) {
    const static auto rax = std::make_unique<backend::codegen::register_storage>(backend::codegen::register_t::rax);

    debug::assert(virtual_operands.size() <= 1, "Invalid Parameter Count for Return");

    if (virtual_operands.empty()) {
        context.ostream << "    ret\n";
        return {};
    }

    codegen::emit_move(context, rax.get(), virtual_operands[0], 8);
    context.ostream << "    leave\n\tret\n";
    return {};
}

const char* arithmetic_command(ir::block::arithmetic_type type) {
    switch (type) {
        using enum ir::block::arithmetic_type;

        case add:
            return "add";
        case sub:
            return "sub";
        case mul:
            return "imul";
        case div:
            return "idiv";

        default:
            throw std::runtime_error("no such arithmetic type");
    }
}

backend::codegen::instruction_return backend::codegen::gen_arithmetic(
        backend::codegen::function_context &context,
        const ir::block::arithmetic &arithmetic,
        const v_operands &virtual_operands) {
    debug::assert(virtual_operands.size() == 2, ">2 operands for arithmetic instruction not yet supported");

    const auto rhs = context.get_value(virtual_operands[1]);

    auto reg = backend::codegen::find_register(context);
    auto op = arithmetic_command(arithmetic.type);

    backend::codegen::emit_move(context, reg.get(), virtual_operands[0], 8);
    context.ostream << "    " << op << "     " << reg->get_address(8) << ", " << rhs->get_address(8) << '\n';

    return {
        .return_dest = std::move(reg)
    };
}

backend::codegen::instruction_return backend::codegen::gen_call(
        backend::codegen::function_context &context,
        const ir::block::call &call,
        const v_operands &virtual_operands) {

    bool dropped_reassignable = context.dropped_reassignable;
    context.dropped_reassignable = false;

    std::stringstream pop_calls {};

    backend::codegen::empty_register(context, backend::codegen::register_t::rax);

    for (size_t i = 0; i < virtual_operands.size(); i++) {
        const auto param_reg_id = backend::codegen::param_register((uint8_t) i);

        move_to_register(context, virtual_operands[i], param_reg_id);
    }

    context.ostream << "    call    " << call.name << '\n';
    context.ostream << pop_calls.str();

    context.dropped_reassignable = dropped_reassignable;

    return {
        .return_dest = std::make_unique<backend::codegen::register_storage>(backend::codegen::register_t::rax)
    };
}