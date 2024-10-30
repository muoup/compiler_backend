#include "asm_interface.hpp"

#include "../codegen.hpp"
#include "asm_nodes.hpp"

void backend::as::emit_move(
    backend::codegen::function_context &context,
    const std::string_view dest_name,
    const std::string_view src_name,
    const size_t size
) {
    const auto *dest = context.get_value(dest_name);

    emit_move(context, dest, src_name, size);
}

void backend::as::emit_move(
    backend::codegen::function_context &context,
    const backend::codegen::vptr *dest,
    const std::string_view src_name,
    size_t size
) {
    const auto *src = context.get_value(src_name);

    const auto *dest_stack = dynamic_cast<const backend::codegen::stack_pointer*>(dest);
    const auto *src_stack = dynamic_cast<const backend::codegen::stack_pointer*>(src);

    if (!dest_stack || !src_stack) {
        context.add_asm_node<as::inst::mov>(
                as::create_operand(dest, 8),
                as::create_operand(src, 8)
        );
        return;
    }

    // If both are stack pointers, we need to find an intermediate register to move the value

    auto reg = backend::codegen::force_find_register(context);

    context.add_asm_node<as::inst::mov>(
            as::create_operand(reg.get(), 8),
            as::create_operand(src, 8)
    );

    context.add_asm_node<as::inst::mov>(
            as::create_operand(dest, 8),
            as::create_operand(reg.get(), 8)
    );
}