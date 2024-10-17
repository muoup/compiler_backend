#include "valuegen.hpp"
#include "codegen.hpp"

#include "../ir_analyzer/node_metadata.hpp"

#include <sstream>

backend::codegen::virtual_pointer backend::codegen::stack_allocate(backend::codegen::function_context &context, size_t size) {
    context.current_stack_size += size;

    return std::make_unique<stack_pointer>(context.current_stack_size);
}

backend::codegen::virtual_pointer backend::codegen::request_register(backend::codegen::function_context &context) {
    // First check if any registers are being dropped
    const auto& dropped_data = context.current_instruction->dropped_data;

    for (size_t i = 0; i < dropped_data.size(); i++) {
        if (!dropped_data[i]) continue;

        const auto& operand = context.current_instruction->instruction.operands[i];

        if (!std::holds_alternative<ir::variable>(operand.val)) continue;

        const auto &variable = std::get<ir::variable>(operand.val);
        auto &mapped_val = context.value_map.at(variable.name);
        auto *reg_storage = dynamic_cast<backend::codegen::register_storage*>(mapped_val.get());

        if (!reg_storage) continue;

        return std::make_unique<backend::codegen::register_storage>(reg_storage->reg);
    }

    // Then check for dropped parameter registers
    const auto param_count = context.current_function.parameters.size();

    for (size_t i = 0; i < param_count; i++) {
        const auto reg = backend::codegen::param_register(i);

        if (context.used_register[reg]) continue;

        return std::make_unique<backend::codegen::register_storage>(reg);
    }

    throw std::runtime_error("unimplemented!");
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
            return "QUAD";
    }

    throw std::runtime_error("unsupported size type");
}