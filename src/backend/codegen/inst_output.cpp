#include "inst_output.hpp"

#include "codegen.hpp"
#include "valuegen.hpp"
#include "asmgen/asm_nodes.hpp"

void backend::codegen::emit_move(function_context &context,
                                 const ir::value& dest,
                                 const ir::value& src) {
    auto ref = context.get_value(dest);
    auto dest_vptr = ref.get_variable();

    debug::assert(dest_vptr, "Destination must be a variable");

    emit_move(context, dest_vptr, src);
}

void backend::codegen::emit_move(function_context &context,
                                 const vptr* dest_vptr,
                                 const ir::value &src) {
    const auto &src_ref = context.get_value(src);

    debug::assert(dest_vptr->size == src_ref.get_size(), "Move sizes must match");

    const auto *src_vptr = src_ref.get_variable();

    const auto *src_reg = dynamic_cast<const register_storage*>(src_vptr);
    const auto *dest_reg = dynamic_cast<const register_storage*>(dest_vptr);

    if (src_reg || dest_reg) {
        context.add_asm_node<as::inst::mov>(
            as::create_operand(dest_vptr),
            src_ref.gen_as_operand()
        );

        return;
    }

    // A move requires one operand to be a register, so if not we must employ a temporary register
    auto reg = backend::codegen::force_find_register(context, dest_vptr->size);

    context.add_asm_node<as::inst::mov>(
        as::create_operand(reg.get()),
        src_ref.gen_as_operand()
    );

    context.add_asm_node<as::inst::mov>(
        as::create_operand(dest_vptr),
        as::create_operand(reg.get())
    );
}

void backend::codegen::emit_move(function_context &context,
                                 const vptr* dest,
                                 const vptr* src) {
    context.add_asm_node<as::inst::mov>(
            as::create_operand(dest),
            as::create_operand(src)
    );
}
