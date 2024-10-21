#pragma once

#include "codegen.hpp"

namespace backend::codegen {
    void emit_move(function_context &context,
              std::string_view dest,
              std::string_view src,
              size_t size);

    void emit_move(function_context &context,
               const vptr* dest,
               std::string_view src,
               size_t size);
}