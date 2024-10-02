#include <iostream>
#include <format>
#include <algorithm>

#include "codegen.hpp"
#include "registers.hpp"

std::reference_wrapper<std::ostream> output = std::ref(std::cout);

void codegen::generate(const ir::root &root, std::ostream &out) {
    output = std::ref(out);

    output.get() << "[bits 64]" << std::endl;
    output.get() << "global main" << std::endl;
    output.get() << "default rel" << std::endl;

    output.get() << "\nsection .externs" << std::endl;

    for (auto &extern_function : root.extern_functions)
        gen_extern_function(extern_function);

    output.get() << "\nsection .text" << std::endl;

    for (auto &function : root.functions)
        gen_function(function);

    output.get() << "\nsection .global_strings" << std::endl;

    for (auto &global_string : root.global_strings)
        gen_global_string(global_string);
}

void codegen::gen_global_string(const ir::global::global_string &global_string) {
    output.get() << std::format("{} db \"{}\", 0", global_string.name, global_string.value) << std::endl;
}

void codegen::gen_extern_function(const ir::global::extern_function &extern_function) {
    output.get() << std::format("extern {}", extern_function.name) << std::endl;
}

void codegen::gen_function(const ir::global::function &function) {
    output.get() << std::format("{}:", function.name) << std::endl;
    output.get() << "   push rbp" << std::endl;
    output.get() << "   mov rbp, rsp" << std::endl;

    for (auto &block : function.blocks) {
        output.get() << std::format("\n{}:", block.name) << std::endl;

        std::ranges::for_each(block.instructions, gen_instruction);
    }
}

void codegen::gen_instruction(const ir::block::block_instruction &instruction) {
    auto *inst = instruction.inst.get();

    if (auto *call = dynamic_cast<ir::block::call*>(inst)) gen_call(*call);
    else if (auto *ret = dynamic_cast<ir::block::ret*>(inst)) gen_ret(*ret);
}

void codegen::gen_call(const ir::block::call &call) {
    for (size_t i = 0; i < call.args.size(); i++) {
        auto &arg = call.args[i];

        const auto reg = ir::registers::param_register_string(i);

        output.get() << std::format("   mov {}, {}", reg, codegen::gen_value(*arg)) << std::endl;
    }

    output.get() << std::format("   call {}", call.name) << std::endl;
}

void codegen::gen_ret(const ir::block::ret &ret) {
    if (ret.val.has_value()) {
        const auto& var = ret.val.value();

        if (std::holds_alternative<ir::rvalue>(var.val)) {
            const auto& variable = std::get<ir::rvalue>(var.val);

            output.get() << std::format("   mov rax, {}", variable.name) << std::endl;
        }
    }

    output.get() << "   leave" << std::endl;
    output.get() << "   ret" << std::endl;
}