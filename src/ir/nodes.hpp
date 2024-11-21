#pragma once

#include "nodes.hpp"
#include "../backend/ir_analyzer/node_metadata.hpp"
#include "../debug/assert.hpp"

#include <string>
#include <utility>
#include <vector>
#include <memory>
#include <variant>
#include <optional>
#include <exception>

#define PRINT_DEF(node_name, ...) void print(std::ostream &ostream) const override { __inst_print(ostream, node_name, ##__VA_ARGS__); }
#define VISITOR_DEF(node_name) void accept(auto fn) { fn(*this); }

namespace ir {
    namespace global {
        struct global_node;
    }

    enum class value_size : uint8_t {
        i1, i8, i16, i32, i64, ptr,

        // Not for parser use, nodes used in codegen
        none, param_dependent
    };

    inline const char* value_size_str(value_size size) {
        switch (size) {
            case value_size::none: return "void";
            case value_size::i1: return "i1";
            case value_size::i8: return "i8";
            case value_size::i16: return "i16";
            case value_size::i32: return "i32";
            case value_size::i64: return "i64";
            case value_size::ptr: return "ptr";

            default:
                debug::assert(false, "value size not specified");
        }

        throw std::runtime_error("unreachable");
    }

    inline int size_in_bytes(value_size size) {
        switch (size) {
            case value_size::i1:
            case value_size::i8: return 1;
            case value_size::i16: return 2;
            case value_size::i32: return 4;
            case value_size::i64:
            case value_size::ptr: return 8;

            default:
                debug::assert(false, "value size not specified");
        }

        throw std::runtime_error("unreachable");
    }

    struct node {};

    /**
     *  Represents a constant integer value. The size of which
     *  should match the size of the operation it is involved in.
     */
    struct int_literal {
        value_size size;
        uint64_t value;

        explicit int_literal(value_size size, uint64_t value) : size(size), value(value) {}
        void print(std::ostream &ostream) const {
            ostream << value_size_str(size) << " " << (unsigned int) value;
        }
    };

    /**
     *  Seen in IR as either [size] %[name] or [size] %[name]. Represents data
     *  stored somewhere in storage. There is no guarantee it will be
     *  stored in a register/stack memory, or even that it is directly stored
     *  at all if not required.
     */
    struct variable {
        value_size size;
        std::string name;

        explicit variable(value_size size, std::string name)
            :   size(size), name(std::move(name)) {}

        void print(std::ostream &ostream) const {
            if (size != value_size::param_dependent && size != value_size::none)
                ostream << value_size_str(size) << " ";

            ostream << "%" << name;
        }
    };

    /**
     *  A value is a generic operand type. If a variable is necessary
     *  for instance in noting what an instruction is assigned to,
     *  use @variable instead, and similarly for @int_literal.
     */
    struct value {
        std::variant<int_literal, variable> val;

        explicit value(int_literal val) : val(val) {}
        explicit value(variable val) : val(std::move(val)) {}

        friend std::ostream& operator <<(std::ostream &ostream, const value& value) {
            std::visit([&](auto&& arg) { arg.print(ostream); }, value.val);
            return ostream;
        }

        [[nodiscard]] value_size get_size() const {
            return std::visit([](auto&& arg) -> value_size { return arg.size; }, val);
        }
        [[nodiscard]] std::string_view get_name() const {
            if (is_literal())
                throw std::runtime_error("Cannot get name of literal");

            return var().name;
        }
        [[nodiscard]] bool is_variable() const {
            return std::holds_alternative<variable>(val);
        }
        [[nodiscard]] bool is_literal() const {
            return std::holds_alternative<int_literal>(val);
        }
        [[nodiscard]] const ir::variable &var() const {
            return std::get<ir::variable>(val);
        }
        [[nodiscard]] const ir::int_literal &lit() const {
            return std::get<ir::int_literal>(val);
        }
    };

    namespace block {
        template <typename T>
        inline void __val_print(std::ostream& ostream, const T& arg) {
            ostream << " " << arg;
        }

        template <>
        inline void __val_print<value_size>(std::ostream& ostream, const value_size& arg) {
            ostream << " " << value_size_str(arg);
        }

        template <typename... args>
        inline void __inst_print(std::ostream& ostream, const char* node_name, args... arg) {
            ostream << node_name;
            (__val_print(ostream, arg), ...);
        }

        enum class node_type {
            literal, allocate, store, load,
            branch, jmp, icmp,
            call, ret,
            arithmetic, phi, select,
            sext, zext,
            get_array_ptr,
        };

