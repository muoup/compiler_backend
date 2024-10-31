#include "inst_output.hpp"

#include "codegen.hpp"
#include "valuegen.hpp"
#include "asmgen/asm_nodes.hpp"

void backend::codegen::emit_move(function_context &context,
                                 const std::string_view dest_name,
                                 const std::string_view src_name) {
    const auto *dest = context.get_value(dest_name);

    emit_move(context, dest, src_name);
}

void backend::codegen::emit_move(function_context &context,
                                 const vptr *dest,
                                 const std::string_view src_name) {
    const auto *src = context.get_value(src_name);

    debug::assert(dest->size == src->size, "Move sizes must match");

    const auto *dest_stack = dynamic_cast<const backend::codegen::stack_value*>(dest);
    const auto *src_stack = dynamic_cast<const backend::codegen::stack_value*>(src);

    if (!dest_stack || !src_stack) {
        context.add_asm_node<as::inst::mov>(
            as::create_operand(dest),
            as::create_operand(src)
        );
        return;
    }

    // If both are stack pointers, we need to find an intermediate register to move the value
    auto reg = backend::codegen::force_find_register(context, dest->size);

    context.add_asm_node<as::inst::mov>(
        as::create_operand(reg.get()),
        as::create_operand(src)
    );

    context.add_asm_node<as::inst::mov>(
        as::create_operand(dest),
        as::create_operand(reg.get())
    );
}