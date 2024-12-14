#pragma once

#include <cstdint>
#include "context/function_context.hpp"

namespace backend::codegen {
    void gen_lea(context::function_context &context, const context::virtual_memory *dest, uint64_t m,
                 const context::virtual_memory *x, uint64_t b);

    void gen_lea(context::function_context &context, const context::virtual_memory *dest, uint64_t m,
                 const context::value_reference& x, uint64_t b);

    void gen_lea(context::function_context &context, const context::virtual_memory *dest,
                 const context::value_reference &scaled_reg, uint8_t scale, uint64_t offset,
                 const context::value_reference &unscaled_reg);
}