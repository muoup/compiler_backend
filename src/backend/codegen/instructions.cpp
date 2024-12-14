#include "instructions.hpp"

#include <sstream>

#include "context/function_context.hpp"
#include "context/value_reference.hpp"
#include "valuegen.hpp"

#include "dataflow.hpp"
#include "asmgen/asm_nodes.hpp"
#include "inst_gen.hpp"

template<>
backend::context::instruction_return backend::context::gen_instruction<ir::block::literal>(
    backend::context::function_context &context,
    const ir::block::literal &inst,
    const v_operands &operands
) {
    debug::assert(operands.empty(), "Literal must have no operands");

    return backend::context::instruction_return {
        context.storage.get_misc_storage<vptr_int_literal>(inst.value.size, inst.value.value)
    };
}

template <>
backend::context::instruction_return backend::context::gen_instruction<ir::block::allocate>(
    backend::context::function_context &context,
    const ir::block::allocate &inst,
    const v_operands &operands
) {
    debug::assert(operands.empty(), "Expected no operands for Allocate");

    return backend::context::instruction_return {
        .return_dest = backend::context::stack_allocate(context, inst.size)
    };
}

template <>
backend::context::instruction_return backend::context::gen_instruction<ir::block::store>(
    backend::context::function_context &context,
    const ir::block::store &inst,
    const v_operands &operands
) {
    debug::assert(operands.size() == 2, "Store instruction must have 2 operands");

    context.add_asm_node<as::inst::mov>(
        context.storage.get_value(operands[0]).gen_operand(inst.size),
        context.storage.get_value(operands[1]).gen_operand()
    );

    return backend::context::instruction_return {};
}

template <>
backend::context::instruction_return backend::context::gen_instruction<ir::block::load>(
        backend::context::function_context &context,
        const ir::block::load &inst,
        const v_operands &operands
) {
    debug::assert(operands.size() == 1, "Load instruction must have 2 operands");

    auto dest = backend::context::find_val_storage(context, inst.size);
    auto access = context.storage.get_value(operands[0]).gen_operand();
    access->size = inst.size;

    context.add_asm_node<as::inst::mov>(
        as::create_operand(dest, inst.size),
        std::move(access)
    );

    return {
        .return_dest = dest
    };
}

template <>
backend::context::instruction_return backend::context::gen_instruction<ir::block::icmp>(
        backend::context::function_context &context,
        const ir::block::icmp &inst,
        const v_operands &operands
) {
    debug::assert(operands.size() == 2, "ICMP instruction must have 2 operands");

    auto lhs = context.storage.get_value(operands[0]);
    auto rhs = context.storage.get_value(operands[1]);

    context.add_asm_node<as::inst::cmp>(
            lhs.gen_operand(),
        rhs.gen_operand()
    );

    return {
        .return_dest = context.storage.get_misc_storage<icmp_result>(inst.type)
    };
}

