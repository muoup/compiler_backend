#pragma once

#include <cstdint>
#include <cstdlib>

#include <string>
#include "../../ir/nodes.hpp"

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

    const char * register_as_string(backend::codegen::register_t reg, ir::value_size size);

    register_t  param_register(uint8_t index);
    const char * param_register_string(uint8_t index, ir::value_size size);
}
