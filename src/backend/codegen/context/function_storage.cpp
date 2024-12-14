#include "function_storage.hpp"

#include "function_context.hpp"
#include "value_reference.hpp"

using namespace backend;

void context::function_storage::remap_value(const std::string& name, backend::context::virtual_memory *value) {
    if (has_value(name)) {
        drop_ownership(name.c_str());
        erase_value(name);
    }

    map_value(name, value);
}

void context::function_storage::map_value(std::string name, virtual_memory *value) {
    value_map[name] = value;

    if (auto *ptr = dynamic_cast<register_storage*>(value))
        ptr->owner = std::move(name);
}

void context::function_storage::map_value(const ir::variable &var, virtual_memory *value) {
    map_value(var.name, value);
}

context::register_storage *context::function_storage::register_ref(backend::context::register_t reg) {
    auto *reg_storage = registers[static_cast<size_t>(reg)].get();
    reg_storage->tampered = true;

    return reg_storage;
}

context::register_storage *context::function_storage::get_register(backend::context::register_t reg, ir::value_size size) {
    auto *reg_storage = register_ref(reg);
    reg_storage->grab(size);

    return reg_storage;
}

void
context::function_storage::claim_temp_register(backend::context::register_t reg, context::value_reference &val) {
    static int temp_counter = 0;

    auto temp_name = std::string { "__temp" + std::to_string(temp_counter++) };

    auto *reg_storage = get_register(reg, val.get_size());
    map_value(temp_name, reg_storage);

    val.value = std::move(temp_name);
}

void context::function_storage::drop_ownership(const char *name) {
    if (!has_value(name))
        return;

    auto value = value_map[name];

    if (auto *reg = dynamic_cast<register_storage*>(value)) {
        reg->unclaim();
    }

    dropped_available.emplace_back(value);
}

void context::function_storage::erase_value(std::string_view name) {
    auto owned = std::string { name };
    auto val = value_map[owned];

    value_map.erase(std::string { name });

    if (auto *reg = dynamic_cast<register_storage*>(val)) {
        reg->unclaim();
    }
}

void context::function_storage::drop_reassignable() {
    for (const auto &var : pending_drop) {
        drop_ownership(var.c_str());
    }
}

void context::function_storage::erase_reassignable() {
    for (const auto &var : pending_drop) {
        erase_value(var);
    }

    pending_drop.clear();
}

context::value_reference context::function_storage::get_value(std::string_view name) {
    return context::value_reference { parent_context, std::string { name } };
}

context::value_reference context::function_storage::get_value(const ir::value &value) {
    return std::visit([&](auto &&arg) { return get_value(arg); }, value.val);
}

context::value_reference context::function_storage::get_value(const ir::variable &var) {
    return get_value(var.name);
}

context::value_reference context::function_storage::get_value(const ir::int_literal &literal) {
    return context::value_reference { parent_context, literal };
}

bool context::function_storage::has_value(std::string_view name) const {
    return value_map.contains(std::string { name });
}

void context::function_storage::ensure_in_register(value_reference &val) {
    if (val.get_register())
        return;

    auto reg = context::force_find_register(parent_context, val.get_size());

    if (val.get_size() == ir::value_size::ptr) {
        parent_context.add_asm_node<as::inst::lea>(
            as::create_operand(reg->reg, ir::value_size::ptr),
            val.gen_address()
        );
    } else {
        parent_context.add_asm_node<as::inst::mov>(
            val.gen_operand(),
            as::create_operand(reg->reg, val.get_size())
        );
    }

    claim_temp_register(reg->reg, val);
}