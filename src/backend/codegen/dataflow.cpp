#include "dataflow.hpp"
#include "codegen.hpp"
#include "inst_output.hpp"

void backend::codegen::empty_value(backend::codegen::function_context &context, const char *value) {
    auto &vmem = context.value_map.at(value);

    if (vmem->get_size() > 8)
        throw std::runtime_error("Cannot empty a pointer larger than 8 bytes");

    auto new_memory = backend::codegen::find_memory(context, vmem->get_size());

    backend::codegen::emit_move(context, new_memory.get(), vmem.get(), 8);

    context.remap_value(value, std::move(new_memory));
}

const backend::codegen::vptr* backend::codegen::move_to_register(backend::codegen::function_context &context,
                                                                  const backend::codegen::vptr *vmem,
                                                                  backend::codegen::register_t reg) {
    if (context.used_register[reg]) {
        const auto *reg_storage = dynamic_cast<const backend::codegen::register_storage*>(vmem);

        if (reg_storage && reg_storage->reg == reg) {
            return vmem;
        }

        backend::codegen::empty_register(context, reg);
    }

    auto new_memory = std::make_unique<backend::codegen::register_storage>(reg);

    for (auto &[name, vptr] : context.value_map) {
        if (vmem != vptr.get()) continue;

        backend::codegen::emit_move(context, new_memory.get(), vmem, 8);
        context.remap_value(name.c_str(), std::move(new_memory));
        break;
    }

    auto *literal = dynamic_cast<const backend::codegen::literal*>(vmem);

    if (!literal)
        throw std::runtime_error("Unknown value!");

    backend::codegen::emit_move(context, new_memory.get(), literal, 8);
    return nullptr;
}

void backend::codegen::move_from_register(backend::codegen::function_context &context,
                                          backend::codegen::register_t reg,
                                          const backend::codegen::vptr *vmem) {
    context.ostream << "    mov     " << vmem->get_address(vmem->get_size()) << ", " << backend::codegen::register_as_string(reg, vmem->get_size()) << '\n';
}

const backend::codegen::vptr* backend::codegen::empty_register(backend::codegen::function_context &context, backend::codegen::register_t reg) {
    for (auto &[name, vmem] : context.value_map) {
        auto *reg_storage = dynamic_cast<backend::codegen::register_storage*>(vmem.get());

        if (!reg_storage || reg_storage->reg != reg) continue;

        auto new_memory = backend::codegen::find_memory(context, vmem->get_size());
        backend::codegen::emit_move(context, new_memory.get(), vmem.get(), 8);

        context.remap_value(name.c_str(), std::move(new_memory));
        return context.value_map.at(name).get();
    }

    return nullptr;
}

backend::codegen::virtual_pointer backend::codegen::pop_register(backend::codegen::function_context &context,
                                                                 backend::codegen::register_t reg) {
    for (auto &[name, vmem] : context.value_map) {
        auto *reg_storage = dynamic_cast<backend::codegen::register_storage*>(vmem.get());

        if (!reg_storage || reg_storage->reg != reg) continue;

        auto new_memory = backend::codegen::find_memory(context, vmem->get_size());
        backend::codegen::emit_move(context, new_memory.get(), vmem.get(), 8);

        return new_memory;
    }

    throw std::runtime_error("No value in register");
}

backend::codegen::virtual_pointer backend::codegen::find_memory(backend::codegen::function_context &context, size_t size) {
    if (size > 8)
        return backend::codegen::stack_allocate(context, size);

    auto reg = backend::codegen::find_register(context);

    if (reg)
        return reg;

    return backend::codegen::stack_allocate(context, size);
}