        /**
         *  The IR unit of a block. The contract of an instruction
         *  does not promise a certain action will be performed,
         *  but that the side effects intended by the instruction
         *  will be encapsulated within the code generated.
         *
         *  For instance, useless code whose side effects are non-existent,
         *  like an 'add' instruction with no assignment, will be ignored,
         *  as there is no intended functionality with the instruction.
         */
        struct instruction : node {
            const node_type type;

            explicit instruction(node_type type) : type(type) {}
            virtual ~instruction() = default;

            virtual void print(std::ostream&) const = 0;

            [[nodiscard]] virtual bool dropped_reassignable() const { return true; }
            [[nodiscard]] virtual ir::value_size get_return_size() const { return ir::value_size::none; }
        };

        /**
         *  Container struct for an instruction. To encapsulate the virtual
         *  nature of an instruction, data handling of instructions is done
         *  internally by the struct.
         */
        struct block_instruction : node {
            std::unique_ptr<instruction> inst;
            std::vector<value> operands;
            std::optional<variable> assigned_to;

            std::unique_ptr<backend::md::instruction_metadata> metadata { nullptr};

            explicit block_instruction(std::unique_ptr<instruction> instruction,
                                       std::vector<value> operands)
                :   inst(std::move(instruction)), operands(std::move(operands)) {
                if (assigned_to == std::nullopt)
                    return;

                const auto size = instruction->get_return_size();
                debug::assert(size != ir::value_size::none, "instruction with no return size assigned to variable");

                if (size == ir::value_size::param_dependent) {
                    debug::assert(!operands.empty(), "param dependent size with multiple operands");

                    auto o0_size = operands[0].get_size();

                    for (const auto &operand : operands) {
                        debug::assert(operand.get_size() == o0_size, "param dependent size with mismatched operand sizes");
                    }

                    assigned_to->size = o0_size;
                }
            }

            void print(std::ostream &ostream) const;
        };

        /**
         *  The organizational unit of a function body. Essentially represents
         *  a local label within a subroutine in assembly.
         */
        struct block : node {
            std::string name;
            std::vector<block_instruction> instructions {};

            explicit block(std::string name) : name(std::move(name)), instructions() {};
        };

        /**
         *  Allows an alias to be created for an integer literal. This will
         *  automatically be folded as a constant and will never be allocated
         *  in actual memory.
         */
        struct literal : instruction {
            ir::int_literal value;

            explicit literal(ir::int_literal value)
                : instruction(node_type::literal), value(value) {}
            ~literal() override = default;

            void print(std::ostream &ostream) const override {
                ostream << value.value;
            }
            VISITOR_DEF();

            [[nodiscard]] ir::value_size get_return_size() const override { return value.size; }
        };

        /**
         *  Guarantees that there is a space in stack memory of @size bytes that can
         *  be written and read from without the possibility of said stack memory
         *  being overwritten, and returns a logical pointer to said stack memory.
         *
         *  This does not necessarily guarantee stack stack memory, nor does it
         *  guarantee that unreferenced parts of this stack memory will actually
         *  exist, only that there is some referencable stack memory of @size bytes.
         */
        struct allocate : instruction {
            size_t size;

            explicit allocate(size_t allocation_size)
                :   instruction(node_type::allocate), size(allocation_size) {}
            ~allocate() override = default;

            PRINT_DEF("allocate", size);
            VISITOR_DEF();

            [[nodiscard]] ir::value_size get_return_size() const override { return ir::value_size::ptr; }
        };

        /**
         *  Given a logical pointer and an operand of @size bytes, stores
         *  that value in the stack memory referenced by the pointer.
         */
        struct store : instruction {
            value_size size;

            explicit store(value_size size)
                :   instruction(node_type::store), size(size) {}
            ~store() override = default;

            PRINT_DEF("store", size);
            VISITOR_DEF();
        };

        /**
         *  Given a logical pointer, returns @size bytes of data from the
         *  referenced stack memory.
         */
        struct load : instruction {
            value_size size;

            explicit load(value_size size)
                :   instruction(node_type::load), size(size) {}
            ~load() override = default;

            PRINT_DEF("load", ir::value_size_str(size));
            VISITOR_DEF();

            [[nodiscard]] ir::value_size get_return_size() const override { return size; }
        };

        /**
         *  Branches depending on the provided condition, @true_branch if non-zero
         *  and @false_branch if zero.
         */
        struct branch : instruction {
            std::string true_branch;
            std::string false_branch;

            explicit branch(std::string false_branch, std::string true_branch)
                :   instruction(node_type::branch),
                    true_branch(std::move(true_branch)),
                    false_branch(std::move(false_branch)) {}
            ~branch() override = default;

