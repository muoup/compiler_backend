#include "instructions.hpp"

#include <sstream>

#include "codegen.hpp"
#include "valuegen.hpp"
#include "asmgen/interface.hpp"

#include "dataflow.hpp"
#include "asmgen/asm_nodes.hpp"

template<>
backend::codegen::instruction_return backend::codegen::gen_instruction<ir::block::literal>(
    backend::codegen::function_context &context,
    const ir::block::literal &inst,
    const v_operands &operands
) {
    debug::assert(operands.empty(), "Literal must have no operands");

    return backend::codegen::instruction_return {
        std::make_unique<backend::codegen::vptr_int_literal>(inst.get_return_size(), inst.value.value)
    };
}

template <>
backend::codegen::instruction_return backend::codegen::gen_instruction<ir::block::allocate>(
    backend::codegen::function_context &context,
    const ir::block::allocate &inst,
    const v_operands &operands
) {
    debug::assert(operands.empty(), "Expected no operands for Allocate");

    auto vptr = backend::codegen::stack_allocate(context, inst.size);
    vptr->droppable = false;

    return backend::codegen::instruction_return {
        .return_dest = std::move(vptr)
    };
}

template <>
backend::codegen::instruction_return backend::codegen::gen_instruction<ir::block::store>(
    backend::codegen::function_context &context,
    const ir::block::store &store,
    const v_operands &operands
) {
    debug::assert(operands.size() == 2, "Store instruction must have 2 operands");

    auto mem = context.get_value(operands[0]).gen_operand();
    mem->size = store.size;

    context.add_asm_node<as::inst::mov>(
        std::move(mem),
        context.get_value(operands[1]).gen_operand()
    );

    return backend::codegen::instruction_return {};
}

template <>
backend::codegen::instruction_return backend::codegen::gen_instruction<ir::block::load>(
        backend::codegen::function_context &context,
        const ir::block::load &inst,
        const v_operands &operands
) {
    debug::assert(operands.size() == 1, "Load instruction must have 2 operands");

    auto dest = backend::codegen::find_val_storage(context, inst.size);
    auto access = context.get_value(operands[0]).gen_operand();
    access->size = inst.size;

    context.add_asm_node<as::inst::mov>(
        as::create_operand(dest.get()),
        std::move(access)
    );

    return {
        .return_dest = std::move(dest)
    };
}

template <>
backend::codegen::instruction_return backend::codegen::gen_instruction<ir::block::icmp>(
        backend::codegen::function_context &context,
        const ir::block::icmp &inst,
        const v_operands &operands
) {
    debug::assert(operands.size() == 2, "ICMP instruction must have 2 operands");

    auto lhs = context.get_value(operands[0]);
    auto rhs = context.get_value(operands[1]);

    context.add_asm_node<as::inst::cmp>(
        lhs.gen_operand(),
        rhs.gen_operand()
    );

    return {
        .return_dest = std::make_unique<backend::codegen::icmp_result>(inst.type)
    };
}

template <>
backend::codegen::instruction_return backend::codegen::gen_instruction<ir::block::branch>(
        backend::codegen::function_context &context,
        const ir::block::branch &inst,
        const v_operands &operands
) {
    debug::assert(operands.size() == 1, "Invalid Parameter Count for Branch");

    const auto cond = context.get_value(operands[0]);
    const auto *icmp_result = cond.get_vptr_type<codegen::icmp_result>();
    debug::assert(icmp_result, "Parameter of Branch is not a ICMP Result!");

    context.add_asm_node<as::inst::cond_jmp>(
            icmp_result->flag,
            inst.true_branch
    );

    context.add_asm_node<as::inst::jmp>(
            inst.false_branch
    );

    return {};
}

template <>
backend::codegen::instruction_return backend::codegen::gen_instruction<ir::block::jmp>(
        backend::codegen::function_context &context,
        const ir::block::jmp &inst,
        const v_operands &operands
) {
    debug::assert(operands.empty(), "Invalid Parameter Count for Jump");

    context.add_asm_node<as::inst::jmp>(inst.label);
    return {};
}

