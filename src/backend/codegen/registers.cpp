#include "registers.hpp"

using namespace backend;

const char* codegen::register_name[register_count][4] = {
        { "al", "ax", "eax", "rax" },
        { "bl", "bx", "ebx", "rbx" },
        { "cl", "cx", "ecx", "rcx" },
        { "dl", "dx", "edx", "rdx" },
        { "sil", "si", "esi", "rsi" },
        { "dil", "di", "edi", "rdi" },
        { "bpl", "bp", "ebp", "rbp" },
        { "spl", "sp", "esp", "rsp" },
        { "r8b", "r8w", "r8d", "r8" },
        { "r9b", "r9w", "r9d", "r9" },
        { "r10b", "r10w", "r10d", "r10" },
        { "r11b", "r11w", "r11d", "r11" },
        { "r12b", "r12w", "r12d", "r12" },
        { "r13b", "r13w", "r13d", "r13" },
        { "r14b", "r14w", "r14d", "r14" },
        { "r15b", "r15w", "r15d", "r15" },
        { "r16b", "r16w", "r16d", "r16" }
};

codegen::register_t codegen::param_register(uint8_t index) {
    const static register_t call_registers[] = {
            rdi,
            rsi,
            rdx
    };

    return call_registers[index];
}

std::string codegen::param_register_string(uint8_t index, size_t size) {
    return register_as_string(param_register(index), size);
}

std::string codegen::register_as_string(backend::codegen::register_t reg, size_t size) {
    return register_name[reg][size - 1];
}