            PRINT_DEF("branch", true_branch, false_branch);
            VISITOR_DEF();
        };

        /**
         *  Unconditional jump to @label.
         */
        struct jmp : instruction {
            std::string label;

            explicit jmp(std::string branch)
                : instruction(node_type::jmp),
                  label(std::move(branch)) {}
            ~jmp() override = default;

            PRINT_DEF("jmp", label);
            VISITOR_DEF();
        };

        enum icmp_type : uint8_t {
            // Bits : is_signed | is_greater_than | is_equal | is_less_than

            eq  = 0b0010,
            neq = 0b0101,

            slt = 0b1001,
            sgt = 0b1100,
            sle = 0b1011,
            sge = 0b1110,

            ult = 0b0001,
            ugt = 0b0100,
            ule = 0b0011,
            uge = 0b0110
        };

        inline const char* icmp_str(icmp_type type) {
            switch (type) {
                case eq: return "eq";
                case neq: return "neq";

                case slt: return "slt";
                case sgt: return "sgt";
                case sle: return "sle";
                case sge: return "sge";

                case ult: return "ult";
                case ugt: return "ugt";
                case ule: return "ule";
                case uge: return "uge";

                default: throw std::runtime_error("no such icmp type");
            }
        }

        /**
         *  Must immediately precede a branching instruction; performs a subtraction of two values
         *  and then informs the branch of the icmp_type so that it can generate the appropriate
         *  conditional jump instruction.
         */
        struct icmp : instruction {
            icmp_type type;

            explicit icmp(icmp_type type)
                :   instruction(node_type::icmp), type(type) {}
            ~icmp() override = default;

            PRINT_DEF("icmp", icmp_str(type));
            VISITOR_DEF();

            [[nodiscard]] ir::value_size get_return_size() const override { return ir::value_size::i1; }
        };

        /**
         *  Invokes a subroutine. Will ensure that all parameters specified are stored in the appropriate
         *  stack memory location so that the subroutine can unconditionally reference the parameters it requires.
         */
        struct call : instruction {
            value_size return_size;
            std::string name;

            explicit call(std::string name, value_size return_size)
                :   instruction(node_type::call), return_size(return_size), name(std::move(name)) {}
            ~call() override = default;


            PRINT_DEF("call", return_size, name);
            VISITOR_DEF();

            [[nodiscard]] bool dropped_reassignable() const override { return false; }
            [[nodiscard]] ir::value_size get_return_size() const override { return return_size; }
        };

        struct get_array_ptr : instruction {
            value_size element_size;

            explicit get_array_ptr(value_size element_size)
                :   instruction(node_type::get_array_ptr),
                    element_size(element_size) {}
            ~get_array_ptr() override = default;

            PRINT_DEF("get_array_ptr", element_size);
            VISITOR_DEF();

            [[nodiscard]] ir::value_size get_return_size() const override { return ir::value_size::ptr; }
        };

        /**
         *  Returns from a subroutine back to the callee.
         */
        struct ret : instruction {
            ret() : instruction(node_type::ret) {}
            ~ret() override = default;

            PRINT_DEF("ret");
            VISITOR_DEF();
        };

        enum arithmetic_type : uint8_t {
            add, sub, mul, div, mod,
        };

        inline const char* arithmetic_name(arithmetic_type type) {
            switch (type) {
                case add: return "add";
                case sub: return "sub";
                case mul: return "mul";
                case div: return "div";
                case mod: return "mod";

                default: throw std::runtime_error("no such arithmetic type");
            }

            throw std::runtime_error("no such arithmetic type");
        }

        /**
         *  Represents a arithmetic command, which for now is limited
         *  to addition, subtraction, and multiplication.
         */
        struct arithmetic : instruction {
            arithmetic_type type;

            explicit arithmetic(arithmetic_type type)
                :   instruction(node_type::arithmetic), type(type) {}
            ~arithmetic() override = default;

            PRINT_DEF(arithmetic_name(type));
            VISITOR_DEF();

            [[nodiscard]] bool dropped_reassignable() const override { return false; }
            [[nodiscard]] ir::value_size get_return_size() const override { return ir::value_size::i32; }
        };

        /**
         *  Represents a value which differs depending on the branch taken.
         *  For each pair of label and operand, ensures that there is one storage
         *  location where when said label labels to the phi label, the according
         *  value will be stored there for reference in the phi label.
         *
         *  Requires that the amount of provided operands equals the amount of labels.
         */
        struct phi : instruction {
            std::vector<std::string> labels;