template <>
backend::codegen::instruction_return backend::codegen::gen_instruction<ir::block::ret>(
        backend::codegen::function_context &context,
        const ir::block::ret &,
        const v_operands &operands
) {
    const auto rax = std::make_unique<backend::codegen::register_storage>(context.return_type, backend::codegen::register_t::rax);

    debug::assert(operands.size() <= 1, "Invalid Parameter Count for Return");

    if (!operands.empty()) {
        auto ret_val = context.get_value(operands[0]);

        debug::assert(ret_val.get_size() == context.return_type, "Return instruction must return the same type as the function declares.");

        context.add_asm_node<as::inst::mov>(
            as::create_operand(rax.get()),
            ret_val.gen_operand()
        );
    } else {
        debug::assert(context.return_type == ir::value_size::none, "Cannot return nothing from a non-void returning function!");
    }

    context.add_asm_node<as::inst::ret>();
    return {};
}

template <>
backend::codegen::instruction_return backend::codegen::gen_instruction<ir::block::arithmetic>(
        backend::codegen::function_context &context,
        const ir::block::arithmetic &inst,
        const v_operands &operands
) {
    debug::assert(operands.size() == 2, ">2 operands for inst instruction not yet supported");

    const auto lhs = context.get_value(operands[0]);
    const auto rhs = context.get_value(operands[1]);

    auto reg = backend::codegen::force_find_register(context, rhs.get_size());

    context.add_asm_node<as::inst::mov>(
        as::create_operand(reg.get()),
        lhs.gen_operand()
    );

    context.add_asm_node<as::inst::arithmetic>(
        inst.type,
        as::create_operand(reg.get()),
        rhs.gen_operand()
    );

    return {
        .return_dest = std::move(reg)
    };
}

template <>
backend::codegen::instruction_return backend::codegen::gen_instruction<ir::block::call>(
        backend::codegen::function_context &context,
        const ir::block::call &inst,
        const v_operands &operands
) {
    for (size_t i = 0; i < operands.size(); i++) {
        const auto param_reg_id = backend::codegen::param_register((uint8_t) i);
        const auto param_old_storage = context.get_value(operands[i]).get_vptr_type<codegen::register_storage>();

        if (operands[i].get_size() == ir::value_size::ptr) {
            if (auto *stack = context.get_value(operands[i]).get_vptr_type<stack_value>()) {
                codegen::empty_register(context, param_reg_id);
                context.add_asm_node<as::inst::lea>(
                    as::create_operand(param_reg_id, ir::value_size::ptr),
                    as::create_operand(complex_ptr {
                        ir::value_size::param_dependent,
                        -stack->rbp_off,
                        register_t::rbp,
                        1
                    })
                );
                continue;
            } else if (auto *complex = context.get_value(operands[i]).get_vptr_type<complex_ptr>()) {
                codegen::empty_register(context, param_reg_id);

                context.add_asm_node<as::inst::lea>(
                    as::create_operand(param_reg_id, complex->size),
                    as::create_operand(complex)
                );
                continue;
            }
        }

        // If the variable needs to be preserved and is already in the param register,
        // move its ownership to another piece of memory, otherwise, copy it to the param register,
        // even if this produces a redundant move operation, it will be recognized by the asm-gen and
        // discarded during assembly generation.
        if (!context.current_instruction->dropped_data[i]
        &&  param_old_storage && param_old_storage->reg == param_reg_id) {
            empty_register(context, param_reg_id);
        } else {
            copy_to_register(context, operands[i], param_reg_id);
        }
    }

    empty_register(context, backend::codegen::register_t::rax);
    context.add_asm_node<as::inst::mov>(
        as::create_operand(backend::codegen::register_t::rax, ir::value_size::i64),
        as::create_operand(ir::int_literal { ir::value_size::i64, 0 })
    );
    context.add_asm_node<as::inst::call>(inst.name);

    return {
        .return_dest = std::make_unique<backend::codegen::register_storage>(inst.get_return_size(), codegen::register_t::rax)
    };
}

