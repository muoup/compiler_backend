#include "inst_output.hpp"
#include "codegen.hpp"

void backend::codegen::emit_move(function_context &context, const backend::codegen::vptr *dest,
                                 const backend::codegen::vptr *src, size_t size) {
    const auto *dest_stack = dynamic_cast<const backend::codegen::stack_pointer*>(dest);
    const auto *src_stack = dynamic_cast<const backend::codegen::stack_pointer*>(src);

    const auto *dest_lit = dynamic_cast<const backend::codegen::literal*>(dest);
    const auto *src_lit = dynamic_cast<const backend::codegen::literal*>(src);

    if (dest_lit && src_lit)
        throw std::runtime_error("Cannot move between two literals");

    const auto dest_out = dest->get_address(size);
    const auto src_out = src->get_address(size);

    if (dest_out == src_out)
        return;

    if (dest_stack && src_stack) {
        if (context.used_register[backend::codegen::register_t::rax]) {
            context.ostream << "    push    rax\n";
            context.ostream << "    mov     rax, " << src_out << "\n";
            context.ostream << "    mov     " << dest_out << ", rax\n";
            context.ostream << "    pop     rax\n";
        } else {
            context.ostream << "    mov     rax, " << src_out << "\n";
            context.ostream << "    mov     " << dest_out << ", rax\n";
        }

        return;
    }

    context.ostream << "    mov     " << dest_out << ", " << src_out << "\n";
}