            explicit phi(std::vector<std::string> labels)
                : instruction(node_type::phi),
                  labels(std::move(labels)) {}
            explicit phi(std::string branch1, std::string branch2)
                : instruction(node_type::phi),
                  labels {std::move(branch1), std::move(branch2) } {}
            ~phi() override = default;

            void print(std::ostream &ostream) const override {
                __inst_print(ostream, "phi");

                for (const auto &branch : labels) {
                    ostream << " " << branch;
                }
            };

            VISITOR_DEF();

            [[nodiscard]] ir::value_size get_return_size() const override { return ir::value_size::param_dependent; }
        };

        /**
         *  Equivalent essentially to a ternary operator, the first operand accepted
         *  is the condition, if true the second operand is stored in the value, else
         *  the third operand is.
         */
        struct select : instruction {
            select() : instruction(node_type::select) {};
            ~select() override = default;

            PRINT_DEF("select");
            VISITOR_DEF();

            [[nodiscard]] bool dropped_reassignable() const override { return false; }
            [[nodiscard]] ir::value_size get_return_size() const override { return ir::value_size::param_dependent; }
        };

        struct sext : instruction {
            value_size new_size;

            explicit sext(value_size new_size) : instruction(node_type::sext), new_size(new_size) {}
            ~sext() override = default;

            PRINT_DEF("sext", ir::value_size_str(new_size));
            VISITOR_DEF();

            [[nodiscard]] ir::value_size get_return_size() const override { return new_size; }
        };

        struct zext : instruction {
            value_size new_size;

            explicit zext(value_size new_size) : instruction(node_type::zext), new_size(new_size) {}
            ~zext() override = default;

            PRINT_DEF("zext", ir::value_size_str(new_size));
            VISITOR_DEF();

            [[nodiscard]] ir::value_size get_return_size() const override { return new_size; }
        };

        auto node_visit(const block_instruction &inst, auto fn) {
            switch (inst.inst->type) {
                case node_type::literal:
                    return fn(dynamic_cast<literal&>(*inst.inst));
                case node_type::allocate:
                    return fn(dynamic_cast<allocate&>(*inst.inst));
                case node_type::store:
                    return fn(dynamic_cast<store&>(*inst.inst));
                case node_type::load:
                    return fn(dynamic_cast<load&>(*inst.inst));
                case node_type::branch:
                    return fn(dynamic_cast<branch&>(*inst.inst));
                case node_type::jmp:
                    return fn(dynamic_cast<jmp&>(*inst.inst));
                case node_type::icmp:
                    return fn(dynamic_cast<icmp&>(*inst.inst));
                case node_type::call:
                    return fn(dynamic_cast<call&>(*inst.inst));
                case node_type::ret:
                    return fn(dynamic_cast<ret&>(*inst.inst));
                case node_type::arithmetic:
                    return fn(dynamic_cast<arithmetic&>(*inst.inst));
                case node_type::phi:
                    return fn(dynamic_cast<phi&>(*inst.inst));
                case node_type::select:
                    return fn(dynamic_cast<select&>(*inst.inst));
                case node_type::sext:
                    return fn(dynamic_cast<sext&>(*inst.inst));
                case node_type::zext:
                    return fn(dynamic_cast<zext&>(*inst.inst));
                case node_type::get_array_ptr:
                    return fn(dynamic_cast<get_array_ptr&>(*inst.inst));
            }

            throw std::runtime_error("no such instruction type");
        }
    }

    namespace global {
        struct global_node : node {};

        struct global_string : global_node {
            std::string name;
            std::string value;

            explicit global_string(std::string name,
                                   std::string value)
                :   name(std::move(name)),
                    value(std::move(value)) {}
        };

        struct extern_function : global_node {
            std::string name;
            std::vector<variable> parameters;
            value_size return_type;

            explicit extern_function(std::string name,
                                     std::vector<variable> parameters,
                                     value_size return_type)
                : name(std::move(name)),
                  parameters(std::move(parameters)),
                  return_type(return_type) {}
        };

        struct function : global_node {
            std::string name;
            std::vector<variable> parameters;
            std::vector<block::block> blocks;
            value_size return_type;

            std::unique_ptr<backend::md::function_metadata> metadata = nullptr;

            explicit function(std::string name,
                              std::vector<variable> parameters,
                              std::vector<block::block> blocks,
                              value_size return_type)
                :   name(std::move(name)),
                    parameters(std::move(parameters)),
                    blocks(std::move(blocks)),
                    return_type(return_type) {}
        };
    }

    struct root : node {
        std::vector<global::global_string> global_strings;
        std::vector<global::extern_function> extern_functions;
        std::vector<global::function> functions;

        root() = default;
    };
}