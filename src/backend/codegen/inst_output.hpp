#pragma once

#include "codegen.hpp"

namespace backend::codegen {
    void emit_move(function_context &context,
                   const ir::value& dest,
                   const ir::value& src);

    void emit_move(function_context &context,
                   const vptr* dest,
                   const ir::value& src);

    void emit_move(function_context &context,
                   const vptr* dest,
                   const vptr* src);
}