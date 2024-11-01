#include "dataflow.hpp"
#include "codegen.hpp"
#include "inst_output.hpp"

void backend::codegen::empty_value(backend::codegen::function_context &context, const char *value) {
    auto &vmem = context.value_map.at(value);
    auto new_memory = backend::codegen::find_val_storage(context, vmem->size);

//    context.unmap_to_temp(value);
    context.map_value(value, std::move(new_memory));
    backend::codegen::emit_move(context, "temp", value);
}

void backend::codegen::copy_to_register(backend::codegen::function_context &context,
                                        std::string_view value,
                                        backend::codegen::register_t reg) {
    auto val = context.get_value(value);
    auto new_memory = std::make_unique<backend::codegen::register_storage>(val->size, reg);

    if (auto *val_reg = dynamic_cast<backend::codegen::register_storage*>(val); val_reg && val_reg->reg != reg) {
        backend::codegen::empty_register(context, reg);
        backend::codegen::emit_move(context, new_memory.get(), value);
    }
}

const backend::codegen::vptr* backend::codegen::empty_register(backend::codegen::function_context &context, backend::codegen::register_t reg) {
    auto *reg_storage = context.register_mem[reg];

    if (!reg_storage)
        return nullptr;

    auto *old_mem = context.get_value(reg_storage);
    auto new_mem = backend::codegen::find_val_storage(context, old_mem->size);

    backend::codegen::emit_move(context, new_mem.get(), reg_storage);
    context.remap_value(reg_storage, std::move(new_mem));
    return context.value_map.at(reg_storage).get();
}

backend::codegen::virtual_pointer backend::codegen::find_val_storage(backend::codegen::function_context &context, ir::value_size size) {
    auto reg = backend::codegen::find_register(context, size);

    if (reg)
        return reg;

    return backend::codegen::stack_allocate(context, size);
}