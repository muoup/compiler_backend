#pragma once

#include <string>
#include <optional>
#include <variant>

#include "../../../ir/nodes.hpp"
#include "../../ir_analyzer/node_metadata.hpp"
#include "../asmgen/asm_nodes.hpp"

namespace backend::context {
    struct function_context;

    struct value_reference {
        using var_type = std::string;
        using literal_type = ir::int_literal;

        function_context &context;
        std::variant<var_type, literal_type> value;

        explicit value_reference(function_context &context, var_type var) : context(context), value(var) {}
        explicit value_reference(function_context &context, literal_type literal) : context(context), value(literal) {}

        [[nodiscard]] ir::value_size get_size() const;

        [[nodiscard]] std::unique_ptr<as::op::operand_t> gen_operand(ir::value_size size) const;

        [[nodiscard]] std::unique_ptr<as::op::operand_t> gen_operand() const;

        [[nodiscard]] std::unique_ptr<as::op::operand_t> gen_address() const;

        [[nodiscard]] bool is_variable() const {
            return std::holds_alternative<std::string>(value);
        }

        [[nodiscard]] bool is_literal() const {
            return std::holds_alternative<ir::int_literal>(value);
        }

        [[nodiscard]] std::optional<std::string_view> get_name() const;

        [[nodiscard]] const std::string& get_name_ref() const;

        [[nodiscard]] std::optional<virtual_memory *> get_vmem() const;

        [[nodiscard]] std::optional<ir::int_literal> get_literal() const {
            const auto *ptr = std::get_if<literal_type>(&value);

            return ptr ? std::make_optional(*ptr) : std::nullopt;
        }

        [[nodiscard]] std::optional<context::register_t> get_register() const;

        template <typename T>
        [[nodiscard]] T *get_vptr_type() const {
            return dynamic_cast<T*>(*get_vmem());
        }
    };
}
