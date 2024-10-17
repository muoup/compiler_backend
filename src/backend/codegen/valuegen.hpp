#pragma once

#include <string>

namespace backend::codegen {
    struct function_context;

    std::string stack_allocate(backend::codegen::function_context &context, size_t size);
}