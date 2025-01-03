#include <iostream>
#include "value_reference.hpp"
#include "function_context.hpp"

using namespace backend::context;

ir::value_size value_reference::get_size() const {
    if (is_variable())
        return (*get_vmem())->size;
    else
        return get_literal()->size;
}

[[nodiscard]] std::unique_ptr<backend::as::op::operand_t>
value_reference::gen_operand(ir::value_size size) const {
    if (is_variable())
        return as::create_operand(*get_vmem(), size);
    else
        return as::create_operand(std::get<ir::int_literal>(value));
}

std::unique_ptr<backend::as::op::operand_t>
value_reference::gen_operand() const {
    return gen_operand(get_size());
}

std::unique_ptr<backend::as::op::operand_t>
value_reference::gen_address() const {
    auto operand = gen_operand();
    operand->address = true;
    return operand;
}

std::optional<std::string_view> value_reference::get_name() const {
    if (is_variable())
        return std::get<std::string>(value);

    return std::nullopt;
}

const std::string &value_reference::get_name_ref() const {
    return std::get<std::string>(value);
}

[[nodiscard]] std::optional<virtual_memory *> value_reference::get_vmem() const {
    if (!is_variable())
        return std::nullopt;

    if (context.storage.value_map.contains(std::get<std::string>(value)))
        return context.storage.value_map.at(std::get<std::string>(value));

    // look for a global string with the same name
    for (const auto &global_string : context.global_strings) {
        if (global_string->name == std::get<std::string>(value))
            return global_string.get();
    }

    std::cerr << "Variable " << std::get<std::string>(value) << " not found in value map\n";
    return std::nullopt;
}

std::optional<backend::context::register_t>
value_reference::get_register() const {
    if (auto vmem = get_vmem()) {
        if (auto reg = dynamic_cast<const register_storage *>(*vmem); reg)
            return reg->reg;
    }

    return std::nullopt;
}
