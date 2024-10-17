#pragma once

#include <string>
#include <optional>
#include "../../ir/nodes.hpp"

namespace ir::block {
    struct allocate;
}

namespace backend::codegen {
    struct function_context;

    struct instruction_return {
        std::string value;
    };

    std::optional<instruction_return> gen_allocate(backend::codegen::function_context &context, const ir::block::allocate &allocate);
    std::optional<instruction_return> gen_store(backend::codegen::function_context &context, const ir::block::store &store);
}