template <>
backend::codegen::instruction_return backend::codegen::gen_instruction<ir::block::phi>(
        backend::codegen::function_context &context,
        const ir::block::phi &inst,
        const backend::codegen::v_operands &operands
) {
    debug::assert(operands.size() == inst.labels.size(), "Invalid Parameter Count for Phi");
    auto mem = backend::codegen::find_val_storage(context, inst.get_return_size());
    const auto &phi_block = context.current_label->name;

    for (size_t op = 0; op < operands.size(); op++) {
        const auto &target = inst.labels[op];
        const auto &val = context.get_value(operands[op]);
        const auto &val_name = operands[0].get_name();

        auto branch = context.find_block(target);
        auto &nodes = context.asm_blocks[branch].nodes;

        if (nodes.empty()) {
            nodes.emplace_back(std::make_unique<as::inst::mov>(
                    as::create_operand(mem.get()),
                    val.gen_operand()
            ));
            continue;
        }

        for (int64_t i = 0; i < (int64_t) nodes.size(); i++) {
            auto iter = nodes.begin() + i;

            if (auto *jmp = dynamic_cast<const as::inst::jmp*>(iter->get())) {
                if (jmp->label_name != phi_block) continue;

                nodes.insert(iter, std::make_unique<as::inst::mov>(
                        as::create_operand(mem.get()),
                        val.gen_operand()
                ));
                i++;
            } else if (auto *cond_jmp = dynamic_cast<as::inst::cond_jmp*>(iter->get())) {
                if (cond_jmp->branch_name != phi_block) continue;

                std::string temp_phi = std::string("__").append(std::to_string(branch)).append("_phi").append(val_name);

                cond_jmp->branch_name = temp_phi;

                auto &temp_block = context.asm_blocks.emplace_back(std::move(temp_phi));

                auto cached_context = context.current_label;
                context.current_label = &temp_block;

                context.add_asm_node<as::inst::mov>(
                    as::create_operand(mem.get()),
                    val.gen_operand()
                );

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

template <>
backend::codegen::instruction_return backend::codegen::gen_instruction<ir::block::select>(
        backend::codegen::function_context &context,
        const ir::block::select &inst,
        const backend::codegen::v_operands &operands
) {
    debug::assert(operands.size() == 3, "Invalid Parameter Count for Select");

    const auto cond = context.get_value(operands[0]).get_vptr_type<codegen::icmp_result>();
    auto true_val= context.get_value(operands[1]);
    auto false_val = context.get_value(operands[2]);

    const auto *icmp = dynamic_cast<const backend::codegen::icmp_result*>(cond);

    debug::assert(true_val.get_size() == false_val.get_size(), "Select Operands must be the same size");
    debug::assert(icmp, "First parameter of Select is not a ICMP Result!");

    if (true_val.is_literal() && false_val.is_literal()) {
        if (auto arith_select = gen_arithmetic_select(context, inst, operands))
            return std::move(*arith_select);

        auto temp = backend::codegen::claim_temp_register(context, true_val.get_size());
        codegen::copy_to_register(context, operands[1], temp->reg);

        true_val.value = temp.release();
    }

    auto icmp_type = icmp->flag;
    auto mem = backend::codegen::force_find_register(context, true_val.get_size());

    context.add_asm_node<as::inst::mov>(
        as::create_operand(mem.get()),
        false_val.gen_operand()
    );

    context.add_asm_node<as::inst::cmov>(
        icmp_type,
        as::create_operand(mem.get()),
        true_val.gen_operand()
    );

    return {
        .return_dest = std::move(mem)
    };
}

std::optional<backend::codegen::instruction_return> backend::codegen::gen_arithmetic_select(
        backend::codegen::function_context &context,
        const ir::block::select &,
        const backend::codegen::v_operands &operands
) {
    debug::assert(operands.size() == 3, "Invalid Parameter Count for Select");

    auto *cond = context.get_value(operands[0]).get_vptr_type<codegen::icmp_result>();
    auto lhs = context.get_value(operands[1]).get_literal();
    auto rhs = context.get_value(operands[2]).get_literal();

    debug::assert(lhs->size == rhs->size, "Select Operands must be the same size");
    debug::assert(cond, "First parameter of Select is not a ICMP Result!");

    if (!lhs.has_value() || rhs.has_value())
        return std::nullopt;

    auto higher = (uint64_t) lhs->value;
    auto lower = (uint64_t) rhs->value;
    auto flag = cond->flag;

    if (lower > higher) {
        std::swap(higher, lower);
        flag = invert(flag);
    }

    auto mem = backend::codegen::force_find_register(context, ir::value_size::i64);

    context.add_asm_node<as::inst::set>(
        flag,
        as::create_operand(mem.get(), ir::value_size::i1)
    );

    // val = lower + (upper - lower) * cond
    context.add_asm_node<as::inst::lea>(
        as::create_operand(mem.get()),
        as::create_operand(complex_ptr {
            ir::value_size::none,
            (int64_t) lower,
            mem->reg,
            (int8_t) higher
        })
    );

    return instruction_return {
        .return_dest = std::move(mem)
    };
}

template <>
backend::codegen::instruction_return backend::codegen::gen_instruction<ir::block::zext>(
        backend::codegen::function_context &context,
        const ir::block::zext &inst,
        const backend::codegen::v_operands &operands
) {
    debug::assert(operands.size() == 1, "Invalid Parameter Count for Zext");

    if (const auto *literal = context.get_value(operands[0]).get_vptr_type<ir::int_literal>()) {
        return {
            .return_dest = std::make_unique<backend::codegen::vptr_int_literal>(inst.get_return_size(), literal->value)
        };
    }

    auto new_mem = backend::codegen::force_find_register(context, inst.get_return_size());

    if (operands[0].get_size() == ir::value_size::i1) {
        context.add_asm_node<as::inst::set>(
            context.get_value(operands[0]).get_vptr_type<codegen::icmp_result>()->flag,
            as::create_operand(new_mem.get(), ir::value_size::i1)
        );
    } else {
        context.add_asm_node<as::inst::mov>(
            as::create_operand(new_mem.get(), operands[0].get_size()),
            context.get_value(operands[0]).gen_operand()
        );
    }

    return {
        .return_dest = std::move(new_mem)
    };
}

template <>
backend::codegen::instruction_return backend::codegen::gen_instruction<ir::block::sext>(
    backend::codegen::function_context &context,
    const ir::block::sext &inst,
    const backend::codegen::v_operands &operands
) {
    debug::assert(operands.size() == 1, "Invalid Parameter Count for Sext");

    auto new_mem = backend::codegen::find_val_storage(context, inst.get_return_size());

    if (context.get_value(operands[0]).get_vptr_type<ir::int_literal>()) {
        return {
            .return_dest = std::make_unique<backend::codegen::vptr_int_literal>(inst.get_return_size(), context.get_value(operands[0]).get_literal()->value)
        };
    }

    if (inst.get_return_size() > operands[0].get_size()) {
        context.add_asm_node<as::inst::movsx>(
            as::create_operand(new_mem.get()),
            context.get_value(operands[0]).gen_operand()
        );
    }

    return {
        .return_dest = std::move(new_mem)
    };
}

template<>
backend::codegen::instruction_return backend::codegen::gen_instruction<ir::block::get_array_ptr>(
    backend::codegen::function_context &context,
    const ir::block::get_array_ptr &inst,
    const v_operands &operands
) {
    debug::assert(operands.size() == 2, "Invalid Parameter Count for Get Array Ptr");

    const auto &array = context.get_value(operands[0]);
    const auto &index = context.get_value(operands[1]);

    const auto index_size = size_in_bytes(inst.element_size);

    if (auto lit = index.get_literal(); lit.has_value()) {
        auto final_offset = lit->value * index_size;

        if (auto *reg_storage = array.get_vptr_type<register_storage>(); reg_storage) {
            return {
                .return_dest = std::make_unique<codegen::complex_ptr>(
                    inst.element_size,
                    final_offset,
                    reg_storage->reg,
                    1
                )
            };
        } else if (auto *stack = array.get_vptr_type<stack_value>(); stack) {
            return {
                .return_dest = std::make_unique<stack_value>(inst.element_size, final_offset - stack->rbp_off)
            };
        }

        throw std::runtime_error("Invalid array pointer type");
    }

    auto mem = backend::codegen::find_val_storage(context, inst.get_return_size());


    context.add_asm_node<as::inst::mov>(
        as::create_operand(mem.get()),
        index.gen_operand()
    );

    context.add_asm_node<as::inst::arithmetic>(
        ir::block::arithmetic_type::mul,
        as::create_operand(mem.get()),
        as::create_operand(ir::int_literal { ir::value_size::i64, static_cast<uint64_t>(-index_size) })
    );

    context.add_asm_node<as::inst::arithmetic>(
        ir::block::arithmetic_type::add,
        as::create_operand(mem.get()),
        array.gen_operand()
    );

    return {
        .return_dest = std::move(mem)
    };
}