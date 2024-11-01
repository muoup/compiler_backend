#pragma once

#include <span>
#include <unordered_map>

#include "registers.hpp"
#include "valuegen.hpp"

#include "../../ir/nodes.hpp"
#include "asmgen/asm_nodes.hpp"

namespace ir {
    struct root;

    namespace global {
        struct global_string;
        struct extern_function;
    }

    namespace block {
        struct block_instruction;
    }
}

namespace backend::as {
    struct label;
}

namespace backend::codegen {
    struct instruction_return;
    struct vptr;

    struct function_context {
        const ir::root& root;
        const ir::value_size return_type;

        std::ostream& ostream;
        std::vector<backend::as::label> asm_blocks;

        backend::as::label *current_label;

        const ir::global::function &current_function;
        const backend::instruction_metadata *current_instruction;

        std::unordered_map<std::string, virtual_pointer> value_map;
        std::unordered_map<std::string, virtual_pointer> dropped_at_map;

        std::vector<register_t> dropped_available;

        bool dropped_reassignable = true;
        bool register_tampered[register_count] {};
        bool register_parameter[register_count] {};

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

        void remove_ownership(const vptr* value) {
            if (auto *reg_storage = dynamic_cast<const register_storage*>(value)) {
                register_mem[reg_storage->reg] = nullptr;
            }
        }

        const vptr* get_value(const char* name) {
            if (value_map.contains(name)) {
                return value_map.at(name).get();
            }

            for (auto &str : root.global_strings) {
                if (str.name == name) {
                    value_map[name] = std::make_unique<global_pointer>(str.name);
                    return value_map.at(name).get();
                }
            }

            throw std::runtime_error("Value not found");
        }

        const vptr* get_value(const ir::variable &var) {
            auto ptr = get_value(var.name.c_str());

            debug::assert(ptr->size == var.size, "Variable size mismatch");

            return ptr;
        }

        const vptr* get_value(const ir::int_literal &var) {
            auto ptr = get_value(std::to_string(var.value).c_str());

            debug::assert(ptr->size == var.size, "Variable size mismatch");

            return ptr;
        }

        const vptr* get_value(const ir::value &value) {
            return std::visit([&](auto &&arg) { return get_value(arg); }, value.val);
        }

        void map_value(const char* name, virtual_pointer value) {
            give_ownership(value.get(), name);
            value_map[std::string { name }] = std::move(value);
        }

        void drop_value(const char* name) {
            auto &value = value_map.at(name);

            if (!value->droppable) return;
            if (dynamic_cast<const register_storage*>(value.get())) {
                dropped_available.emplace_back(dynamic_cast<const register_storage*>(value.get())->reg);
            }

            remove_ownership(value.get());
            dropped_at_map.emplace(name, std::move(value_map.at(name)));
            value_map.erase(name);
        }

        void remap_value(const char* name, virtual_pointer value) {
            remove_ownership(value_map[name].get());
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
    };

    void generate(const ir::root& root, std::ostream& ostream);
    void gen_function(const ir::root& root, std::ostream &ostream, const ir::global::function &function);

    instruction_return gen_instruction(backend::codegen::function_context &context, const ir::block::block_instruction &instruction);
}
