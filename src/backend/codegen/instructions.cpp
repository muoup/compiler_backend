#include <sstream>
#include "instructions.hpp"
#include "codegen.hpp"
#include "valuegen.hpp"

std::optional<backend::codegen::instruction_return> backend::codegen::gen_allocate(
    backend::codegen::function_context &context,
    const ir::block::allocate &allocate
) {
    return backend::codegen::instruction_return {
        .value = backend::codegen::stack_allocate(context, allocate.size)
    };
}