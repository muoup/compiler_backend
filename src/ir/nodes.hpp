#pragma once

#include "nodes.hpp"
#include "../backend/ir_analyzer/node_metadata.hpp"

#include <string>
#include <utility>
#include <vector>
#include <memory>
#include <variant>
#include <optional>
#include <exception>

#define PRINT_DEF(node_name, ...) void print(std::ostream &ostream) const override { __inst_print(ostream, node_name, ##__VA_ARGS__); }

namespace ir {
    namespace global {
        struct global_node;
    }

    struct node {};

    /**
     *  Represents a constant integer value. The size of which
     *  should match the size of the operation it is involved in.
     */
    struct int_literal {
        uint64_t value;

        explicit int_literal(uint64_t value) : value(value) {}
        void print(std::ostream &ostream) const {
            ostream << (unsigned int) value;
        }
    };

    /**
     *  Seen in IR as either %[name] or %ptr [name]. Represents data
     *  stored somewhere in storage. There is no guarantee it will be
     *  stored in a register/memory, or even that it is directly stored
     *  at all.
     */
    struct variable {
        std::string name;
        bool is_pointer;

        explicit variable(std::string name, bool is_pointer) : name(std::move(name)), is_pointer(is_pointer) {}
        void print(std::ostream &ostream) const {
            ostream << "%";
            if (is_pointer) ostream << "ptr ";
            ostream << name;
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
    };

    namespace block {
        template <typename... args>
        static inline void __inst_print(std::ostream& ostream, const char* node_name, args... arg) {
            ostream << node_name;
            ((ostream << " " << arg), ...);
        }

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
        struct instruction : virtual node {
            virtual void print(std::ostream&) const = 0;
            virtual ~instruction() = default;
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

            std::unique_ptr<backend::instruction_metadata> metadata { nullptr};

            explicit block_instruction(std::unique_ptr<instruction> instruction,
                                       std::vector<value> operands)
                :   inst(std::move(instruction)), operands(std::move(operands)) {}

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
         *  Guarantees that there is a space in memory of @size bytes that can
         *  be written and read from without the possibility of said memory
         *  being overwritten, and returns a logical pointer to said memory.
         *
         *  This does not necessarily guarantee stack memory, nor does it
         *  guarantee that unreferenced parts of this memory will actually
         *  exist, only that there is some referencable memory of @size bytes.
         */
        struct allocate : instruction {
            size_t size;

            explicit allocate(size_t allocation_size)
                : size(allocation_size) {}

            PRINT_DEF("allocate", size);
            ~allocate() override = default;
        };

        /**
         *  Given a logical pointer and an operand of @size bytes, stores
         *  that value in the memory referenced by the pointer.
         */
        struct store : instruction {
            uint64_t size;

            explicit store(uint64_t size)
                    : size(size) {}

            PRINT_DEF("store", size);
            ~store() override = default;
        };

        /**
         *  Given a logical pointer, returns @size bytes of data from the
         *  referenced memory.
         */
        struct load : instruction {
            uint64_t size;

            explicit load(uint64_t size)
                : size(size) {}

            PRINT_DEF("load", size);
            ~load() override = default;
        };

        /**
         *  Branches depending on the provided condition, @true_branch if non-zero
         *  and @false_branch if zero.
         */
        struct branch : instruction {
            std::string true_branch;
            std::string false_branch;

            explicit branch(std::string false_branch, std::string true_branch)
                : true_branch(std::move(true_branch)), false_branch(std::move(false_branch)) {}

            PRINT_DEF("branch", true_branch, false_branch);
            ~branch() override = default;
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
                : type(type) {}

            PRINT_DEF("icmp", icmp_str(type));
            ~icmp() override = default;
        };

        /**
         *  Invokes a subroutine. Will ensure that all parameters specified are stored in the appropriate
         *  memory location so that the subroutine can unconditionally reference the parameters it requires.
         */
        struct call : instruction {
            std::string name;

            explicit call(std::string name)
                    : name(std::move(name)) {}

            PRINT_DEF("call", name);
            ~call() override = default;
        };

        /**
         *  Returns from a subroutine back to the callee.
         *
         *  TODO: Handle return values
         */
        struct ret : instruction {
            PRINT_DEF("ret");
            ~ret() override = default;
        };

        enum arithmetic_type {
            iadd, isub, imul, idiv, imod
        };

        inline const char* arithmetic_name(arithmetic_type type) {
            switch (type) {
                case iadd: return "iadd";
                case isub: return "isub";
                case imul: return "imul";
                case idiv: return "idiv";
                case imod: return "imod";
            }
        }

        struct arithmetic : instruction {
            arithmetic_type type;

            explicit arithmetic(arithmetic_type type)
                : type(type) {}

            PRINT_DEF(arithmetic_name(type));
            ~arithmetic() override = default;
        };

        /**
         *  Basic algebraic operator. Adds all arguments provided and
         *  stores their result in the assignment variable.
         */
        struct add : instruction {
            PRINT_DEF("add");
            ~add() override = default;
        };

        /**
         *  Basic algebraic operator. Adds all arguments provided and
         *  stores their result in the assignment variable.
         */
        struct sub : instruction {
            PRINT_DEF("sub");
            ~sub() override  = default;
        };
    }

    namespace global {
        struct global_node : node {};

        enum parameter_type : uint8_t {
            i8, i16, i32, i64, ptr
        };

        struct parameter {
            parameter_type type;
            std::string name;
        };

        struct global_string : global_node {
            std::string name;
            std::string value;

            explicit global_string(std::string name, std::string value)
                : name(std::move(name)), value(std::move(value)) {}
        };

        struct extern_function : global_node {
            std::string name;
            std::vector<parameter> parameters;

            explicit extern_function(std::string name, std::vector<parameter> parameters)
                : name(std::move(name)), parameters(std::move(parameters)) {}
        };

        struct function : global_node {
            std::string name;
            std::vector<parameter> parameters;
            std::vector<block::block> blocks;

            std::unique_ptr<backend::function_metadata> metadata { nullptr };

            explicit function(std::string name, std::vector<parameter> parameters, std::vector<block::block> blocks)
                : name(std::move(name)), parameters(std::move(parameters)), blocks(std::move(blocks)) {}
        };
    }

    struct root : node {
        std::vector<global::global_string> global_strings;
        std::vector<global::extern_function> extern_functions;
        std::vector<global::function> functions;

        root() = default;
    };
}