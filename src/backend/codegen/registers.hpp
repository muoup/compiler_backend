#pragma once

#include <cstdint>
#include <cstdlib>

#include <string>
#include "../../ir/nodes.hpp"

namespace backend::context {
    struct function_context;

    enum register_t : uint8_t {
        rax, rbx, rcx, rdx,
        rsi, rdi,
        r8, r9, r10, r11, r12,
        r13, r14, r15, r16,

        // Not to be used for regular storage
        rsp, rbp
    };

    constexpr size_t register_count = register_t::r16 + 1;

    enum register_size : uint8_t {
        byte,
        word,
        dword,
        qword
    };

    extern const char* register_name[register_count + 2][4];

    const char * register_as_string(backend::context::register_t reg, ir::value_size size);

    register_t  param_register(uint8_t index);
    const char * param_register_string(uint8_t index, ir::value_size size);
}
