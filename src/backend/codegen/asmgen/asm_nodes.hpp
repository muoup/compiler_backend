#pragma once

#include <ostream>
#include <utility>
#include <iomanip>

#include "../instructions.hpp"
#include "../../../debug/assert.hpp"

namespace backend::as {
    enum operand_types : uint8_t {
        literal,
        reg,
        stack_mem,
        global_ptr,
        complex_ptr,
    };

    namespace op {
        struct operand_t {
            const operand_types type;
            ir::value_size size;
            bool address = false;

            explicit operand_t(operand_types type, ir::value_size size) : type(type), size(size) {}
            virtual ~operand_t() = default;

            [[nodiscard]] virtual std::string get_value() const = 0;
            [[nodiscard]] virtual bool equals(const operand_t& other) = 0;
        };
    }

    namespace inst {
        using operand = std::unique_ptr<op::operand_t>;

        struct explicit_register {
            backend::context::register_t reg;
            ir::value_size size;
        };

        struct asm_node {
            bool is_valid = true;

            virtual ~asm_node() = default;
            virtual void print(backend::context::function_context &context) const = 0;
            [[nodiscard]] bool printable() const { return is_valid; }
        };

        struct stack_save : asm_node {
            stack_save() = default;

            ~stack_save() override = default;

            void print(backend::context::function_context &context) const override;
        };

        struct mov : asm_node {
            operand src, dest;

            mov(operand dest, operand src)
                    : src(std::move(src)), dest(std::move(dest)) {
                if (this->src->equals(*this->dest))
                    is_valid = false;
            }

            ~mov() override = default;

            void print(backend::context::function_context &context) const override;
        };

        struct lea : asm_node {
            operand dest;
            operand ptr;

            lea(operand dest, operand ptr)
                    : dest(std::move(dest)), ptr(std::move(ptr)) {
                this->ptr->address = true;
            }

            ~lea() override = default;

            void print(backend::context::function_context &context) const override;
        };

        struct cmov : asm_node {
            ir::block::icmp_type type;
            operand src, dest;

            cmov(ir::block::icmp_type type, operand op1, operand op2)
                    : type(type), src(std::move(op1)), dest(std::move(op2)) {}

            ~cmov() override = default;

            void print(backend::context::function_context &context) const override;
        };

        struct movsx : asm_node {
            operand src, dest;

            movsx(operand src, operand dest)
                    : src(std::move(src)), dest(std::move(dest)) {}

            ~movsx() override = default;

            void print(backend::context::function_context &context) const override;
        };

        struct set : asm_node {
            ir::block::icmp_type type;
            operand op;

            set(ir::block::icmp_type type, operand op)
                    : type(type), op(std::move(op)) {
                debug::assert(this->op->type == operand_types::reg, "set operand must be a register");
                debug::assert(this->op->size == ir::value_size::i1, "set operand must be a i1");
            }

            ~set() override = default;

            void print(backend::context::function_context &context) const override;
        };

        struct jmp : asm_node {
            std::string label_name;

            explicit jmp(std::string label_name)
                    : label_name(std::move(label_name)) {}

            ~jmp() override = default;

            void print(backend::context::function_context &context) const override;
        };

        struct cmp : asm_node {
            operand oper1, oper2;

            cmp(operand oper1, operand oper2)
                    : oper1(std::move(oper1)), oper2(std::move(oper2)) {}

            ~cmp() override = default;

            void print(backend::context::function_context &context) const override;
        };

        struct cond_jmp : asm_node {
            ir::block::icmp_type type;
            std::string branch_name;

            cond_jmp(ir::block::icmp_type type, std::string branch_name)
                    : type(type), branch_name(std::move(branch_name)) {}

            ~cond_jmp() override = default;

            void print(backend::context::function_context &context) const override;
        };

        struct arithmetic : asm_node {
            ir::block::arithmetic_type type;
            operand oper1, oper2;

            arithmetic(ir::block::arithmetic_type type, operand oper1, operand oper2)
                    : type(type), oper1(std::move(oper1)), oper2(std::move(oper2)) {}

            ~arithmetic() override = default;

            void print(backend::context::function_context &context) const override;
        };

        struct call : asm_node {
            std::string function_name;

            explicit call(std::string function_name)
                    : function_name(std::move(function_name)) {}

            ~call() override = default;

            void print(backend::context::function_context &context) const override;
        };

        struct ret : asm_node {
            ret() = default;
            ~ret() override = default;

            void print(backend::context::function_context &context) const override;
        };
    }

    struct label {
        std::string name;
        std::vector<std::unique_ptr<inst::asm_node>> nodes;
    };

    std::unique_ptr<backend::as::op::operand_t> create_operand(const context::virtual_memory *vptr, ir::value_size size);
    std::unique_ptr<backend::as::op::operand_t> create_operand(ir::int_literal lit, ir::value_size size);

    std::unique_ptr<backend::as::op::operand_t> create_operand(const context::virtual_memory *vptr);
    std::unique_ptr<backend::as::op::operand_t> create_operand(ir::int_literal lit);

    std::unique_ptr<backend::as::op::operand_t> create_operand(backend::context::register_t reg, ir::value_size size);
    std::unique_ptr<backend::as::op::operand_t> create_operand(const backend::context::memory_addr& ptr);
}