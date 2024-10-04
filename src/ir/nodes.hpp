#pragma once

#include "nodes.hpp"
#include "../backend/ir_analyzer/analysis_nodes.hpp"

#include <string>
#include <utility>
#include <vector>
#include <memory>
#include <variant>
#include <optional>

namespace ir {
    namespace global {
        struct global_node;
    }

    struct node {
    };

    struct literal {
        uint64_t value;

        explicit literal(uint64_t value) : value(value) {}
    };

    struct variable {
        bool is_pointer;
        std::string name;

        explicit variable(std::string name, bool is_pointer) : name(std::move(name)), is_pointer(is_pointer) {}
    };

    struct value {
        std::variant<literal, variable> val;

        explicit value(literal val) : val(val) {}
        explicit value(variable val) : val(std::move(val)) {}
    };

    namespace block {
        struct instruction : node {};

        struct block_instruction : node {
            std::unique_ptr<instruction> inst;
            std::vector<value> operands;
            std::optional<variable> assigned_to;

            std::unique_ptr<backend::instruction_metadata> metadata { nullptr};

            explicit block_instruction(std::unique_ptr<instruction> instruction,
                                       std::vector<value> operands)
                :   inst(std::move(instruction)), operands(std::move(operands)) {}
        };

        struct block : node {
            std::string name;
            std::vector<block_instruction> instructions {};

            explicit block(std::string name) : name(std::move(name)), instructions() {};
        };

        struct allocate : instruction {
            size_t size;

            allocate(size_t allocation_size)
                : size(allocation_size) {}
        };

        struct store : instruction {
            uint8_t size;

            explicit store(uint8_t size)
                    : size(size) {}
        };

        struct load : instruction {
            uint8_t size;

            explicit load(uint8_t size)
                : size(size) {}
        };

        struct branch : instruction {
            std::string true_branch;
            std::string false_branch;

            explicit branch(std::string true_branch, std::string false_branch)
                : true_branch(std::move(true_branch)), false_branch(std::move(false_branch)) {}
        };

        enum icmp_type : uint8_t {
            // Bits : is_signed | is_greater_than | is_equal | is_less_than

            eq = 0b0010,
            neq = 0b0000,

            slt = 0b1011,
            sgt = 0b1101,
            sle = 0b1010,
            sge = 0b1100,

            ult = 0b1011,
            ugt = 0b1101,
            ule = 0b1010,
            uge = 0b1100
        };

        struct icmp : instruction {
            icmp_type type;

            explicit icmp(icmp_type type)
                : type(type) {}
        };

        struct call : instruction {
            std::string name;

            explicit call(std::string name)
                    : name(std::move(name)) {}
        };

        struct ret : instruction {};
        struct add : instruction {};
        struct sub : instruction {};
    }

    namespace global {
        struct global_node : node {};

        enum parameter_type {
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