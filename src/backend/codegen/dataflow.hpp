#pragma once

#include "valuegen.hpp"
#include "codegen.hpp"
#include <cstdint>

namespace backend::context {
    enum register_t : uint8_t;

    virtual_memory * empty_register(backend::context::function_context &context, backend::context::register_t reg);
    void copy_to_register(backend::context::function_context &context, const ir::value &value, backend::context::register_t reg);

    virtual_memory *
    find_val_storage(backend::context::function_context &context, ir::value_size size);
}