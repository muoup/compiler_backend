#pragma once

#include <cstdint>

namespace ir::registers {
    enum register_t : uint8_t {
        rax, rbx, rcx, rdx,
        rsi, rdi,
        rbp, rsp,
        r8, r9, r10, r11, r12,
        r13, r14, r15, r16
    };

    const char* register_as_string(register_t reg);

    register_t  param_register(uint8_t index);
    const char* param_register_string(uint8_t index);
}
