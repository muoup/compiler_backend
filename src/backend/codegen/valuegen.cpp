#include "valuegen.hpp"
#include "codegen.hpp"

#include "../ir_analyzer/node_metadata.hpp"
#include "dataflow.hpp"

#include <sstream>

std::unique_ptr<backend::codegen::register_storage>
backend::codegen::force_find_register(backend::codegen::function_context &context, ir::value_size size) {
    if (auto find = backend::codegen::find_register(context, size); find)
        return find;

    backend::codegen::empty_register(context, backend::codegen::register_t::rax);
    return std::make_unique<backend::codegen::register_storage>(size, backend::codegen::register_t::rax);
}

backend::codegen::virtual_pointer backend::codegen::stack_allocate(backend::codegen::function_context &context, ir::value_size size) {
    context.current_stack_size += ir::size_in_bytes(size);

    return std::make_unique<stack_value>(size, context.current_stack_size);
}

std::unique_ptr<backend::codegen::register_storage>
backend::codegen::find_register(backend::codegen::function_context &context, ir::value_size size) {
    // First check if any registers are being dropped
    const auto& dropped_data = context.current_instruction->dropped_data;

    if (context.dropped_reassignable) {
        for (size_t i = 0; i < dropped_data.size(); i++) {
            if (!dropped_data[i]) continue;

            const auto& operand = context.current_instruction->instruction.operands[i];

            if (!std::holds_alternative<ir::variable>(operand.val)) continue;

            const auto &variable = std::get<ir::variable>(operand.val);
            auto &mapped_val = context.value_map.at(variable.name);
            auto *reg_storage = dynamic_cast<backend::codegen::register_storage*>(mapped_val.get());

            if (!reg_storage) continue;

            context.used_register[reg_storage->reg] = true;
            return std::make_unique<backend::codegen::register_storage>(size, reg_storage->reg);
        }
    }

    // Check for dropped registers that are available
    for (auto iter = context.dropped_available.begin(); iter != context.dropped_available.end(); iter++) {
        auto reg = *iter;

        if (context.used_register[reg]) continue;

        context.dropped_available.erase(iter);
        context.used_register[reg] = true;
        return std::make_unique<backend::codegen::register_storage>(size, reg);
    }

    // Otherwise check to see if any registers can be taken temporarily
    // i = 1 as rax should not be tampered with
    for (size_t i = 1; i < register_count; i++) {
        if (context.used_register[i]) continue;

        context.register_tampered[i] = true;

        context.used_register[i] = true;
        return std::make_unique<backend::codegen::register_storage>(size, (register_t) i);
    }

    return nullptr;
}

std::string backend::codegen::get_stack_prefix(ir::value_size size) {
    switch (ir::size_in_bytes(size)) {
        case 1:
            return "BYTE";
        case 2:
            return "WORD";
        case 4:
            return "DWORD";
        case 8:
            return "QWORD";
    }

    throw std::runtime_error("unsupported size type");
}