#include <sstream>
#include "codegen.hpp"
#include "instructions.hpp"

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
    ostream << "global " << function.name << '\n';
    ostream << function.name << ':' << '\n';

    std::stringstream ss;

    backend::codegen::function_context context {
        ss
    };

    for (const auto &block : function.blocks) {
        for (const auto &instruction : block.instructions) {
            gen_instruction(context, instruction);
        }
    }

    if (context.rsp_off != 0) {
        ostream << "push rbp" << '\n';
        ostream << "mov rbp, rsp" << '\n';
        ostream << "sub rsp, " << context.rsp_off << '\n';
    }

    ostream << ss.str();
}

void backend::codegen::gen_instruction(backend::codegen::function_context &context, const ir::block::block_instruction &instruction) {
    if (auto *allocate = dynamic_cast<ir::block::allocate*>(instruction.inst.get())) {
        gen_allocate(context, *allocate);
    }
}