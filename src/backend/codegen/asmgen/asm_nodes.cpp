#include "asm_nodes.hpp"
#include "../codegen.hpp"

namespace backend::as::op {
    struct reg : backend::as::op::operand_t {
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

    struct imm : backend::as::op::operand_t {
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

    struct global_pointer : operand_t {
        std::string name;

        explicit global_pointer(std::string name)
                : operand_t(operand_types::global_ptr, 8), name(std::move(name)) {}

        [[nodiscard]] std::string get_address() const override {
            return name;
        }
        [[nodiscard]] bool equals(const operand_t& other) override {
            return other.type == operand_types::global_ptr;
        }
    };
}

namespace backend::as::inst {
    constexpr static std::string cond_inst(const char* prefix, ir::block::icmp_type type) {
        std::string prefix_str { prefix };

        switch (type) {
            using enum ir::block::icmp_type;

            case eq:
                return prefix_str + "e";
            case neq:
                return prefix_str + "ne";

            case slt:
                return prefix_str + "l";
            case sgt:
                return prefix_str + "g";
            case sle:
                return prefix_str + "le";
            case sge:
                return prefix_str + "ge";

            case ult:
                return prefix_str + "b";
            case ugt:
                return prefix_str + "a";
            case ule:
                return prefix_str + "be";
            case uge:
                return prefix_str + "ae";

            default:
                throw std::runtime_error("no such icmp type");
        }
    }

    constexpr static const char* arithmetic_command(ir::block::arithmetic_type type) {
        switch (type) {
            using enum ir::block::arithmetic_type;

            case add:
                return "add";
            case sub:
                return "sub";
            case mul:
                return "imul";
            case div:
                return "idiv";

            default:
                throw std::runtime_error("no such arithmetic type");
        }
    }

    static void print_inst(std::ostream &ostream, const char* name) {
        ostream << '\t' << std::setw(8) << std::left << name;
    }

    static void print_inst(std::ostream &ostream, const char* name, const operand &oper1) {
        print_inst(ostream, name);
        ostream << oper1->get_address();
    }

    static void print_inst(std::ostream &ostream, const char* name, const operand &oper1, const operand &oper2) {
        print_inst(ostream, name, oper1);
        ostream << ", ";
        ostream << oper2->get_address();
    }

    void stack_save::print(backend::codegen::function_context &context) const {
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

    void mov::print(backend::codegen::function_context &context) const {
        print_inst(context.ostream, "mov", src, dest);
    }

    void arith_lea::print(backend::codegen::function_context &context) const {
        print_inst(context.ostream, "lea", dest);

        context.ostream << ", [";

        if (reg1.has_value()) {
            context.ostream
                << backend::codegen::register_as_string((codegen::register_t) reg1.value(), 8)
                << " + ";
        }

        if (reg2.has_value()) {
            if (reg2_mul.has_value()) {
                context.ostream
                        << reg2_mul.value() << " * ";
            }

            context.ostream
                    << backend::codegen::register_as_string((codegen::register_t) reg2.value(), 8);
        }

        if (offset.has_value()) {
            if (reg1.has_value() || reg2.has_value()) {
                if (reg2_mul.has_value() && *reg2_mul < 0) {
                    context.ostream << " - ";
                } else {
                    context.ostream << " + ";
                }
            }

            context.ostream << offset.value();
        }

        context.ostream << "]";
    }

    void cmov::print(backend::codegen::function_context &context) const {
        print_inst(context.ostream, cond_inst("cmov", type).c_str(), src, dest);
    }

    void set::print(backend::codegen::function_context &context) const {
        print_inst(context.ostream, cond_inst("set", type).c_str(), op);
    }

    void jmp::print(backend::codegen::function_context &context) const {
        print_inst(context.ostream, "jmp");
        context.ostream << "." << label_name;
    }

    void cmp::print(backend::codegen::function_context &context) const {
        print_inst(context.ostream, "cmp", oper1, oper2);
    }

    void cond_jmp::print(backend::codegen::function_context &context) const {
        print_inst(context.ostream, cond_inst("j", type).c_str());
        context.ostream << "." << branch_name;
    }

    void arithmetic::print(backend::codegen::function_context &context) const {
        const auto *cmd = arithmetic_command(type);

        print_inst(context.ostream, cmd, oper1, oper2);
    }

    void call::print(backend::codegen::function_context &context) const {
        print_inst(context.ostream, "call");
        context.ostream << function_name;
    }

    void ret::print(backend::codegen::function_context &context) const {
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
}

std::unique_ptr<backend::as::op::operand_t> backend::as::create_operand(const codegen::vptr *vptr, size_t size) {
    if (const auto *reg = dynamic_cast<const codegen::register_storage*>(vptr)) {
        return std::make_unique<op::reg>(reg->reg, size);
    } else if (const auto *stack = dynamic_cast<const codegen::stack_pointer*>(vptr)) {
        return std::make_unique<op::stack_memory>(stack->rsp_off, stack->alloc_size);
    } else if (const auto *imm = dynamic_cast<const codegen::literal*>(vptr)) {
        return std::make_unique<op::imm>(imm->value, size);
    } else if (const auto *global = dynamic_cast<const codegen::global_pointer*>(vptr)) {
        return std::make_unique<op::global_pointer>(global->name);
    }

    throw std::runtime_error("Invalid operand type");
}
