#include "asm_nodes.hpp"
#include "../context/function_context.hpp"

namespace backend::as::op {
    struct reg : backend::as::op::operand_t {
        backend::context::register_t index;

        explicit reg(ir::value_size size, backend::context::register_t index)
                : operand_t(operand_types::reg, size), index(index) {}
        ~reg() override = default;

        [[nodiscard]] std::string get_value() const override {
            if (address) {
                return context::get_stack_prefix(size)
                    .append("[")
                    .append(backend::context::register_as_string(index, ir::value_size::ptr))
                    .append("]");
            } else {
                return backend::context::register_as_string(index, size);
            }
        }

        [[nodiscard]] bool equals(const operand_t& other) override {
            if (other.type != operand_types::reg)
                return false;

            return dynamic_cast<const reg&>(other).index == index;
        }
    };

    struct imm : backend::as::op::operand_t {
        uint64_t val;

        explicit imm(ir::value_size size, uint64_t val)
                : operand_t(operand_types::literal, size), val(val) {}
        ~imm() override = default;

        [[nodiscard]] std::string get_value() const override {
            return std::to_string(val);
        }
        [[nodiscard]] bool equals(const operand_t& other) override {
            if (other.type != operand_types::literal)
                return false;

            return dynamic_cast<const imm&>(other).val == val;
        }
    };

    struct stack_memory : operand_t {
        int64_t rbp_off;

        explicit stack_memory(ir::value_size size, int64_t rbp_off)
                : operand_t(operand_types::stack_mem, size), rbp_off(rbp_off) {}

        [[nodiscard]] std::string get_value() const override {
            std::stringstream ss;

            if (!this->address)
                ss << context::get_stack_prefix(size);

            ss << "[rbp";

            if (rbp_off != 0) {
                ss << ((rbp_off < 0) ? " - " : " + ");
                ss << std::abs(rbp_off);
            }

            ss << "]";

            return ss.str();
        }
        [[nodiscard]] bool equals(const operand_t& other) override {
            if (other.type != operand_types::stack_mem)
                return false;

            return dynamic_cast<const stack_memory&>(other).rbp_off == rbp_off;
        }
    };

    struct complex_ptr : operand_t {
        int64_t base;

        std::optional<backend::context::register_t> reg;
        std::optional<int8_t> reg_scale;
        std::optional<backend::context::register_t> unscaled_reg;

        explicit complex_ptr(ir::value_size size, const backend::context::memory_addr &addr)
                : operand_t(operand_types::complex_ptr, size), base(addr.offset) {
            if (addr.scaled.has_value()) {
                reg = addr.scaled->reg;
                reg_scale = addr.scaled->scale;
            }

            if (addr.unscaled.has_value())
                unscaled_reg = addr.unscaled;
        }

        [[nodiscard]] std::string get_value() const override {
            std::stringstream ss;

            if (!this->address)
                ss << context::get_stack_prefix(size);

            ss << "[";

            if (reg_scale) {
                if (*reg_scale != 1)
                    ss << (int) *reg_scale << " * ";

                ss << backend::context::register_as_string(*reg, ir::value_size::ptr);

                ss << " + ";
            }

            if (unscaled_reg.has_value()) {
                ss << backend::context::register_as_string(*unscaled_reg, ir::value_size::ptr);
            }

            if (base != 0) {
                ss << ((base < 0) ? " - " : " + ");
                ss << std::abs(base);
            }

            ss << "]";

            return ss.str();
        }
        [[nodiscard]] bool equals(const operand_t& other) override {
            if (other.type != operand_types::complex_ptr)
                return false;

            auto &other_ptr = dynamic_cast<const complex_ptr&>(other);
            return other_ptr.base == base && other_ptr.reg == reg && other_ptr.reg_scale == reg_scale;
        }
    };

    struct global_pointer : operand_t {
        std::string name;

        explicit global_pointer(std::string name)
                : operand_t(operand_types::global_ptr, ir::value_size::ptr), name(std::move(name)) {}

        [[nodiscard]] std::string get_value() const override {
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
        ostream << oper1->get_value();
    }

    static void print_inst(std::ostream &ostream, const char* name, const operand &oper1, const operand &oper2) {
        print_inst(ostream, name, oper1);
        ostream << ", ";
        ostream << oper2->get_value();
    }

