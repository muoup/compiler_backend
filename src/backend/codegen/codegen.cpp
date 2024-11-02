#include <sstream>

#include "codegen.hpp"
#include "instructions.hpp"
#include "asmgen/asm_nodes.hpp"

void backend::codegen::generate(const ir::root& root, std::ostream& ostream) {
    ostream << "[bits 64]\n";

    ostream << "section .global_strings\n";
    for (const auto& global_string : root.global_strings) {
        ostream << global_string.name << " db \"" << global_string.value << "\", 0\n";
    }

    ostream << "section .external_functions\n";
    for (const auto& extern_function : root.extern_functions) {
        ostream << "extern " << extern_function.name << '\n';
    }

    ostream << "section .text\n";
    for (const auto& function : root.functions) {
        gen_function(root, ostream, function);
    }
}

void backend::codegen::gen_function(const ir::root &root,
                                    std::ostream &ostream,
                                    const ir::global::function &function) {
    ostream << "\nglobal " << function.name << "\n\n";
    ostream << function.name << ':' << '\n';

    backend::codegen::function_context context {
        .root = root,
        .return_type = ir::value_size::i32,
        .ostream = ostream,
        .current_function = function,
    };

    context.asm_blocks.emplace_back("__stacksave");
    context.current_label = &context.asm_blocks.back();
    context.add_asm_node<as::inst::stack_save>();

    for (size_t i = 0; i < function.parameters.size(); i++){
        auto reg = backend::codegen::param_register(i);
        auto size = function.parameters[i].size;

        auto reg_storage = std::make_unique<backend::codegen::register_storage>(size, reg);

        context.map_value(function.parameters[i], std::move(reg_storage));
        context.register_parameter[reg] = true;
    }

    for (const auto &block : function.blocks) {
        context.asm_blocks.emplace_back(block.name);
        context.current_label = &context.asm_blocks.back();

        for (const auto &instruction : block.instructions) {
            // Do not drop the value, but allow the compiler to reassign to by the end of the instruction
            // unless the instruction explicitly says that is not allowed
            for (size_t i = 0; i < instruction.metadata->dropped_data.size(); i++) {
                if (!instruction.metadata->dropped_data[i]) continue;
                if (!std::holds_alternative<ir::variable>(instruction.operands[i].val)) continue;

                const auto &var = std::get<ir::variable>(instruction.operands[i].val);
                const auto *val = context.value_map.at(var.name).get();

                if (auto *reg_storage = dynamic_cast<const backend::codegen::register_storage*>(val)) {
                    context.dropped_available.emplace_back(reg_storage->reg);
                }

                if (context.dropped_reassignable)
                    context.remove_ownership(val, var.name.c_str());
            }

            context.current_instruction = instruction.metadata.get();
            auto info = gen_instruction(context, instruction);

            // Now that the instruction is done and the variable will no longer by referenced,
            // fully drop the value -- i.e. remove it from the value map
            for (size_t i = 0; i < instruction.metadata->dropped_data.size(); i++) {
                if (!instruction.metadata->dropped_data[i]) continue;
                if (!std::holds_alternative<ir::variable>(instruction.operands[i].val)) continue;

                const auto &var = std::get<ir::variable>(instruction.operands[i].val);

                context.drop_value(
                    var
                );
            }

            if (info.return_dest && instruction.assigned_to) {
                context.map_value(
                    *instruction.assigned_to,
                    std::move(info.return_dest)
                );
            }
        }
    }

    for (const auto &block : context.asm_blocks) {
        ostream << '.' << block.name << ":\n";
        for (const auto &inst : block.nodes) {
            if (!inst->printable())
                continue;

            inst->print(context);
            context.ostream << '\n';
        }
    }
}

template <typename T>
backend::codegen::instruction_return gen(auto fn,
                                         backend::codegen::function_context &context,
                                         const ir::block::block_instruction &instruction,
                                         const backend::codegen::v_operands &operands) {
    context.dropped_reassignable = instruction.inst->dropped_reassignable();

    auto ret = fn(context, dynamic_cast<const T&>(*instruction.inst), operands);

    context.dropped_reassignable = true;

    return ret;
}

backend::codegen::instruction_return backend::codegen::gen_instruction(backend::codegen::function_context &context, const ir::block::block_instruction &instruction) {
    switch (instruction.inst->type) {
        using enum ir::block::node_type;

        case literal:
            return gen<ir::block::literal>(gen_literal, context, instruction, instruction.operands);
        case allocate:
            return gen<ir::block::allocate>(gen_allocate, context, instruction, instruction.operands);
        case store:
            return gen<ir::block::store>(gen_store, context, instruction, instruction.operands);
        case load:
            return gen<ir::block::load>(gen_load, context, instruction, instruction.operands);
        case icmp:
            return gen<ir::block::icmp>(gen_icmp, context, instruction, instruction.operands);
        case branch:
            return gen<ir::block::branch>(gen_branch, context, instruction, instruction.operands);
        case jmp:
            return gen<ir::block::jmp>(gen_jmp, context, instruction, instruction.operands);
        case ret:
            return gen<ir::block::ret>(gen_return, context, instruction, instruction.operands);
        case arithmetic:
            return gen<ir::block::arithmetic>(gen_arithmetic, context, instruction, instruction.operands);
        case call:
            return gen<ir::block::call>(gen_call, context, instruction, instruction.operands);
        case phi:
            return gen<ir::block::phi>(gen_phi, context, instruction, instruction.operands);
        case select:
            return gen<ir::block::select>(gen_select, context, instruction, instruction.operands);
    }

    throw std::runtime_error("Unknown instruction");
}