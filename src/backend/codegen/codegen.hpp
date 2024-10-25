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
        std::ostream& ostream;
        std::vector<backend::as::label> asm_blocks;

        backend::as::label *current_label;

        const ir::global::function &current_function;
        const backend::instruction_metadata *current_instruction;

        std::unordered_map<std::string, virtual_pointer> value_map;
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

        void map_value(const char* name, virtual_pointer value) {
            set_used(value.get(), true);
            value_map[std::string { name }] = std::move(value);
        }

        void unmap_value(const char* name) {
            set_used(value_map.at(name).get(), false);
            value_map.erase(name);
        }

        void remap_value(const char* name, virtual_pointer value) {
            set_used(value.get(), true);
            value_map[name] = std::move(value);
            set_used(value.get(), false);
        }

        bool has_value(std::string_view name) const {
            return value_map.contains(std::string { name });
        }

        vptr* get_value(std::string_view name) const {
            return value_map.at(std::string { name }).get();
        }

        void unmap_to_temp(const char* name) {
            auto temp = std::move(value_map.at(name));
            value_map.erase(name);
            value_map["__int__temp"] = std::move(temp);
        }

        void override_temp(virtual_pointer ptr) {
           value_map["__int__temp"] = std::move(ptr);
        }
    };

    void generate(const ir::root& root, std::ostream& ostream);

    void gen_global_strings(std::ostream& ostream, const std::vector<ir::global::global_string> &global_strings);
    void gen_extern_functions(std::ostream& ostream, const std::vector<ir::global::extern_function> &extern_functions);
    void gen_defined_functions(std::ostream &ostream, const std::vector<ir::global::function> &functions);

    void gen_function(std::ostream &ostream, const ir::global::function &function);

    instruction_return gen_instruction(backend::codegen::function_context &context, const ir::block::block_instruction &instruction);
}
