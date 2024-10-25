#pragma once

#include <ostream>
#include <utility>
#include <iomanip>

#include "../instructions.hpp"
#include "../codegen.hpp"

namespace backend::as {
    enum operand_types : uint8_t {
        literal,
        reg,
        stack_mem
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

        struct reg : operand_t {
            backend::codegen::register_t index;

            explicit reg(backend::codegen::register_t index, uint8_t size)
                : operand_t(operand_types::reg, size), index(index) {}
            ~reg() override = default;

            [[nodiscard]] std::string get_address() const override {
                return backend::codegen::register_as_string(index, size);
            }
            [[nodiscard]] bool equals(const operand_t& other) override {
                if (other.type != operand_types::reg)
                    return false;

                return dynamic_cast<const reg&>(other).index == index;
            }
        };

        struct imm : operand_t {
            uint64_t val;

            explicit imm(uint64_t val, uint8_t size)
                : operand_t(operand_types::literal, size), val(val) {}
            ~imm() override = default;

            [[nodiscard]] std::string get_address() const override {
                return std::to_string(val);
            }
            [[nodiscard]] bool equals(const operand_t& other) override {
                if (other.type != operand_types::literal)
                    return false;

                return dynamic_cast<const imm&>(other).val == val;
            }
        };

        struct stack_memory : operand_t {
            size_t rbp_off;

            explicit stack_memory(size_t rbp_off, uint8_t size)
                : operand_t(operand_types::stack_mem, size), rbp_off(rbp_off) {}

            [[nodiscard]] std::string get_address() const override {
                return std::string("[rbp + ") + std::to_string(rbp_off) + "]";
            }
            [[nodiscard]] bool equals(const operand_t& other) override {
                if (other.type != operand_types::stack_mem)
                    return false;

                return dynamic_cast<const stack_memory&>(other).rbp_off == rbp_off;
            }
        };
    }

    namespace inst {
        using operand = std::unique_ptr<op::operand_t>;

        inline void print_inst(std::ostream &ostream, const char* name) {
            ostream << '\t' << std::setw(8) << std::left << name;
        }

        inline void print_inst(std::ostream &ostream, const char* name, const operand &oper1) {
            print_inst(ostream, name);
            ostream << oper1->get_address();
        }

        inline void print_inst(std::ostream &ostream, const char* name, const operand &oper1, const operand &oper2) {
            print_inst(ostream, name, oper1);
            ostream << ", ";
            ostream << oper2->get_address();
        }

        struct asm_node {
            bool is_valid = true;

            virtual ~asm_node() = default;
            virtual void print(backend::codegen::function_context &context) const = 0;
            [[nodiscard]] bool printable() const { return is_valid; }
        };

        struct stack_save : asm_node {
            stack_save() = default;
            ~stack_save() override = default;

            void print(backend::codegen::function_context &context) const override {
                for (size_t i = 0; i < backend::codegen::register_count; i++) {
                    if (!context.register_tampered[i]) continue;

                    print_inst(context.ostream, "push");
                    context.ostream << backend::codegen::register_as_string((backend::codegen::register_t) i, 8) << '\n';
                }

                if (context.current_stack_size == 0)
                    return;

                print_inst(context.ostream, "push");
                context.ostream << "rbp\n";

                print_inst(context.ostream, "mov");
                context.ostream << "rbp, rsp\n";

                print_inst(context.ostream, "sub");
                context.ostream << "rsp, " << context.current_stack_size;
            }
        };

        struct label : asm_node {
            std::string name;

            explicit label(std::string_view name)
                : name(name) {}
            ~label() override = default;

            void print(backend::codegen::function_context &context) const override {
                context.ostream << "." << name << ":";
            }
        };

        struct mov : asm_node {
            operand op1, op2;

            mov(operand op1, operand op2)
                : op1(std::move(op1)), op2(std::move(op2)) {
                if (this->op1->equals(*this->op2))
                    is_valid = false;
            }
            ~mov() override = default;

            void print(backend::codegen::function_context &context) const override {
                print_inst(context.ostream, "mov");
                context.ostream << op1->get_address() << ", " << op2->get_address();
            }
        };

        struct jmp : asm_node {
            std::string label_name;

            explicit jmp(std::string label_name)
                : label_name(std::move(label_name)) {}
            ~jmp() override = default;

            void print(backend::codegen::function_context &context) const override {
                print_inst(context.ostream, "jmp");
                context.ostream << label_name;
            }
        };

        struct cmp : asm_node {
            operand oper1, oper2;

            cmp(operand oper1, operand oper2)
                : oper1(std::move(oper1)), oper2(std::move(oper2)) {}
            ~cmp() override = default;

            void print(backend::codegen::function_context &context) const override {
                print_inst(context.ostream, "cmp", oper1, oper2);
            }
        };

        struct cond_jmp : asm_node {
            ir::block::icmp_type type;
            std::string branch_name;

            cond_jmp(ir::block::icmp_type type, std::string branch_name)
                : type(type), branch_name(std::move(branch_name)) {}
            ~cond_jmp() override = default;

            void print(backend::codegen::function_context &context) const override {
                print_inst(context.ostream, backend::codegen::jmp_inst(type));
                context.ostream << branch_name;
            }
        };

        struct arithmetic : asm_node {
            ir::block::arithmetic_type type;
            operand oper1, oper2;

            arithmetic(ir::block::arithmetic_type type, operand oper1, operand oper2)
                : type(type), oper1(std::move(oper1)), oper2(std::move(oper2)) {}
            ~arithmetic() override = default;

            void print(backend::codegen::function_context &context) const override {
                const auto* cmd = backend::codegen::arithmetic_command(type);

                print_inst(context.ostream, cmd, oper1, oper2);
            }
        };

        struct call : asm_node {
            std::string function_name;

            explicit call(std::string function_name)
                    : function_name(std::move(function_name)) {}
            ~call() override = default;

            void print(backend::codegen::function_context &context) const override {
                print_inst(context.ostream, "call");
                context.ostream << function_name;
            }
        };

        struct ret : asm_node {
            ret() = default;
            ~ret() override = default;

            void print(backend::codegen::function_context &context) const override {
                for (size_t i = backend::codegen::register_count - 1; i >= 1; i--) {
                    if (!context.register_tampered[i]) continue;

                    print_inst(context.ostream, "pop");
                    context.ostream << backend::codegen::register_as_string((backend::codegen::register_t) i, 8) << '\n';
                }

                if (context.current_stack_size != 0) {
                    print_inst(context.ostream, "leave");
                    context.ostream << '\n';
                }

                print_inst(context.ostream, "ret");
            }
        };
    }

    inline std::unique_ptr<op::operand_t> create_operand(const codegen::vptr *vptr) {
        if (const auto *reg = dynamic_cast<const codegen::register_storage*>(vptr)) {
            return std::make_unique<op::reg>(reg->reg, reg->get_size());
        } else if (const auto *stack = dynamic_cast<const codegen::stack_pointer*>(vptr)) {
            return std::make_unique<op::stack_memory>(stack->rsp_off, stack->alloc_size);
        } else if (const auto *imm = dynamic_cast<const codegen::literal*>(vptr)) {
            return std::make_unique<op::imm>(imm->value, 8);
        }

        throw std::runtime_error("Invalid operand type");
    }
}