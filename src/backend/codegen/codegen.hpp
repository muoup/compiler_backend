#pragma once

#include <span>
#include <unordered_map>
#include <functional>

#include "registers.hpp"
#include "valuegen.hpp"

#include "asmgen/asm_nodes.hpp"
#include "../../ir/node_prototypes.hpp"
#include "../ir_analyzer/node_metadata.hpp"

namespace backend::codegen {
    struct instruction_return;
    struct vptr;

    struct value_reference {
        std::variant<const vptr*, ir::int_literal> value;

        explicit value_reference(const vptr* value) : value(value) {}
        explicit value_reference(ir::int_literal literal) : value(literal) {}

        [[nodiscard]] ir::value_size get_size() const {
            return std::visit([](auto &&arg) -> ir::value_size {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, const vptr*>) {
                    return arg->size;
                } else {
                    return arg.size;
                }
            }, value);
        }

        [[nodiscard]] auto gen_operand() const {
            return std::visit([](auto &&arg) { return as::create_operand(arg); }, value);
        }

        [[nodiscard]] auto gen_address() {
            auto operand = gen_operand();
            operand->address = true;
            return operand;
        }

        [[nodiscard]] bool is_variable() const {
            return std::holds_alternative<const vptr*>(value);
        }

        [[nodiscard]] bool is_literal() const {
            return std::holds_alternative<ir::int_literal>(value);
        }

        void with_variable(const std::function<void(const vptr*)>& func) const {
            auto *ptr = std::get_if<const vptr*>(&value);

            if (ptr) {
                func(*ptr);
            }
        }

        [[nodiscard]] const vptr* get_variable() const {
            const auto *ptr = std::get_if<const vptr*>(&value);

            return ptr ? *ptr : nullptr;
        }

        [[nodiscard]] std::optional<ir::int_literal> get_literal() const {
            const auto *ptr = std::get_if<ir::int_literal>(&value);

            return ptr ? std::make_optional(*ptr) : std::nullopt;
        }

        template <typename T>
        [[nodiscard]] const T* get_vptr_type() const {
            return dynamic_cast<const T*>(get_variable());
        }
    };

    struct function_context {
        const ir::value_size return_type;

        std::ostream& ostream;
        std::vector<backend::as::label> asm_blocks;

        backend::as::label *current_label;

        const backend::md::instruction_metadata *current_instruction;

        std::unordered_map<std::string, virtual_pointer> value_map;
        std::unordered_map<int64_t, virtual_pointer> literal_cache;

        std::vector<register_t> dropped_available;

        bool register_tampered[register_count] {};
        bool register_is_param[register_count] {};

        const char* register_mem[register_count] {};

        size_t current_stack_size = 0;

        template <typename T, typename... Args>
        void add_asm_node(Args... constructor_args) {
            this->current_label->nodes.emplace_back(std::make_unique<T>(std::move(constructor_args)...));
        }

        void give_ownership(const vptr* value, const char* name) {
            if (auto *reg_storage = dynamic_cast<const register_storage*>(value)) {
                register_mem[reg_storage->reg] = name;
            }
        }

        void remove_ownership(const vptr* value, const char* name) {
            if (auto *reg_storage = dynamic_cast<const register_storage*>(value)) {
                // Another variable has already taken ownership of this register
                if (register_mem[reg_storage->reg] != name) return;

                register_mem[reg_storage->reg] = nullptr;
            }
        }

        const vptr* get_value(const char* name) {
            return value_map.at(name).get();
        }

        value_reference get_value(const ir::variable &var) {
            auto ptr = value_map.at(var.name).get();

            return value_reference { ptr };
        }

        value_reference get_value(const ir::int_literal &var) {
            return value_reference { var };
        }

        value_reference get_value(const ir::value &value) {
            return std::visit([&](auto &&arg) { return get_value(arg); }, value.val);
        }

        void map_value(const ir::variable &var, virtual_pointer value) {
            give_ownership(value.get(), var.name.c_str());
            value_map[var.name] = std::move(value);
        }

        void drop_value(const ir::variable &var) {
            auto value = get_value(var);

            if (!value.get_variable()->droppable) return;

            remove_ownership(value.get_variable(), var.name.c_str());
            value_map.erase(var.name);
        }

        void remap_value(const char* name, virtual_pointer value) {
            remove_ownership(value_map[name].get(), name);
            give_ownership(value.get(), name);
            value_map[name] = std::move(value);
        }

        bool has_value(std::string_view name) const {
            return value_map.contains(std::string { name });
        }

        vptr* get_value(std::string_view name) const {
            return value_map.at(std::string { name }).get();
        }

        int64_t find_block(std::string_view name) {
            for (int64_t i = 0; i < (int64_t) asm_blocks.size(); i++) {
                if (asm_blocks[i].name == name) {
                    return i;
                }
            }

            throw std::runtime_error("Block not found");
        }

        bool dropped_reassignable() {
            return current_instruction->instruction.inst->dropped_reassignable();
        }
    };

    void generate(const ir::root& root, std::ostream& ostream);
    void gen_function(const ir::root& root, std::ostream &ostream, const ir::global::function &function);

    instruction_return gen_instruction(backend::codegen::function_context &context, const ir::block::block_instruction &instruction);
}
