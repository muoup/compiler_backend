#include "valuegen.hpp"
#include "codegen.hpp"

#include "../../ir/nodes.hpp"

#include <sstream>

backend::codegen::virtual_pointer backend::codegen::stack_allocate(backend::codegen::function_context &context, size_t size) {
    context.current_stack_size += size;

    return std::make_unique<stack_pointer>(context.current_stack_size, size);
}

backend::codegen::virtual_pointer backend::codegen::find_register(backend::codegen::function_context &context) {
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
            return std::make_unique<backend::codegen::register_storage>(reg_storage->reg);
        }
    }

    // Then check for dropped parameter registers
    const auto param_count = context.current_function.parameters.size();

    for (size_t i = 0; i < param_count; i++) {
        const auto reg = backend::codegen::param_register(i);

        if (context.used_register[reg]) continue;

        context.used_register[reg] = true;
        return std::make_unique<backend::codegen::register_storage>(reg);
    }

    // Otherwise check to see if any registers can be taken temporarily
    // i = 1 as rax should not be tampered with
    for (size_t i = 1; i < register_count; i++) {
        if (context.used_register[i]) continue;

        context.register_tampered[i] = true;

        context.used_register[i] = true;
        return std::make_unique<backend::codegen::register_storage>((register_t) i);
    }

    return nullptr;
}

std::string backend::codegen::get_stack_prefix(size_t size) {
    switch (size) {
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