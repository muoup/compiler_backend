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

void backend::codegen::gen_function(const ir::root &,
                                    std::ostream &ostream,
                                    const ir::global::function &function) {
    ostream << "\nglobal " << function.name << "\n\n";
    ostream << function.name << ':' << '\n';

    backend::codegen::function_context context {
        .return_type = function.return_type,
        .ostream = ostream,
    };

    context.asm_blocks.emplace_back("__stacksave");
    context.current_label = &context.asm_blocks.back();
    context.add_asm_node<as::inst::stack_save>();

    for (size_t i = 0; i < function.parameters.size(); i++){
        auto reg = backend::codegen::param_register(i);
        auto size = function.parameters[i].size;

        auto reg_storage = std::make_unique<backend::codegen::register_storage>(size, reg);

        context.map_value(function.parameters[i], std::move(reg_storage));
        context.register_is_param[reg] = true;
    }

    for (const auto &block : function.blocks) {
        context.asm_blocks.emplace_back(block.name);
        context.current_label = &context.asm_blocks.back();

        for (const auto &instruction : block.instructions) {
            context.current_instruction = instruction.metadata.get();

            // Do not drop the value, but allow the compiler to reassign to by the end of the instruction
            // unless the instruction explicitly says that is not allowed
            for (size_t i = 0; i < instruction.metadata->dropped_data.size(); i++) {
                if (!instruction.metadata->dropped_data[i]) continue;
                if (instruction.operands[i].is_literal()) continue;

                const auto var_name = instruction.operands[i].get_name().data();
                const auto *val = context.value_map.at(var_name).get();

                if (auto *reg_storage = dynamic_cast<const backend::codegen::register_storage*>(val)) {
                    context.dropped_available.emplace_back(reg_storage->reg);
                }

                if (context.dropped_reassignable())
                    context.remove_ownership(val, var_name);
            }

            auto info = gen_instruction(context, instruction);

            // Now that the instruction is done and the variable will no longer by referenced,
            // fully drop the value -- i.e. remove it from the value map
            for (size_t i = 0; i < instruction.metadata->dropped_data.size(); i++) {
                if (!instruction.metadata->dropped_data[i]) continue;
                if (!instruction.operands[i].is_variable()) continue;

                context.drop_value(
                    instruction.operands[i].var()
                );
            }

            for (const auto &temp_reg : context.temp_reg_used) {
                context.register_mem[temp_reg] = nullptr;
            }

            context.temp_reg_used.clear();

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

backend::codegen::instruction_return backend::codegen::gen_instruction(backend::codegen::function_context &context, const ir::block::block_instruction &instruction) {
    return ir::block::node_visit(instruction, [&] <typename T> (const T &inst) {
        return gen_instruction<T>(context, inst, instruction.operands);
    });
}