template <>
backend::context::instruction_return backend::context::gen_instruction<ir::block::branch>(
        backend::context::function_context &context,
        const ir::block::branch &inst,
        const v_operands &operands
) {
    debug::assert(operands.size() == 1, "Invalid Parameter Count for Branch");

    const auto cond = context.storage.get_value(operands[0]);
    const auto *icmp_result = cond.get_vptr_type<context::icmp_result>();
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
backend::context::instruction_return backend::context::gen_instruction<ir::block::jmp>(
        backend::context::function_context &context,
        const ir::block::jmp &inst,
        const v_operands &operands
) {
    debug::assert(operands.empty(), "Invalid Parameter Count for Jump");

    context.add_asm_node<as::inst::jmp>(inst.label);
    return {};
}

template <>
backend::context::instruction_return backend::context::gen_instruction<ir::block::ret>(
        backend::context::function_context &context,
        const ir::block::ret &,
        const v_operands &operands
) {
    debug::assert(operands.size() <= 1, "Invalid Parameter Count for Return");

    if (!operands.empty()) {
        auto ret_val = context.storage.get_value(operands[0]);

        debug::assert(ret_val.get_size() == context.return_type, "Return instruction must return the same type as the function declares.");

        context.add_asm_node<as::inst::mov>(
            as::create_operand(rax, ret_val.get_size()),
            ret_val.gen_operand()
        );
    } else {
        debug::assert(context.return_type == ir::value_size::none, "Cannot return nothing from a non-void returning function!");
    }

    context.add_asm_node<as::inst::ret>();
    return {};
}

template <>
backend::context::instruction_return backend::context::gen_instruction<ir::block::arithmetic>(
        backend::context::function_context &context,
        const ir::block::arithmetic &inst,
        const v_operands &operands
) {
    debug::assert(operands.size() == 2, ">2 operands for inst instruction not yet supported");

    auto lhs = context.storage.get_value(operands[0]);
    auto rhs = context.storage.get_value(operands[1]);

    const auto &[dom, sub] = [&]() {
        if (lhs.get_register()) {
            return std::make_pair(lhs, rhs);
        }

        context.storage.ensure_in_register(rhs);
        return std::make_pair(rhs, lhs);
    }();

    context.add_asm_node<as::inst::arithmetic>(
        inst.type,
        dom.gen_operand(),
        sub.gen_operand()
    );

    return {
        .return_dest = *dom.get_vmem()
    };
}

template <>
backend::context::instruction_return backend::context::gen_instruction<ir::block::call>(
        backend::context::function_context &context,
        const ir::block::call &inst,
        const v_operands &operands
) {
    for (size_t i = 0; i < operands.size(); i++) {
        const auto param_reg_id = backend::context::param_register((uint8_t) i);
        auto operand_storage = context.storage.get_value(operands[i]);

        if (operands[i].get_size() == ir::value_size::ptr) {
            if (auto *complex = operand_storage.get_vptr_type<memory_addr>()) {
              context::empty_register(context, param_reg_id);

                context.add_asm_node<as::inst::lea>(
                    as::create_operand(param_reg_id, complex->size),
                    as::create_operand(complex)
                );
                continue;
            }
        }

        if (!context.current_instruction->dropped_data[i] &&
            operand_storage.get_register() == param_reg_id) {
            empty_register(context, param_reg_id);
        } else {
            copy_to_register(context, operands[i], param_reg_id);
        }
    }

    empty_register(context, backend::context::register_t::rax);
    context.add_asm_node<as::inst::mov>(
        as::create_operand(backend::context::register_t::rax, ir::value_size::i64),
        as::create_operand(ir::int_literal { ir::value_size::i64, 0 })
    );
    context.add_asm_node<as::inst::call>(inst.name);

    return {
        .return_dest = context.storage.get_register(backend::context::register_t::rax, inst.get_return_size())
    };
}

template <>
backend::context::instruction_return backend::context::gen_instruction<ir::block::phi>(
        backend::context::function_context &context,
        const ir::block::phi &inst,
        const backend::context::v_operands &operands
) {
    debug::assert(operands.size() == inst.labels.size(), "Invalid Parameter Count for Phi");
    auto mem = backend::context::find_val_storage(context, inst.get_return_size());
    const auto &phi_block = context.current_label->name;

    for (size_t op = 0; op < operands.size(); op++) {
        const auto &target = inst.labels[op];
        const auto &val = context.storage.get_value(operands[op]);
        const auto &val_name = operands[0].get_name();

        auto branch = context.find_block(target);
        auto &nodes = context.asm_blocks[branch].nodes;

        if (nodes.empty()) {
            nodes.emplace_back(std::make_unique<as::inst::mov>(
                    as::create_operand(mem),
                    val.gen_operand()
            ));
            continue;
        }

        for (int64_t i = 0; i < (int64_t) nodes.size(); i++) {
            auto iter = nodes.begin() + i;

            if (auto *jmp = dynamic_cast<const as::inst::jmp*>(iter->get())) {
                if (jmp->label_name != phi_block) continue;

                nodes.insert(iter, std::make_unique<as::inst::mov>(
                        as::create_operand(mem),
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
                    as::create_operand(mem),
                    val.gen_operand()
                );

                context.current_label = cached_context;

                temp_block.nodes.emplace_back(std::make_unique<as::inst::jmp>(phi_block));
            }
        }
    }

    return {
        .return_dest = mem
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
backend::context::instruction_return backend::context::gen_instruction<ir::block::select>(
        backend::context::function_context &context,
        const ir::block::select &inst,
        const backend::context::v_operands &operands
) {
    debug::assert(operands.size() == 3, "Invalid Parameter Count for Select");

    const auto cond = context.storage.get_value(operands[0]).get_vptr_type<context::icmp_result>();
    auto true_val= context.storage.get_value(operands[1]);
    auto false_val = context.storage.get_value(operands[2]);

    const auto *icmp = dynamic_cast<const backend::context::icmp_result*>(cond);

    debug::assert(true_val.get_size() == false_val.get_size(), "Select Operands must be the same size");
    debug::assert(icmp, "First parameter of Select is not a ICMP Result!");

    if (true_val.is_literal() && false_val.is_literal()) {
        if (auto arith_select = gen_arithmetic_select(context, inst, operands))
            return *arith_select;

        context.storage.ensure_in_register(true_val);
    }

    auto icmp_type = icmp->flag;
    auto mem = backend::context::force_find_register(context, true_val.get_size());

    context.add_asm_node<as::inst::mov>(
        as::create_operand(mem),
        false_val.gen_operand()
    );

    context.add_asm_node<as::inst::cmov>(
        icmp_type,
        as::create_operand(mem),
        true_val.gen_operand()
    );

    return {
        .return_dest = mem
    };
}

std::optional<backend::context::instruction_return> backend::context::gen_arithmetic_select(
        backend::context::function_context &context,
        const ir::block::select &,
        const backend::context::v_operands &operands
) {
    debug::assert(operands.size() == 3, "Invalid Parameter Count for Select");

    auto *cond = context.storage.get_value(operands[0]).get_vptr_type<context::icmp_result>();
    auto lhs = context.storage.get_value(operands[1]).get_literal();
    auto rhs = context.storage.get_value(operands[2]).get_literal();

    debug::assert(lhs->size == rhs->size, "Select Operands must be the same size");
    debug::assert(cond, "First parameter of Select is not a ICMP Result!");

    if (!lhs.has_value() || !rhs.has_value())
        return std::nullopt;

    auto higher = (uint64_t) lhs->value;
    auto lower = (uint64_t) rhs->value;
    auto flag = cond->flag;

    if (lower > higher) {
        std::swap(higher, lower);
        flag = invert(flag);
    }

    auto base = lower;
    auto offset = higher - lower;

    if (base >= 10)
        return std::nullopt;

    context.storage.drop_reassignable();

    auto mem = backend::context::force_find_register(context, ir::value_size::i32);

    context.add_asm_node<as::inst::set>(
        flag,
        as::create_operand(mem, ir::value_size::i1)
    );

    backend::codegen::gen_lea (
        context,
        mem,
        base,
        mem,
        offset
    );

    return instruction_return {
        .return_dest = mem
    };
}

template <>
backend::context::instruction_return backend::context::gen_instruction<ir::block::zext>(
        backend::context::function_context &context,
        const ir::block::zext &inst,
        const backend::context::v_operands &operands
) {
    debug::assert(operands.size() == 1, "Invalid Parameter Count for Zext");

    if (const auto *literal = context.storage.get_value(operands[0]).get_vptr_type<ir::int_literal>()) {
        return {
            .return_dest = context.storage.get_misc_storage<vptr_int_literal>(inst.get_return_size(), literal->value)
        };
    }

    context.storage.drop_reassignable();
    auto new_mem = backend::context::force_find_register(context, inst.get_return_size());

    if (operands[0].get_size() == ir::value_size::i1) {
        context.add_asm_node<as::inst::set>(
                context.storage.get_value(operands[0]).get_vptr_type<context::icmp_result>()->flag,
            as::create_operand(new_mem, ir::value_size::i1)
        );
    } else {
        context.add_asm_node<as::inst::mov>(
            as::create_operand(new_mem, operands[0].get_size()),
            context.storage.get_value(operands[0]).gen_operand()
        );
    }

    return {
        .return_dest = new_mem
    };
}

template <>
backend::context::instruction_return backend::context::gen_instruction<ir::block::sext>(
    backend::context::function_context &context,
    const ir::block::sext &inst,
    const backend::context::v_operands &operands
) {
    debug::assert(operands.size() == 1, "Invalid Parameter Count for Sext");

    auto input = context.storage.get_value(operands[0]);
    context.storage.drop_reassignable();

    auto new_mem = backend::context::find_val_storage(context, inst.get_return_size());

    if (context.storage.get_value(operands[0]).get_vptr_type<ir::int_literal>()) {
        return {
            .return_dest = context.storage.get_misc_storage<vptr_int_literal>(
                    inst.get_return_size(),
                    context.storage.get_value(operands[0]).get_literal()->value
            )
        };
    }

    if (inst.get_return_size() > operands[0].get_size()) {
        context.add_asm_node<as::inst::movsx>(
            as::create_operand(new_mem),
            input.gen_operand()
        );
    }

    return {
        .return_dest = new_mem
    };
}

template<>
backend::context::instruction_return backend::context::gen_instruction<ir::block::get_array_ptr>(
    backend::context::function_context &context,
    const ir::block::get_array_ptr &inst,
    const v_operands &operands
) {
    debug::assert(operands.size() == 2, "Invalid Parameter Count for Get Array Ptr");

    auto array = context.storage.get_value(operands[0]);
    auto index = context.storage.get_value(operands[1]);

    auto index_size = size_in_bytes(inst.element_size);

    auto mem = backend::context::force_find_register(context, ir::value_size::ptr);

    context.storage.ensure_in_register(array);

    if (index.is_literal()) {
        return {
            .return_dest = context.storage.get_misc_storage<memory_addr>(
                ir::value_size::ptr,
                index.get_literal()->value * index_size,
                *array.get_register()
            )
        };
    }

    context.storage.ensure_in_register(index);

    if (index_size > 10) {
        context.add_asm_node<as::inst::arithmetic>(
            ir::block::arithmetic_type::mul,
            index.gen_operand(),
            as::create_operand(ir::int_literal { ir::value_size::i64, (uint64_t) index_size })
        );

        index_size = 1;
    }

    return {
        .return_dest = context.storage.get_misc_storage<memory_addr>(
            ir::value_size::ptr,
            0,
            memory_addr::scaled_reg {
                *index.get_register(),
                (int8_t) index_size
            },
            *array.get_register()
        )
    };
}