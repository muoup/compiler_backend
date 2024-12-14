#include "valuegen.hpp"
#include "dataflow.hpp"
#include "context/function_context.hpp"

#include <sstream>

backend::context::register_storage*
backend::context::force_find_register(backend::context::function_context &context, ir::value_size size) {
    if (auto find = backend::context::find_register(context, size); find)
        return find;

    for (const auto &reg : context.storage.registers) {
        if (reg->frozen)
            continue;

        backend::context::empty_register(context, reg->reg);
        return context.storage.get_register(reg->reg, size);
    }

    throw std::runtime_error("single instruction has somehow used all registers");
}

backend::context::memory_addr *
backend::context::stack_allocate(backend::context::function_context &context, size_t size) {
    context.current_stack_size += size;

    auto addr = std::make_unique<backend::context::memory_addr>(ir::value_size::ptr, -context.current_stack_size);
    auto *addr_ptr = addr.get();

    context.storage.misc_storage.emplace_back(std::move(addr));

    return dynamic_cast<backend::context::memory_addr*>(addr_ptr);
}

backend::context::register_storage *
backend::context::find_register(backend::context::function_context &context, ir::value_size size) {
    // First check if any registers are being dropped, the most recent dropped registers are going
    // to be the operands dropped in the current instruction, so a separate routine for defaulting to
    // those is not needed.
    if (context.auto_drop_reassignable() && !context.dropped_available.empty()) {
        auto reassign = context.dropped_available.back();
        context.dropped_available.pop_back();

        return context.storage.get_register(reassign, size);
    }

    // Otherwise check to see if any registers can be taken temporarily
    // i = 1 as rax should not be tampered with
    for (auto &reg : context.storage.registers) {
        if (reg->in_use()) continue;

        return context.storage.get_register(reg->reg, size);
    }

    return nullptr;
}

std::string backend::context::get_stack_prefix(ir::value_size size) {
    switch (ir::size_in_bytes(size)) {
        case 1:
            return "BYTE ";
        case 2:
            return "WORD ";
        case 4:
            return "DWORD ";
        case 8:
            return "QWORD ";
    }

    throw std::runtime_error("unsupported size type");
}