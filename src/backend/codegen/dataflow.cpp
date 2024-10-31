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

void backend::codegen::move_to_register(backend::codegen::function_context &context,
                                        std::string_view value,
                                        backend::codegen::register_t reg) {
    if (context.used_register[reg]) {
        const auto *vmem = context.get_value(value);
        const auto *reg_storage = dynamic_cast<const backend::codegen::register_storage*>(vmem);

        if (reg_storage && reg_storage->reg == reg)
            return;

        backend::codegen::empty_register(context, reg);
    }

    if (!context.has_value(value))
        throw std::runtime_error("Value not found");

    auto val_size = context.get_value(value)->size;
    auto new_memory = std::make_unique<backend::codegen::register_storage>(val_size, reg);

    backend::codegen::emit_move(context, new_memory.get(), value);

    if (!dynamic_cast<backend::codegen::literal*>(context.get_value(value)))
       context.remap_value(value.data(), std::move(new_memory));
}

const backend::codegen::vptr* backend::codegen::empty_register(backend::codegen::function_context &context, backend::codegen::register_t reg) {
    for (auto &[name, vmem] : context.value_map) {
        auto *reg_storage = dynamic_cast<backend::codegen::register_storage*>(vmem.get());

        if (!reg_storage || reg_storage->reg != reg) continue;

        auto new_memory = backend::codegen::find_val_storage(context, vmem->size);
        backend::codegen::emit_move(context, new_memory.get(), name);

        context.remap_value(name.c_str(), std::move(new_memory));
        return context.value_map.at(name).get();
    }

    return nullptr;
}

backend::codegen::virtual_pointer backend::codegen::find_val_storage(backend::codegen::function_context &context, ir::value_size size) {
    auto reg = backend::codegen::find_register(context, size);

    if (reg)
        return reg;

    return backend::codegen::stack_allocate(context, size);
}