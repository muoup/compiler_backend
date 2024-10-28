#pragma once

#include <ostream>
#include <utility>
#include <iomanip>

#include "../instructions.hpp"

namespace backend::as {
    enum operand_types : uint8_t {
        literal,
        reg,
        stack_mem,
        global_ptr
    };

    namespace op {
        struct operand_t {
            const operand_types type;
            uint8_t size;

            explicit operand_t(operand_types type, uint8_t size) : type(type), size(size) {}
            virtual ~operand_t() = default;

            [[nodiscard]] virtual std::string get_address() const = 0;
            [[nodiscard]] virtual bool equals(const operand_t& other) = 0;
        };
    }

    namespace inst {
        using operand = std::unique_ptr<op::operand_t>;

        struct asm_node {
            bool is_valid = true;

            virtual ~asm_node() = default;
            virtual void print(backend::codegen::function_context &context) const = 0;
            [[nodiscard]] bool printable() const { return is_valid; }
        };

        struct stack_save : asm_node {
            stack_save() = default;

            ~stack_save() override = default;

            void print(backend::codegen::function_context &context) const override;
        };

        struct mov : asm_node {
            operand op1, op2;

            mov(operand op1, operand op2)
                    : op1(std::move(op1)), op2(std::move(op2)) {
                if (this->op1->equals(*this->op2))
                    is_valid = false;
            }

            ~mov() override = default;

            void print(backend::codegen::function_context &context) const override;
        };

        struct jmp : asm_node {
            std::string label_name;

            explicit jmp(std::string label_name)
                    : label_name(std::move(label_name)) {}

            ~jmp() override = default;

            void print(backend::codegen::function_context &context) const override;
        };

        struct cmp : asm_node {
            operand oper1, oper2;

            cmp(operand oper1, operand oper2)
                    : oper1(std::move(oper1)), oper2(std::move(oper2)) {}

            ~cmp() override = default;

            void print(backend::codegen::function_context &context) const override;
        };

        struct cond_jmp : asm_node {
            ir::block::icmp_type type;
            std::string branch_name;

            cond_jmp(ir::block::icmp_type type, std::string branch_name)
                    : type(type), branch_name(std::move(branch_name)) {}

            ~cond_jmp() override = default;

            void print(backend::codegen::function_context &context) const override;
        };

        struct arithmetic : asm_node {
            ir::block::arithmetic_type type;
            operand oper1, oper2;

            arithmetic(ir::block::arithmetic_type type, operand oper1, operand oper2)
                    : type(type), oper1(std::move(oper1)), oper2(std::move(oper2)) {}

            ~arithmetic() override = default;

            void print(backend::codegen::function_context &context) const override;
        };

        struct call : asm_node {
            std::string function_name;

            explicit call(std::string function_name)
                    : function_name(std::move(function_name)) {}

            ~call() override = default;

            void print(backend::codegen::function_context &context) const override;
        };

        struct ret : asm_node {
            ret() = default;
            ~ret() override = default;

            void print(backend::codegen::function_context &context) const override;
        };
    }

    struct label {
        std::string name;
        std::vector<std::unique_ptr<inst::asm_node>> nodes;
    };

    std::unique_ptr<op::operand_t> create_operand(const codegen::vptr *vptr);
}