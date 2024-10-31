#include "instructions.hpp"

#include <sstream>

#include "codegen.hpp"
#include "valuegen.hpp"
#include "inst_output.hpp"

#include "../../debug/assert.hpp"
#include "dataflow.hpp"
#include "asmgen/asm_nodes.hpp"

backend::codegen::instruction_return
backend::codegen::gen_literal(
        backend::codegen::function_context &context,
        const ir::block::literal &literal,
        const backend::codegen::v_operands &virtual_operands
) {
    debug::assert(virtual_operands.empty(), "Literal must have no operands");

    return backend::codegen::instruction_return {
        std::make_unique<backend::codegen::literal>(literal.get_return_size(), literal.value.value)
    };
}

backend::codegen::instruction_return backend::codegen::gen_allocate(
    backend::codegen::function_context &context,
    const ir::block::allocate &allocate,
    const v_operands &virtual_operands
) {
    debug::assert(virtual_operands.empty(), "Allocation size must be a multiple of 8");

    auto vptr = backend::codegen::stack_allocate(context, allocate.get_return_size());
    vptr->droppable = false;

    return backend::codegen::instruction_return {
        .return_dest = std::move(vptr)
    };
}

backend::codegen::instruction_return backend::codegen::gen_store(
    backend::codegen::function_context &context,
    const ir::block::store &store,
    const v_operands &virtual_operands
) {
    debug::assert(virtual_operands.size() == 2, "Store instruction must have 2 operands");

    backend::codegen::emit_move(context, virtual_operands[1], virtual_operands[0]);

    return backend::codegen::instruction_return {};
}

backend::codegen::instruction_return backend::codegen::gen_load(
        backend::codegen::function_context &context,
        const ir::block::load &load,
        const v_operands &virtual_operands
) {
    debug::assert(virtual_operands.size() == 1, "Load instruction must have 2 operands");

    auto dest = backend::codegen::find_val_storage(context, load.size);
    backend::codegen::emit_move(context, dest.get(), virtual_operands[0]);

    return {
        .return_dest = std::move(dest)
    };
}

