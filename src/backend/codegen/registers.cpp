#include <stdexcept>
#include "registers.hpp"

using namespace backend;

const char* codegen::register_name[register_count + 2][4] = {
        { "al", "ax", "eax", "rax" },
        { "bl", "bx", "ebx", "rbx" },
        { "cl", "cx", "ecx", "rcx" },
        { "dl", "dx", "edx", "rdx" },
        { "sil", "si", "esi", "rsi" },
        { "dil", "di", "edi", "rdi" },
        { "r8b", "r8w", "r8d", "r8" },
        { "r9b", "r9w", "r9d", "r9" },
        { "r10b", "r10w", "r10d", "r10" },
        { "r11b", "r11w", "r11d", "r11" },
        { "r12b", "r12w", "r12d", "r12" },
        { "r13b", "r13w", "r13d", "r13" },
        { "r14b", "r14w", "r14d", "r14" },
        { "r15b", "r15w", "r15d", "r15" },
        { "r16b", "r16w", "r16d", "r16" },
        { "rsp", "rsp", "rsp", "rsp" },
        { "rbp", "rbp", "rbp", "rbp" }
};

codegen::register_t codegen::param_register(uint8_t index) {
    const static register_t call_registers[] = {
            rdi,
            rsi,
            rdx
    };

    return call_registers[index];
}

const char * codegen::param_register_string(uint8_t index, ir::value_size size) {
    return register_as_string(param_register(index), size);
}

const char * codegen::register_as_string(backend::codegen::register_t reg, ir::value_size size) {
    switch (ir::size_in_bytes(size)) {
        case 1:
            return register_name[reg][0];
        case 2:
            return register_name[reg][1];
        case 4:
            return register_name[reg][2];
        case 8:
            return register_name[reg][3];
        default:
            throw std::runtime_error("invalid register size");
    }
}