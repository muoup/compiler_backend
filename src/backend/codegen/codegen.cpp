#include <sstream>

#include "codegen.hpp"
#include "context/function_context.hpp"
#include "instructions.hpp"
#include "asmgen/asm_nodes.hpp"
#include "context/value_reference.hpp"

void backend::context::generate(const ir::root& root, std::ostream& ostream) {
    std::vector<std::unique_ptr<global_pointer>> global_strings;

    ostream << "[bits 64]\n";

    ostream << "section .global_strings\n";
    for (const auto& global_string : root.global_strings) {
        ostream << global_string.name << " db \"" << global_string.value << "\", 0\n";
        global_strings.emplace_back(std::make_unique<global_pointer>(global_string.name));
    }

    ostream << "section .external_functions\n";
    for (const auto& extern_function : root.extern_functions) {
        ostream << "extern " << extern_function.name << '\n';
    }

    ostream << "section .text\n";
    for (const auto& function : root.functions) {
        gen_function(root, ostream, function, global_strings);
    }
}

void backend::context::gen_function(const ir::root &,
                                    std::ostream &ostream,
                                    const ir::global::function &function,
                                    std::vector<std::unique_ptr<global_pointer>> &global_strings) {
    ostream << "\nglobal " << function.name << "\n\n";
    ostream << function.name << ':' << '\n';

    backend::context::function_context context {
        .return_type = function.return_type,
        .ostream = ostream,
        .global_strings = global_strings,
    };

    context.asm_blocks.emplace_back("__stacksave");
    context.current_label = &context.asm_blocks.back();
    context.add_asm_node<as::inst::stack_save>();

    for (size_t i = 0; i < function.parameters.size(); i++){
        auto reg = backend::context::param_register(i);

        auto &name = function.parameters[i].name;
        auto &size = function.parameters[i].size;

        context.storage.value_map[name] = context.storage.get_register(reg, size);
        context.register_is_param[reg] = true;
    }

    for (const auto &block : function.blocks) {
        context.asm_blocks.emplace_back(block.name);
        context.current_label = &context.asm_blocks.back();

        for (const auto &instruction : block.instructions) {
            context.current_instruction = instruction.metadata.get();

            for (size_t i = 0; i < instruction.metadata->dropped_data.size(); i++) {
                if (!instruction.metadata->dropped_data[i]) continue;
                if (!instruction.operands[i].is_variable()) continue;

                auto var = context.storage.get_value(instruction.operands[i]);

                if (!var.is_variable()) continue;

                context.storage.pending_drop.emplace_back(*var.get_name());
            }

            if (context.auto_drop_reassignable())
                context.storage.drop_reassignable();

            auto info = gen_instruction(context, instruction);

            if (!context.auto_drop_reassignable())
                context.storage.drop_reassignable();

            context.storage.erase_reassignable();

            for (const auto &reg : context.storage.registers) {
                if (reg->owner.starts_with("__temp"))
                    reg->unclaim();
                reg->frozen = false;
            }

            if (info.return_dest && instruction.assigned_to) {
                context.storage.map_value(
                    *instruction.assigned_to,
                    info.return_dest
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

backend::context::instruction_return backend::context::gen_instruction(backend::context::function_context &context, const ir::block::block_instruction &instruction) {
    return ir::block::node_visit(instruction, [&] <typename T> (const T &inst) {
        return gen_instruction<T>(context, inst, instruction.operands);
    });
}