const char* backend::codegen::jmp_inst(ir::block::icmp_type type) {
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

    context.add_asm_node<as::inst::cmp>(
            as::create_operand(lhs),
            as::create_operand(rhs)
    );

    return {
        .return_dest = std::make_unique<backend::codegen::icmp_result>(icmp.type)
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

    context.add_asm_node<as::inst::cond_jmp>(
        icmp_result->flag,
        branch.true_branch
    );

    context.add_asm_node<as::inst::jmp>(
        branch.false_branch
    );

    return {};
}

backend::codegen::instruction_return backend::codegen::gen_jmp(
        backend::codegen::function_context &context,
        const ir::block::jmp &jmp,
        const v_operands &virtual_operands
) {
    debug::assert(virtual_operands.empty(), "Invalid Parameter Count for Jump");

    context.add_asm_node<as::inst::jmp>(jmp.label);
    return {};
}

backend::codegen::instruction_return backend::codegen::gen_return(
        backend::codegen::function_context &context,
        const ir::block::ret &,
        const v_operands &virtual_operands
) {
    const auto rax = std::make_unique<backend::codegen::register_storage>(context.return_type, backend::codegen::register_t::rax);

    debug::assert(virtual_operands.size() <= 1, "Invalid Parameter Count for Return");

    if (!virtual_operands.empty())
        codegen::emit_move(context, rax.get(), virtual_operands[0]);

    context.add_asm_node<as::inst::ret>();
    return {};
}

const char* backend::codegen::arithmetic_command(ir::block::arithmetic_type type) {
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
        const v_operands &virtual_operands
) {
    debug::assert(virtual_operands.size() == 2, ">2 operands for arithmetic instruction not yet supported");

    const auto rhs = context.get_value(virtual_operands[1]);

    auto reg = backend::codegen::find_register(context, rhs->size);
    backend::codegen::emit_move(context, reg.get(), virtual_operands[0]);

    context.add_asm_node<as::inst::arithmetic>(
            arithmetic.type,
            as::create_operand(reg.get()),
            as::create_operand(rhs)
    );

    return {
        .return_dest = std::move(reg)
    };
}

backend::codegen::instruction_return backend::codegen::gen_call(
        backend::codegen::function_context &context,
        const ir::block::call &call,
        const v_operands &virtual_operands
) {
    backend::codegen::empty_register(context, backend::codegen::register_t::rax);

    for (size_t i = 0; i < virtual_operands.size(); i++) {
        const auto param_reg_id = backend::codegen::param_register((uint8_t) i);

        move_to_register(context, virtual_operands[i], param_reg_id);
    }

    context.add_asm_node<as::inst::call>(call.name);

    return {
        .return_dest = std::make_unique<backend::codegen::register_storage>(call.get_return_size(), codegen::register_t::rax)
    };
}

backend::codegen::instruction_return backend::codegen::gen_phi(
        backend::codegen::function_context &context,
        const ir::block::phi &phi,
        const backend::codegen::v_operands &virtual_operands
) {
    debug::assert(virtual_operands.size() == phi.branches.size(), "Invalid Parameter Count for Phi");
    auto mem = backend::codegen::find_val_storage(context, phi.get_return_size());
    const auto &phi_block = context.current_label->name;

    for (size_t op = 0; op < virtual_operands.size(); op++) {
        const auto &target = phi.branches[op];
        const auto &val_name = virtual_operands[op];
        const auto &val = context.get_value(val_name);

        auto branch = context.find_block(target);
        auto &nodes = context.asm_blocks[branch].nodes;

        if (nodes.empty()) {
            nodes.emplace_back(std::make_unique<as::inst::mov>(
                    as::create_operand(mem.get()),
                    as::create_operand(val)
            ));
            continue;
        }

        for (int64_t i = 0; i < (int64_t) nodes.size(); i++) {
            auto iter = nodes.begin() + i;

            if (auto *jmp = dynamic_cast<const as::inst::jmp*>(iter->get())) {
                if (jmp->label_name != phi_block) continue;

                nodes.insert(iter, std::make_unique<as::inst::mov>(
                        as::create_operand(mem.get()),
                        as::create_operand(val)
                ));
                i++;
            } else if (auto *cond_jmp = dynamic_cast<as::inst::cond_jmp*>(iter->get())) {
                if (cond_jmp->branch_name != phi_block) continue;

                std::string temp_phi = std::string("__").append(std::to_string(branch)).append("_phi").append(val_name);

                cond_jmp->branch_name = temp_phi;

                auto &temp_block = context.asm_blocks.emplace_back(std::move(temp_phi));

                auto cached_context = context.current_label;
                context.current_label = &temp_block;

                codegen::emit_move(context, mem.get(), val_name);

                context.current_label = cached_context;

                temp_block.nodes.emplace_back(std::make_unique<as::inst::jmp>(phi_block));
            }
        }
    }

    return {
        .return_dest = std::move(mem)
    };
}

ir::block::icmp_type invert(ir::block::icmp_type type) {
    switch (type) {
        using enum ir::block::icmp_type;

        case eq:
            return neq;
        case neq:
            return eq;

        case slt:
            return sge;
        case sgt:
            return sle;
        case sle:
            return sgt;
        case sge:
            return slt;

        case ult:
            return uge;
        case ugt:
            return ule;
        case ule:
            return ugt;
        case uge:
            return ult;

        default:
            throw std::runtime_error("no such icmp type");
    }
}


backend::codegen::instruction_return backend::codegen::gen_select(
        backend::codegen::function_context &context,
        const ir::block::select &select,
        const backend::codegen::v_operands &virtual_operands
) {
    debug::assert(virtual_operands.size() == 3, "Invalid Parameter Count for Select");

    const auto *cond = context.get_value(virtual_operands[0]);
    auto *lhs = context.get_value(virtual_operands[1]);
    auto *rhs = context.get_value(virtual_operands[2]);

    const auto *icmp = dynamic_cast<const backend::codegen::icmp_result*>(cond);

    debug::assert(icmp, "First parameter of Select is not a ICMP Result!");

    if (!lhs->addressable() && !rhs->addressable()) {
        return gen_arithmetic_select(context, select, virtual_operands);
    }

    auto icmp_type = icmp->flag;

    if (!lhs->addressable()) {
        std::swap(lhs, rhs);
        icmp_type = invert(icmp->flag);
    }

    auto mem = backend::codegen::find_val_storage(context, select.get_return_size());

    context.add_asm_node<as::inst::mov>(
            as::create_operand(mem.get()),
        as::create_operand(rhs)
    );

    context.add_asm_node<as::inst::cmov>(
        icmp_type,
        as::create_operand(mem.get()),
        as::create_operand(lhs)
    );

    return {
        .return_dest = std::move(mem)
    };
}

backend::codegen::instruction_return backend::codegen::gen_arithmetic_select(
        backend::codegen::function_context &context,
        const ir::block::select &,
        const backend::codegen::v_operands &virtual_operands
) {
    debug::assert(virtual_operands.size() == 3, "Invalid Parameter Count for Select");

    const auto *cond = context.get_value(virtual_operands[0]);
    auto *lhs = context.get_value(virtual_operands[1]);
    auto *rhs = context.get_value(virtual_operands[2]);

    debug::assert(lhs->size == rhs->size, "Select Operands must be the same size");

    const auto *icmp = dynamic_cast<const backend::codegen::icmp_result*>(cond);
    const auto *lhs_imm = dynamic_cast<const backend::codegen::literal*>(lhs);
    const auto *rhs_imm = dynamic_cast<const backend::codegen::literal*>(rhs);

    debug::assert(icmp, "First parameter of Select is not a ICMP Result!");
    debug::assert(lhs_imm, "Second parameter of Arithmetic Select is not a Literal!");
    debug::assert(rhs_imm, "Third parameter of Arithmetic Select is not a Literal!");

    auto global_off = (int64_t) lhs_imm->value;
    auto global_multi = (int64_t) rhs_imm->value - global_off;

    auto mem = codegen::force_find_register(context, lhs->size);

    context.add_asm_node<as::inst::set>(
        icmp->flag,
        as::create_operand(mem.get())
    );

    context.add_asm_node<as::inst::arith_lea>(
        as::create_operand(mem.get()),
        global_off,
        std::nullopt,
        std::abs(global_multi),
        as::inst::explicit_register { mem->reg, mem->size }
    );

    return {
        .return_dest = std::move(mem)
    };
}