    void stack_save::print(backend::context::function_context &context) const {
        if (context.current_stack_size != 0) {
            print_inst(context.ostream, "push");
            context.ostream << "rbp\n";

            print_inst(context.ostream, "mov");
            context.ostream << "rbp, rsp\n";

            print_inst(context.ostream, "sub");
            context.ostream << "rsp, " << context.current_stack_size << '\n';
        }

        for (size_t i = 1; i < backend::context::register_count; i++) {
            if (!context.storage.registers[i]->tampered || context.register_is_param[i]) continue;

            print_inst(context.ostream, "push");
            context.ostream << backend::context::register_as_string((backend::context::register_t) i, ir::value_size::i64);

            if (i != backend::context::register_count - 1)
                context.ostream << '\n';
        }
    }

    void mov::print(backend::context::function_context &context) const {
        if (src->get_value() == "0") {
            print_inst(context.ostream, "xor", dest, dest);
            return;
        }

        print_inst(context.ostream, "mov", dest, src);
    }

    void lea::print(backend::context::function_context &context) const {
        print_inst(context.ostream, "lea", dest, ptr);
    }

    void cmov::print(backend::context::function_context &context) const {
        print_inst(context.ostream, cond_inst("cmov", type).c_str(), src, dest);
    }

    void movsx::print(backend::context::function_context &context) const {
        print_inst(context.ostream, "movsx", src, dest);
    }

    void set::print(backend::context::function_context &context) const {
        print_inst(context.ostream, cond_inst("set", type).c_str(), op);
    }

    void jmp::print(backend::context::function_context &context) const {
        print_inst(context.ostream, "jmp");
        context.ostream << "." << label_name;
    }

    void cmp::print(backend::context::function_context &context) const {
        print_inst(context.ostream, "cmp", oper1, oper2);
    }

    void cond_jmp::print(backend::context::function_context &context) const {
        print_inst(context.ostream, cond_inst("j", type).c_str());
        context.ostream << "." << branch_name;
    }

    void arithmetic::print(backend::context::function_context &context) const {
        const auto *cmd = arithmetic_command(type);

        print_inst(context.ostream, cmd, oper1, oper2);
    }

    void call::print(backend::context::function_context &context) const {
        print_inst(context.ostream, "call");
        context.ostream << function_name;
    }

    void ret::print(backend::context::function_context &context) const {
        for (size_t i = backend::context::register_count - 1; i >= 1; i--) {
            if (!context.storage.registers[i]->tampered || context.register_is_param[i]) continue;

            print_inst(context.ostream, "pop");
            context.ostream << backend::context::register_as_string((backend::context::register_t) i, ir::value_size::i64) << '\n';
        }

        if (context.current_stack_size != 0) {
            print_inst(context.ostream, "leave");
            context.ostream << '\n';
        }

        print_inst(context.ostream, "ret");
    }
}

std::unique_ptr<backend::as::op::operand_t> backend::as::create_operand(const context::virtual_memory *vptr, ir::value_size size) {
    if (const auto *reg = dynamic_cast<const context::register_storage*>(vptr)) {
        return std::make_unique<op::reg>(size, reg->reg);
    } else if (const auto *memory_ptr = dynamic_cast<const context::memory_addr *>(vptr)) {
        return std::make_unique<op::complex_ptr>(size, *memory_ptr);
    } else if (const auto *global = dynamic_cast<const context::global_pointer*>(vptr)) {
        return std::make_unique<op::global_pointer>(global->name);
    }

    throw std::runtime_error("Invalid operand type");
}

std::unique_ptr<backend::as::op::operand_t> backend::as::create_operand(ir::int_literal lit, ir::value_size size) {
    return std::make_unique<op::imm>(size, lit.value);
}

std::unique_ptr<backend::as::op::operand_t> backend::as::create_operand(ir::int_literal lit) {
    return create_operand(lit, lit.size);
}

std::unique_ptr<backend::as::op::operand_t> backend::as::create_operand(const context::virtual_memory *vptr) {
    return create_operand(vptr, vptr->size);
}

std::unique_ptr<backend::as::op::operand_t> backend::as::create_operand(backend::context::register_t reg, ir::value_size size) {
    return std::make_unique<op::reg>(size, reg);
}

std::unique_ptr<backend::as::op::operand_t> backend::as::create_operand(const backend::context::memory_addr& ptr) {
    return std::make_unique<op::complex_ptr>(ptr.size, ptr);
}