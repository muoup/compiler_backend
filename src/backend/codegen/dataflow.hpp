#pragma once

#include "valuegen.hpp"
#include "codegen.hpp"
#include <cstdint>

namespace backend::codegen {
    enum register_t : uint8_t;

    const vptr* empty_register(backend::codegen::function_context &context, backend::codegen::register_t reg);
    void copy_to_register(backend::codegen::function_context &context, const ir::value &value, backend::codegen::register_t reg);

    backend::codegen::virtual_pointer find_val_storage(backend::codegen::function_context &context, ir::value_size size);
}