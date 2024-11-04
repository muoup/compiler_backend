#include "dataflow.hpp"
#include "codegen.hpp"
#include "inst_output.hpp"

void backend::codegen::copy_to_register(backend::codegen::function_context &context,
                                        const ir::value &value,
                                        backend::codegen::register_t reg) {
    auto *val_reg = context.get_value(value).get_vptr_type<register_storage>();

    if (val_reg == nullptr || val_reg->reg != reg || !context.current_instruction->dropped_data[0])
        backend::codegen::empty_register(context, reg);

    auto val = context.get_value(value);
    auto new_memory = std::make_unique<backend::codegen::register_storage>(val.get_size(), reg);

    backend::codegen::emit_move(context, new_memory.get(), value);
}

const backend::codegen::vptr* backend::codegen::empty_register(backend::codegen::function_context &context,
                                                               backend::codegen::register_t reg) {
    auto *reg_storage = context.register_mem[reg];

    if (!reg_storage)
        return nullptr;

    auto *old_mem = context.get_value(reg_storage);
    auto new_mem = backend::codegen::find_val_storage(context, old_mem->size);

    backend::codegen::emit_move(context, new_mem.get(), context.get_value(reg_storage));
    context.remap_value(reg_storage, std::move(new_mem));
    return context.value_map.at(reg_storage).get();
}

backend::codegen::virtual_pointer backend::codegen::find_val_storage(backend::codegen::function_context &context, ir::value_size size) {
    auto reg = backend::codegen::find_register(context, size);

    if (reg)
        return reg;

    return backend::codegen::stack_allocate(context, size);
}