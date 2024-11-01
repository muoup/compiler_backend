#pragma once

#include <string_view>

namespace backend::codegen {
    struct function_context;
    struct vptr;
}

namespace backend::as {
    void emit_move(
        backend::codegen::function_context &context,
        std::string_view dest,
        std::string_view src,
        size_t size
    );

    void emit_move(
        backend::codegen::function_context &context,
        const backend::codegen::vptr* dest,
        std::string_view src,
        size_t size
    );
}
