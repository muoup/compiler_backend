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
    auto reg = backend::codegen::find_register(context);

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

    context.ostream << "    cmp " << lhs->get_address(8) << ", " << rhs->get_address(8) << '\n';

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

    context.ostream << "    " << icmp_result->flag << " ." << branch.true_branch << '\n';
    context.ostream << "    jmp ." << branch.false_branch << '\n';

    return {};
}

backend::codegen::instruction_return backend::codegen::gen_return(
        backend::codegen::function_context &context,
        const ir::block::ret &,
        std::vector<const vptr *> &virtual_operands) {
    debug::assert(virtual_operands.size() <= 1, "Invalid Parameter Count for Return");

    if (virtual_operands.empty()) {
        context.ostream << "    ret\n";
        return {};
    }

    const auto *ret_val = virtual_operands[0];

    // TODO: Allow for different sized-register return
    context.ostream << "    mov rax, " << ret_val->get_address(8) << '\n';
    context.ostream << "    leave\n\tret\n";
    return {};
}

const char* arithmetic_command(ir::block::arithmetic_type type) {
    switch (type) {
        using enum ir::block::arithmetic_type;

        case iadd:
            return "add";
        case isub:
            return "sub";
        case imul:
            return "imul";
        case idiv:
            return "idiv";

        default:
            throw std::runtime_error("no such arithmetic type");
    }
}

backend::codegen::instruction_return backend::codegen::gen_arithmetic(
        backend::codegen::function_context &context,
        const ir::block::arithmetic &arithmetic,
        std::vector<const vptr *> &virtual_operands) {
    debug::assert(virtual_operands.size() == 2, ">2 operands for arithmetic instruction not yet supported");

    const auto *lhs = virtual_operands[0];
    const auto *rhs = virtual_operands[1];

    auto reg = backend::codegen::find_register(context);
    auto op = arithmetic_command(arithmetic.type);

    context.ostream << "    mov " << reg->get_address(8) << ", " << lhs->get_address(8) << '\n';
    context.ostream << "    " << op << " " << reg->get_address(8) << ", " << rhs->get_address(8) << '\n';

    return {
        .return_dest = std::move(reg)
    };
}

backend::codegen::instruction_return backend::codegen::gen_call(
        backend::codegen::function_context &context,
        const ir::block::call &call,
        std::vector<const vptr *> &virtual_operands) {

    std::stringstream pop_calls;

    for (int i = 0; i < virtual_operands.size(); i++) {
        auto *operand = virtual_operands[i];
        const auto &operand_val = context.current_instruction->instruction.operands[i].val;
        const auto param_reg_id = backend::codegen::param_register((uint8_t) i);

        if (context.used_register[param_reg_id]) {
            auto reg = find_register(context);

            for (auto &[key, val] : context.value_map) {
                auto *reg_storage = dynamic_cast<backend::codegen::register_storage*>(val.get());

                if (reg_storage && reg_storage->reg == param_reg_id) {
                    context.ostream << "    mov " << reg->get_address(8) << ", " << reg_storage->get_address(8) << '\n';

                    auto key_str = std::string(key);

                    operand = reg.get();

                    context.unmap_value(key_str.c_str());
                    context.map_value(key_str.c_str(), std::move(reg));
                    break;
                }
            }
        }

        const auto *param_reg = backend::codegen::param_register_string((uint8_t) i, 8);

        if (std::holds_alternative<ir::int_literal>(operand_val)) {
            const auto &int_literal = std::get<ir::int_literal>(operand_val);

            context.ostream << "    mov " << param_reg << ", " << int_literal.value << '\n';
        } else {
            context.ostream << "    mov " << param_reg << ", " << operand->get_address(8) << '\n';
        }
    }

    context.ostream << "    call " << call.name << '\n';

    auto ret = backend::codegen::find_register(context);
    context.ostream << "    mov " << ret->get_address(8) << ", rax\n";

    context.ostream << pop_calls.str();

    return {
        .return_dest = std::move(ret)
    };
}