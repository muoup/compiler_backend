#include "dataflow.hpp"
#include "codegen.hpp"
#include "asmgen/interface.hpp"

void backend::codegen::copy_to_register(backend::codegen::function_context &context,
                                        const ir::value &value,
                                        backend::codegen::register_t reg) {
    auto *val_reg = context.get_value(value).get_vptr_type<register_storage>();

    if (val_reg == nullptr || val_reg->reg != reg)
        backend::codegen::empty_register(context, reg);

    auto new_memory = std::make_unique<backend::codegen::register_storage>(value.get_size(), reg);

    context.add_asm_node<as::inst::mov>(
        as::create_operand(new_memory.get()),
        context.get_value(value).gen_operand()
    );
}

const backend::codegen::vptr* backend::codegen::empty_register(backend::codegen::function_context &context,
                                                               backend::codegen::register_t reg) {
    auto *reg_storage = context.register_mem[reg];

    if (!reg_storage || !context.has_value(reg_storage))
        return nullptr;

    auto *old_mem = context.get_value(reg_storage);
    auto new_mem = backend::codegen::find_val_storage(context, old_mem->size);

    context.add_asm_node<as::inst::mov>(
        as::create_operand(new_mem.get()),
        as::create_operand(old_mem)
    );
    context.remap_value(reg_storage, std::move(new_mem));
    return context.value_map.at(reg_storage).get();
}

backend::codegen::virtual_pointer backend::codegen::find_val_storage(backend::codegen::function_context &context, ir::value_size size) {
    auto reg = backend::codegen::find_register(context, size);

    if (reg)
        return reg;

    return backend::codegen::stack_allocate(context, size);
}