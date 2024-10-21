#pragma once

#include "valuegen.hpp"
#include <cstdint>

namespace backend::codegen {
    enum register_t : uint8_t;

    void empty_value(backend::codegen::function_context &context, const char* value);
    void move_to_register(backend::codegen::function_context &context, std::string_view value, backend::codegen::register_t reg);

    const vptr* empty_register(backend::codegen::function_context &context, backend::codegen::register_t reg);

    backend::codegen::virtual_pointer find_memory(backend::codegen::function_context &context, size_t size);
}