#pragma once

#include "codegen.hpp"

namespace backend::codegen {
    void emit_move(function_context &context, const backend::codegen::vptr *dest,
                   const backend::codegen::vptr *src, size_t size);
}