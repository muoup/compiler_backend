#pragma once

#include "valuegen.hpp"
#include <cstdint>

namespace backend::codegen {
    enum register_t : uint8_t;

    void empty_value(backend::codegen::function_context &context, const char* value);
    const vptr *
    move_to_register(backend::codegen::function_context &context, const backend::codegen::vptr *vmem, backend::codegen::register_t reg);
    void move_from_register(backend::codegen::function_context &context, backend::codegen::register_t reg, const backend::codegen::vptr *vmem);

    const vptr* empty_register(backend::codegen::function_context &context, backend::codegen::register_t reg);
    virtual_pointer pop_register(backend::codegen::function_context &context, register_t reg);

    backend::codegen::virtual_pointer find_memory(backend::codegen::function_context &context, size_t size);
}