#include "registers.hpp"

using namespace ir;

registers::register_t registers::param_register(uint8_t index) {
    const static register_t call_registers[] = {
            rdi,
            rsi,
            rdx
    };

    return call_registers[index];
}

const char *registers::param_register_string(uint8_t index) {
    return register_as_string(param_register(index));
}

const char *registers::register_as_string(ir::registers::register_t reg) {
    switch (reg) {
        case rax:   return "rax";
        case rbx:   return "rbx";
        case rcx:   return "rcx";
        case rdx:   return "rdx";

        case rsi:   return "rsi";
        case rdi:   return "rdi";

        case rbp:   return "rbp";
        case rsp:   return "rsp";

        case r8:    return "r8";
        case r9:    return "r9";
        case r10:   return "r10";
        case r11:   return "r11";
        case r12:   return "r12";
        case r13:   return "r13";
        case r14:   return "r14";
        case r15:   return "r15";
        case r16:   return "r16";

        default:    return "ERROR";
    }
}