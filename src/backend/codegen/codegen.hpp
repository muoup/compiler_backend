#pragma once

#include <span>
#include <unordered_map>

#include "registers.hpp"
#include "valuegen.hpp"

#include "../../ir/nodes.hpp"

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

namespace backend::codegen {
    struct instruction_return;
    struct vptr;

    struct function_context {
        std::ostream& ostream;

        const ir::global::function &current_function;
        const backend::instruction_metadata *current_instruction;

        std::unordered_map<std::string, virtual_pointer> value_map;

        bool used_register[register_count] {};
        size_t current_stack_size = 0;

        void map_value(const char* name, virtual_pointer value) {
            value_map[name] = std::move(value);

            if (auto reg = dynamic_cast<register_storage*>(value_map[name].get())) {
                used_register[reg->reg] = true;
            }
        }

        void unmap_value(const char* name) {
            if (auto reg = dynamic_cast<register_storage*>(value_map[name].get())) {
                used_register[reg->reg] = false;
            }

            value_map.erase(name);
        }
    };

    void generate(const ir::root& root, std::ostream& ostream);

    static void gen_global_strings(std::ostream& ostream, const std::vector<ir::global::global_string> &global_strings);
    static void gen_extern_functions(std::ostream& ostream, const std::vector<ir::global::extern_function> &extern_functions);
    static void gen_defined_functions(std::ostream &ostream, const std::vector<ir::global::function> &functions);

    static void gen_function(std::ostream &ostream, const ir::global::function &function);

    static instruction_return gen_instruction(backend::codegen::function_context &context, const ir::block::block_instruction &instruction);
}
