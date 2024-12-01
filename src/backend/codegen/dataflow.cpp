#include "dataflow.hpp"
#include "codegen.hpp"

#include "context/function_context.hpp"
#include "context/value_reference.hpp"

void backend::context::copy_to_register(backend::context::function_context &context,
                                        const ir::value &value,
                                        backend::context::register_t reg) {
    backend::context::empty_register(context, reg);

    context.add_asm_node<as::inst::mov>(
        as::create_operand(reg, value.get_size()),
        context.storage.get_value(value).gen_operand()
    );
}

backend::context::virtual_memory * backend::context::empty_register(backend::context::function_context &context,
                                                                    backend::context::register_t reg) {
    auto *reg_storage = context.storage.registers[static_cast<size_t>(reg)].get();

    if (!reg_storage->in_use())
        return reg_storage;

    auto old_mem = context.storage.get_value(reg_storage->owner);

    if (old_mem.is_literal())
        return reg_storage;

    auto new_mem = backend::context::find_val_storage(context, old_mem.get_size());

    context.add_asm_node<as::inst::mov>(
        as::create_operand(new_mem),
        old_mem.gen_operand()
    );

    context.storage.remap_value(reg_storage->owner, new_mem);
    return new_mem;
}

backend::context::virtual_memory *
backend::context::find_val_storage(backend::context::function_context &context, ir::value_size size) {
    auto reg = backend::context::find_register(context, size);

    if (reg)
        return reg;

    return backend::context::stack_allocate(context, ir::size_in_bytes(size));
}