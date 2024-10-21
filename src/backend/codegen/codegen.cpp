#include <ranges>
#include <sstream>

#include "codegen.hpp"
#include "instructions.hpp"
#include "../../debug/assert.hpp"

void backend::codegen::generate(const ir::root& root, std::ostream& ostream) {
    ostream << "[bits 64]\n";

    gen_extern_functions(ostream, root.extern_functions);
    gen_global_strings(ostream, root.global_strings);
    gen_defined_functions(ostream, root.functions);
}

void backend::codegen::gen_extern_functions(std::ostream &ostream, const std::vector<ir::global::extern_function> &extern_functions) {
    ostream << "section .external_functions" << '\n';

    for (const auto &extern_function : extern_functions) {
        ostream << "extern " << extern_function.name << '\n';
    }
}

void backend::codegen::gen_global_strings(std::ostream &ostream, const std::vector<ir::global::global_string> &global_strings) {
    ostream << "section .global_strings" << '\n';

    for (const auto &global_string : global_strings) {
        ostream << global_string.name << ": db " << global_string.value << '\n';
    }
}

void backend::codegen::gen_defined_functions(std::ostream &ostream, const std::vector<ir::global::function> &functions) {
    ostream << "section .text" << '\n';

    for (const auto &function : functions) {
        gen_function(ostream, function);
    }
}

void backend::codegen::gen_function(std::ostream &ostream, const ir::global::function &function) {
    ostream << "\nglobal " << function.name << "\n\n";
    ostream << function.name << ':' << '\n';

    std::stringstream ss;

    backend::codegen::function_context context {
        .ostream = ss,
        .current_function = function,
    };

    for (size_t i = 0; i < function.parameters.size(); i++){
        auto reg = backend::codegen::param_register(i);
        auto reg_storage = std::make_unique<backend::codegen::register_storage>(reg);

        context.map_value(function.parameters[i].name.c_str(), std::move(reg_storage));
    }

    for (const auto &block : function.blocks) {
        context.ostream << "." << block.name << ":\n";

        for (const auto &instruction : block.instructions) {
            for (size_t i = 0; i < instruction.metadata->dropped_data.size(); i++) {
                if (!instruction.metadata->dropped_data[i]) continue;

                if (std::holds_alternative<ir::int_literal>(instruction.operands[i].val)) continue;

                const auto &var = std::get<ir::variable>(instruction.operands[i].val);
                const auto *ptr = context.value_map.at(var.name).get();

                if (auto *reg = dynamic_cast<const backend::codegen::register_storage*>(ptr)) {
                    context.used_register[reg->reg] = true;
                }
            }

            context.current_instruction = instruction.metadata.get();
            auto info = gen_instruction(context, instruction);

            if (!info.valid)
                goto finish;

            for (size_t i = 0; i < instruction.metadata->dropped_data.size(); i++) {
                if (!instruction.metadata->dropped_data[i]) continue;

                const auto &var = std::get<ir::variable>(instruction.operands[i].val);

                context.unmap_value(
                    var.name.c_str()
                );
            }

            if (info.return_dest && instruction.assigned_to) {
                context.map_value(
                    instruction.assigned_to->name.c_str(),
                    std::move(info.return_dest)
                );
            }
        }
    }

    finish:
    ostream << "    push    rbp" << '\n';
    ostream << "    mov     rbp, rsp" << '\n';

    if (context.current_stack_size != 0) {
        ostream << "    sub     rsp, " << context.current_stack_size << '\n';
    }

    std::vector<const char*> dropped_vars;

    for (size_t i = 1; i < register_count; i++) {
        if (context.register_tampered[i]) {
            const auto *reg = backend::codegen::register_as_string((register_t) i, 8);

            dropped_vars.push_back(reg);
        }
    }

    const bool registers_tampered = !dropped_vars.empty();

    if (registers_tampered) {
        auto code = ss.str();

        while (code.find("ret") != std::string::npos) {
            code.replace(code.find("leave\n\tret"), 10, "jmp     .__int_end");
        }

        for (const auto *reg : dropped_vars) {
            ostream << "    push    " << reg << '\n';
        }

        ostream << code;
        ostream << ".__int_end:\n";

        for (const auto *dropped_var : std::ranges::reverse_view(dropped_vars)) {
            ostream << "    pop " << dropped_var << '\n';
        }
        ostream << "    leave\n\tret";
    } else {
        ostream << ss.str();
    }
}

backend::codegen::instruction_return backend::codegen::gen_instruction(backend::codegen::function_context &context, const ir::block::block_instruction &instruction) {
    std::vector<literal> literals;
    std::vector<const vptr*> operands;

    for (const auto& operand : instruction.operands) {
        if (std::holds_alternative<ir::int_literal>(operand.val)) {
            const auto &literal = std::get<ir::int_literal>(operand.val);

            literals.emplace_back(
            std::to_string(literal.value)
            );

            operands.push_back(&literals.back());
        } else if (std::holds_alternative<ir::variable>(operand.val)) {
            const auto &variable = std::get<ir::variable>(operand.val);

            debug::assert(context.value_map.contains(variable.name), "Variable not found in value map");

            operands.push_back(context.value_map.at(variable.name).get());
        }
    }

    if (auto *allocate = dynamic_cast<ir::block::allocate*>(instruction.inst.get())) {
        return gen_allocate(context, *allocate, operands);
    } else if (auto *store = dynamic_cast<ir::block::store*>(instruction.inst.get())) {
        return gen_store(context, *store, operands);
    } else if (auto *load = dynamic_cast<ir::block::load*>(instruction.inst.get())) {
        return gen_load(context, *load, operands);
    } else if (auto *icmp = dynamic_cast<ir::block::icmp*>(instruction.inst.get())) {
        return gen_icmp(context, *icmp, operands);
    } else if (auto *branch = dynamic_cast<ir::block::branch*>(instruction.inst.get())) {
        return gen_branch(context, *branch, operands);
    } else if (auto *ret = dynamic_cast<ir::block::ret*>(instruction.inst.get())) {
        return gen_return(context, *ret, operands);
    } else if (auto *arithmetic = dynamic_cast<ir::block::arithmetic*>(instruction.inst.get())) {
        return gen_arithmetic(context, *arithmetic, operands);
    } else if (auto *call = dynamic_cast<ir::block::call*>(instruction.inst.get())) {
        return gen_call(context, *call, operands);
    }

    return {
        .valid = false
    };
}