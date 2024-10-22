#pragma once

#include <span>
#include <unordered_map>

#include "registers.hpp"
#include "valuegen.hpp"

#include "asmgen/asm_nodes.hpp"
#include "../../ir/node_prototypes.hpp"
#include "../ir_analyzer/node_metadata.hpp"

namespace backend::codegen {
    struct instruction_return;
    struct vptr;

    struct function_context {
        const ir::root& root;

        std::ostream& ostream;
        std::vector<backend::as::label> asm_blocks;

        backend::as::label *current_label;

        const ir::global::function &current_function;
        const backend::instruction_metadata *current_instruction;

        std::unordered_map<std::string, virtual_pointer> value_map;
        std::unordered_map<std::string, virtual_pointer> dropped_at_map;

        std::vector<register_t> dropped_available;

        bool dropped_reassignable = true;
        bool used_register[register_count] {};
        bool register_tampered[register_count] {};

        size_t current_stack_size = 0;

        template <typename T, typename... Args>
        void add_asm_node(Args... constructor_args) {
            this->current_label->nodes.emplace_back(std::make_unique<T>(std::move(constructor_args)...));
        }

        void set_used(const vptr* value, bool used) {
            if (auto *reg_storage = dynamic_cast<const register_storage*>(value)) {
                used_register[reg_storage->reg] = used;
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

        void map_value(const char* name, virtual_pointer value) {
            set_used(value.get(), true);
            value_map[std::string { name }] = std::move(value);
        }

        void drop_value(const char* name) {
            auto &value = value_map.at(name);

            if (!value->droppable) return;
            if (dynamic_cast<const register_storage*>(value.get())) {
                dropped_available.emplace_back(dynamic_cast<const register_storage*>(value.get())->reg);
            }

            set_used(value_map.at(name).get(), false);
            dropped_at_map.emplace(name, std::move(value_map.at(name)));
            value_map.erase(name);
        }

        void remap_value(const char* name, virtual_pointer value) {
            set_used(value_map[name].get(), false);
            set_used(value.get(), true);
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
