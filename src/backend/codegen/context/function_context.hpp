#pragma once

#include "../asmgen/asm_nodes.hpp"
#include "../codegen.hpp"
#include "../registers.hpp"
#include "../valuegen.hpp"
#include "function_storage.hpp"

#include <unordered_map>

namespace backend::context {
  struct function_context {
    const ir::value_size return_type;

    std::ostream& ostream;
    std::vector<std::unique_ptr<global_pointer>> &global_strings;
    std::vector<backend::as::label> asm_blocks;

    backend::as::label *current_label;
    const backend::md::instruction_metadata *current_instruction;

    function_storage storage {
        .parent_context = *this
    };

    std::unordered_map<std::string, owned_vmem> value_map;

    std::vector<register_t> dropped_available;

    bool register_is_param[register_count] {};

    size_t current_stack_size = 0;

    template <typename T, typename... Args>
    void add_asm_node(Args... constructor_args) {
        this->current_label->nodes.emplace_back(std::make_unique<T>(std::move(constructor_args)...));
    }

    int64_t find_block(std::string_view name) {
      for (int64_t i = 0; i < (int64_t) asm_blocks.size(); i++) {
        if (asm_blocks[i].name == name) {
          return i;
        }
      }

      throw std::runtime_error("Block not found");
    }

    bool auto_drop_reassignable() const {
      return current_instruction->instruction.inst->auto_drop_reassignable();
    }
  };
}