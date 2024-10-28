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
        std::make_unique<backend::codegen::literal>(literal.value)
    };
}

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
    const static auto rax = std::make_unique<backend::codegen::register_storage>(backend::codegen::register_t::rax);

    debug::assert(virtual_operands.size() <= 1, "Invalid Parameter Count for Return");

    if (!virtual_operands.empty())
        codegen::emit_move(context, rax.get(), virtual_operands[0], 8);

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

    auto reg = backend::codegen::find_register(context);
    backend::codegen::emit_move(context, reg.get(), virtual_operands[0], 8);

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
        const v_operands &virtual_operands) {

    bool dropped_reassignable = context.dropped_reassignable;
    context.dropped_reassignable = false;


    backend::codegen::empty_register(context, backend::codegen::register_t::rax);

    for (size_t i = 0; i < virtual_operands.size(); i++) {
        const auto param_reg_id = backend::codegen::param_register((uint8_t) i);

        move_to_register(context, virtual_operands[i], param_reg_id);
    }

    context.add_asm_node<as::inst::call>(call.name);
    context.dropped_reassignable = dropped_reassignable;

    return {
        .return_dest = std::make_unique<backend::codegen::register_storage>(backend::codegen::register_t::rax)
    };
}

backend::codegen::instruction_return backend::codegen::gen_phi(
        backend::codegen::function_context &context,
        const ir::block::phi &phi,
        const backend::codegen::v_operands &virtual_operands
) {
    debug::assert(virtual_operands.size() == phi.branches.size(), "Invalid Parameter Count for Phi");
    auto mem = backend::codegen::find_memory(context, 8);
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

        for (int64_t i = 0; i < nodes.size(); i++) {
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

                auto &temp_block = context.asm_blocks.emplace_back(temp_phi);

                auto cached_context = context.current_label;
                context.current_label = &temp_block;

                codegen::emit_move(context, mem.get(), val_name, 8);

                context.current_label = cached_context;

                temp_block.nodes.emplace_back(std::make_unique<as::inst::jmp>(val_name));
            }
        }
    }

    return {
        .return_dest = std::move(mem)
    };
}