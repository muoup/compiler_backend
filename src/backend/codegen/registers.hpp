#pragma once

#include <cstdint>
#include <cstdlib>

#include <string>

namespace backend::codegen {
    struct function_context;

    constexpr size_t register_count = 17;

    enum register_t : uint8_t {
        rax, rbx, rcx, rdx,
        rsi, rdi,
        rbp, rsp,
        r8, r9, r10, r11, r12,
        r13, r14, r15, r16
    };

    enum register_size : uint8_t {
        byte,
        word,
        dword,
        qword
    };

    extern const char* register_name[register_count][4];

    std::string register_as_string(register_t reg, size_t size);

    register_t  param_register(uint8_t index);
    std::string param_register_string(uint8_t index, size_t size);
}
